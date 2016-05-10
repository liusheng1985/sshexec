#ifndef ENCODER_H
#define ENCODER_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


/*
EXAMPLE:
char pass[8] = "AG$DWU*F"; // password is 8-length string
char* data1 = (char*)malloc(256);

// test all char from 0 to 256
int i;
for(i=0; i<256; i++) data1[i] = i;

for(i=0; i<256; i++) printf("%d", data1[i]); printf("\n\n"); // 0...127 -128...-1
encode(data1, 256, pass);
for(i=0; i<256; i++) printf("%d", data1[i]); printf("\n\n"); // encoded
decode(data1, 256, pass);
for(i=0; i<256; i++) printf("%d", data1[i]); printf("\n\n"); // 0...127 -128...-1
 * */
int encode(char* data, int len, char* password);
int decode(char* data, int len, char* password);

#endif /* ENCODER_H */

