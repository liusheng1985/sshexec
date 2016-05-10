#include "encoder.h"

int _is_in_array(char n, char* arr, int len)
{
    if(arr == NULL || len == 0) return -1;
    int i;
    for(i=0; i<len; i++)
    {
        if(*(arr+i)==n) return 1;
    }
    return -1;
}

int _make_encode_array(char* arr, char* password)
{
    if(password==NULL || arr==NULL) return -1;
    if(strlen(password) < 8) return -2;
    int i, idx=0;
    char h, l;
    for(i=0; i<16; i++) arr[i]=-1;
    for(i=0; i<8; i++)
    {
        h = *(password+i) >> 4;
        l = *(password+i) & 0x0f;
        while(_is_in_array(h, arr, 16)>0) h = (h+1)%16;
        *(arr+idx) = h;
        while(_is_in_array(l, arr, 16)>0) l = (l+1)%16;
        *(arr+idx+1) = l;
        idx+=2;
    }
    return 0;
}

int _make_decode_array(char* decode, char* password)
{
    char encode[16];
    int re = _make_encode_array(encode, password);
    if(re < 0) return re;
    
    int i;
    for(i=0; i<16; i++)
    {
        decode[encode[i]] = i;
    }
    return 0;
}

void _do_code(char* data, int len, char* code)
{
    int i;
    for(i=0; i<len; i++) data[i] = (code[data[i] >> 4 & 0xf] << 4) + code[data[i] & 0xf];
}

int encode(char* data, int len, char* password)
{
    char code[16];
    int re = _make_encode_array(code, password);
    if(re < 0) return re;
    _do_code(data, len, code);
    return 0;
}

int decode(char* data, int len, char* password)
{
    char code[16];
    int re = _make_decode_array(code, password);
    if(re < 0) return re;
    _do_code(data, len, code);
    return 0;
}

/* test

*/


