#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "common.h"


#undef  DBG_ON
#undef  FILE_NAME
#define DBG_ON  		(0x01)
#define FILE_NAME 	"common:"


int em_isdir(const char * path)
{
	int ret = -1;
	if(NULL == path)return(FALSE);
	struct stat status;
	ret = lstat(path,&status);
	if(ret != 0)return(FALSE);
	return (S_ISDIR( status.st_mode ) && !S_ISLNK( status.st_mode ));
}


int em_read_number(char * filename, int * num)
{
	FILE * file = fopen( filename, "r" );
	if ( !file )
	{
		dbg_printf("fopen fail\n");
		return (-1);
	}
	if ( EOF == fscanf( file, "%d", num ))/*格式化输入*/
	{
		dbg_printf("fscanf fail\n");
		return (-2);
	}
	fclose( file );
	return 0;	
}








