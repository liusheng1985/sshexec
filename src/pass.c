/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   pass.c
 * Author: liusheng
 *
 * Created on 2016年5月6日, 下午1:11
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "encoder.h"

char* buf;
int map_size;
int pgsize;
struct stat st;

char* map_file(char* fname)
{
    pgsize = sysconf(_SC_PAGESIZE);
    int fd = open(fname, O_RDWR|O_EXCL);
    if(fd < 0)
    {
        printf("%s\n", strerror(errno));
        exit(1);
    }
    if ((stat(fname, &st)) < 0)
    {
        printf("%s\n", strerror(errno));
        exit(1);
    }
    
    map_size = (st.st_size/pgsize+1)*pgsize;
    char* buf = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if(buf == MAP_FAILED)
    {
        printf("mmap: %s\n", strerror(errno));
        exit(1);
    }
    printf("open file %s\n", fname);
    return buf;
}

int getpassword(char* pass, int len, char* prompt)
{
    struct termios oldt, newt;
    printf("%s", prompt);
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

/*
 * 
 */
int main(int argc, char** argv) {
    char* fname = "password.txt";
    char code[BUFSIZ];
    buf = map_file(argc > 1 ? argv[1] : fname);
    

    
    off_t i;
    int j, count;
    getpassword(code, 8, "input encrypt code(8 char): ");
    for(i=0; i<st.st_size; i++)
    {
        if(buf[i] == '\n' || buf[i] == '\r' || buf[i]==EOF)
        {
            if(buf[i-1] == '\n' || buf[i-1] == '\r') continue;
            for(j=i-1; buf[j-1]!=' ' && buf[j-1]!='\t'; j--);
            encode(buf+j, i-j, code);
            count ++;
            continue;
        }
    }
    printf("encoded %d passwords\n", count);
    munmap(buf, map_size);
    return (EXIT_SUCCESS);
}

