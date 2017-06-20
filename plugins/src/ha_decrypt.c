#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "ha_debug.h"

const unsigned int delta = 0x9e3779b9;
static unsigned int key[] = {0x20160903,0xFAFBFCFD,0x20160708,0xEAEBECED};

/*这是tea解密算法*/
static void tea_decrypt(unsigned int *v, unsigned int *k)
{
    unsigned int y = v[0], z = v[1], sum = 0, i;
    sum = (delta<<5);
    unsigned int a = k[0], b = k[1], c = k[2], d = k[3];
    for(i = 0; i < 32; i++)
    {
        z -= ((y<<4) + c)^(y + sum)^((y>>5) + d);
        y -= ((z<<4) + a)^(z + sum)^((z>>5) + b);
        sum -= delta;
    }
    v[0] = y;
    v[1] = z;
}
/*字节转换成unsigned int*/
static unsigned int bytes2ui(unsigned char *b)
{
    unsigned int tmp = 0;
    tmp |= b[0]<<24;
    tmp |= b[1]<<16;
    tmp |= b[2]<<8;
    tmp |= b[3];
    return tmp;
}
/*unsigned int 转换成bytes*/
static void ui2bytes(unsigned int out,unsigned char* ctmp)
{
    unsigned int tmp = out>>24;
    ctmp[0] = tmp&0xFF;
    tmp = out>>16;
    ctmp[1] = tmp&0xFF;
    tmp = out>>8;
    ctmp[2] = tmp&0xFF;
    tmp = out;
    ctmp[3] = tmp&0xFF;
}

/*对数据进行解码*/
char *ha_decrypt(const char *buf,int cfg_size)
{
    unsigned char ctmp[5] = {0};
    unsigned int in[2] = {0};
    char *json_str = (char*)CALLOC(1,cfg_size+1);

	if(!json_str)
        return NULL;

    for(int i = 0; i < cfg_size;)
    {
        for(int m = 0; m < 2; m++)
        {
            for(int j = 0; j < 4; j++)
            {
                ctmp[j] = buf[i++];
            }
            in[m] = bytes2ui(ctmp);
        }
        tea_decrypt(in, key);
        for(int i = 0; i < 2; i++)
        {
            ui2bytes(in[i], ctmp);
            strcat(json_str, (char *)ctmp);
        }
    }
	
    return json_str;
}
