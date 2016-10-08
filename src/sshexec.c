// 2016-05-18  change to block-io 

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <libssh2.h>
#include <unistd.h>
#include <termios.h>
#include <sys/mman.h>
#include <libssh2_config.h>
#include "encoder.h"

#define SSH_PORT 22


ulong map_file(char* fname, char** buf)
{
    int pgsize = sysconf(_SC_PAGESIZE);
    int fd = open(fname, O_RDWR|O_EXCL);
    if(fd < 0)
    {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
    
    struct stat st;
    if ((stat(fname, &st)) < 0)
    {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
    
    ulong map_size = (st.st_size/pgsize+1)*pgsize;
    *buf = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if(*buf == MAP_FAILED)
    {
        printf("mmap: %s\n", strerror(errno));
        exit(1);
    }
    return map_size;
}

int getpassword(char* pass, int len, char* input_code_prompt)
{
    struct termios oldt, newt;
    printf("%s", input_code_prompt);
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON |ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW, &newt);
    int i = 0;
    char c;
    while(i < len)
    {
        c = getchar();
        if(c == '\n') break;
        pass[i] = c;
        i++;
    }
    pass[len] = '\0';
    tcsetattr(STDIN_FILENO,TCSANOW,&oldt);
    printf("\n");
    return i;
}

char* next_line(char* data)
{
    while(*data != '\n' && *data != '\r' && *data != EOF) data++;
    while((*data == '\n' || *data == '\r') && *data != EOF) data++;
    return data;
}

char* next_column(char* data)
{
    while(*data != ' ' &&*data != '\t' && *data != EOF ) data++;
    while((*data == ' ' || *data == '\t') && *data != EOF) data++;
    return data;
}

int column_length(char* data)
{
    int len = 0;
    while(*data != ' ' &&*data != '\t' && *data != EOF )
    {
        data++;
        len++;
    }
    return len;
}

int col_cmp(char* data, char* str)
{
    char col[BUFSIZ];
    int  col_len = 0;
    if(*data == EOF) return -1;
    col_len = column_length(data);
    strncpy(col, data, col_len);
    col[col_len] = '\0';
    return strcmp(str, col);
}

int decode_password(char* pass, char* fname, char* ip, char* uname)
{
    char code[9];
    ulong map_size = 0;
    char* buf  = NULL;
    char* data = NULL;
    char col[BUFSIZ];
    int  col_len = 0;
    
    
    getpassword(code, 8, "input encrypt code(8 char): ");
    map_size = map_file(fname, &buf);
    
    data = buf;
    while(*data != EOF)
    {
        // get ip
        if(col_cmp(data, ip) != 0)
        {
            data = next_line(data);
            continue;
        }
        
        // get username
        data = next_column(data);
        if(*data == EOF) break;
        if(col_cmp(data, uname) != 0)
        {
            data = next_line(data);
            continue;
        }
        
        // get password length
        data = next_column(data);
        col_len = column_length(data);
        strncpy(col, data, col_len);
        col[col_len] = '\0';
        long p_len = atol(col);
        
        data += (col_len+1);
        memcpy(pass, data, p_len);
        pass[p_len] = '\0';
        decode(pass, p_len, code);
        break;
    }
    munmap(buf, map_size);
    
}

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);
    FD_SET(socket_fd, &fd);

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);
    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND) readfd = &fd;
    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) writefd = &fd;

    rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);
    return rc;
}

int read_channel(LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel, int sock)
{
    char buffer[0x4000];
//    for(;;)
//    {
        int rc;
        do
        {
            if((rc = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0)
            {
                int i;
                for( i=0; i<rc; i++) fputc( buffer[i], stdout);
            }
        }
        while( rc > 0 );
//        if(libssh2_channel_eof(channel) == 1) break;
//        if(rc == LIBSSH2_ERROR_EAGAIN) waitsocket(sock, session);
//        else break;
//    }
    printf("\n");
}

int exec_one_cmd(char* cmd, LIBSSH2_SESSION *session, LIBSSH2_CHANNEL **channel, int sock)
{
    int rc;
    //while((*channel = libssh2_channel_open_session(session)) == NULL && libssh2_session_last_error(session,NULL,NULL,0) == LIBSSH2_ERROR_EAGAIN ) waitsocket(sock, session);
    *channel = libssh2_channel_open_session(session);
    if( *channel == NULL )
    {
        fprintf(stderr,"Error\n");
        exit(1);
    }
    
//    while((rc = libssh2_channel_exec(*channel, cmd)) == LIBSSH2_ERROR_EAGAIN ) waitsocket(sock, session);
    rc = libssh2_channel_exec(*channel, cmd);
    if(rc!=0)
    {
        fprintf(stderr,"Error\n");
        exit(1);
    }
    
    read_channel(session, *channel, sock);
//    while((rc = libssh2_channel_close(*channel)) == LIBSSH2_ERROR_EAGAIN) waitsocket(sock, session);
    rc = libssh2_channel_close(*channel);
    return rc;
}


int open_shell(LIBSSH2_SESSION *session, LIBSSH2_CHANNEL **channel, int sock)
{
    int rc;
    //while((*channel = libssh2_channel_open_session(session)) == NULL && libssh2_session_last_error(session,NULL,NULL,0) == LIBSSH2_ERROR_EAGAIN ) waitsocket(sock, session);
    *channel = libssh2_channel_open_session(session);
    if( *channel == NULL )
    {
        fprintf(stderr,"Error\n");
        exit(1);
    }
//    while((rc=libssh2_channel_request_pty(*channel, "vanilla")) == LIBSSH2_ERROR_EAGAIN ) waitsocket(sock, session);
    rc=libssh2_channel_request_pty(*channel, "vanilla");
    if( rc != 0 )
    {
        fprintf(stderr,"get pty Error %d\n", rc);
        exit(1);
    }
    
//    while((rc=libssh2_channel_shell(*channel)) == LIBSSH2_ERROR_EAGAIN ) waitsocket(sock, session);
    rc=libssh2_channel_shell(*channel);
    if( rc != 0 )
    {
        fprintf(stderr,"get shell Error %d\n", rc);
        exit(1);
    }
    return rc;
}

int write_channel(char* buf, LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel, int sock)
{
    int i, rc;
    for(i=0; i<strlen(buf); i++)
    {
//        while((rc = libssh2_channel_write(channel, buf+i, 1)) == LIBSSH2_ERROR_EAGAIN ) waitsocket(sock, session);
        rc = libssh2_channel_write(channel, buf+i, 1);
        if(rc<0)
        {
            fprintf(stderr,"write channel Error %d\n", rc);
            exit(1);
        }
    }
    return rc;
}

int exec_shell_script(char* fname, LIBSSH2_SESSION *session, LIBSSH2_CHANNEL **channel, int sock)
{
    int rc;
    if((rc=open_shell(session, channel, sock)) <0) return -1;
    
    FILE* cmdfile = fopen(fname, "r");
    if(cmdfile == NULL)
    {
        printf("open file %s errno: %s\n", fname, strerror(errno));
        exit(errno);
    }
    
    char buf[BUFSIZ];
    int i;
    
    
    do
    {
        fgets(buf, BUFSIZ, cmdfile);
        if(feof(cmdfile) == 1) break;
        write_channel(buf, session, *channel, sock);
    }while(!feof(cmdfile));
    
    strncpy(buf, "\nexit\n", 6);
    write_channel(buf, session, *channel, sock);
    read_channel(session, *channel, sock);
    fclose(cmdfile);
//    while((rc = libssh2_channel_close(*channel)) == LIBSSH2_ERROR_EAGAIN) waitsocket(sock, session);
    rc = libssh2_channel_close(*channel);
    return rc;
}

int conn(const char* ip, int port)
{
    struct sockaddr_in sin;
    sin.sin_addr.s_addr = inet_addr(ip);
    sin.sin_port = htons(SSH_PORT);
    sin.sin_family = AF_INET;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sock, (struct sockaddr*)(&sin), sizeof(struct sockaddr_in)) != 0)
    {
        fprintf(stderr, "connect to %s err (%s)\n", ip, strerror(errno));
        return -1;
    }
    return sock;
}

int logon(char* username, char* pass, LIBSSH2_SESSION **session, int sock)
{
    int rc;
    /* Create a session instance */
    if((*session = libssh2_session_init()) == NULL) return -1;
    //libssh2_session_set_blocking(*session, 0);
    libssh2_session_set_blocking(*session, 1);
    //while ((rc = libssh2_session_handshake(*session, sock)) == LIBSSH2_ERROR_EAGAIN);
    rc = libssh2_session_handshake(*session, sock);
    if(rc)
    {
        fprintf(stderr, "Failure establishing SSH session: %d %s\n", rc, strerror(errno));
        return -1;
    }
    
//    while ((rc = libssh2_userauth_password(*session, username, pass)) == LIBSSH2_ERROR_EAGAIN);
    rc = libssh2_userauth_password(*session, username, pass);
    if (rc) 
    {
        fprintf(stderr, "Authentication by password failed.\n");
        if(*session != NULL)
        {
            libssh2_session_disconnect(*session, "");
            libssh2_session_free(*session);
        }
        close(sock);
        libssh2_exit();
        return 0;
    }
}

int str_start(char* str, char* start)
{
    if(str == NULL || start == NULL) return 0;
    int len = strlen(start);
    if(len > strlen(str)) return 0;
    
    int i;
    for(i=0; i<len; i++)
    {
        if(str[i] != start[i]) return 0;
    }
    return 1;
}

void clear(LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel, int sock)
{
    int exitcode = 127, rc;
    char *exitsignal=(char *)"none";
    if( rc == 0 )
    {
        exitcode = libssh2_channel_get_exit_status( channel );
        libssh2_channel_get_exit_signal(channel, &exitsignal, NULL, NULL, NULL, NULL, NULL);
    }

    libssh2_channel_free(channel);
    channel = NULL;

    libssh2_session_disconnect(session, "");
    libssh2_session_free(session);
    close(sock);
    libssh2_exit();
}


int main(int argc, char *argv[])
{
    int rc;
    char ip_str[20]           = "127.0.0.1";
    char username[64]         = "root";
    char commandline[BUFSIZ]  = "uname";
    char pwdfile[BUFSIZ]      = "password.txt";
    char pass[BUFSIZ];
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel = NULL;
    
    if (argc > 1) strncpy(ip_str, argv[1], 19);
    if (argc > 2) strncpy(username,argv[2], 63);
    if (argc > 3) strncpy(commandline,argv[3], BUFSIZ -1);
    if (argc > 4) strncpy(pwdfile,argv[4], BUFSIZ -1);
    
    if ((rc=libssh2_init(0)) != 0)
    {
        fprintf (stderr, "init error (%d) (%s)\n", rc, strerror(errno));
        return 1;
    }

    decode_password(pass, pwdfile, ip_str, username);
    int sock = conn(ip_str, SSH_PORT);
    logon(username, pass, &session, sock);

    if(str_start(commandline, "file=") == 1) exec_shell_script(commandline+5, session, &channel, sock);
    else rc = exec_one_cmd(commandline, session, &channel, sock);
    
    shutdown:
    clear(session, channel, sock);
    
    return 0;
}
