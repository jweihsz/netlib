#ifndef _MD5_H	
#define _MD5_H

#define MD5_DIGEST_LENGTH (16)
#define MD5_STR_LENGTH		(2 * MD5_DIGEST_LENGTH)

void md5sum_mem(const void *buf,unsigned int len,unsigned char digest[MD5_DIGEST_LENGTH]);
int md5sum_file(const char *file,unsigned char digest[MD5_DIGEST_LENGTH]);

#endif

