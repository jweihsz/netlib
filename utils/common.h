#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef TRUE
#undef FALSE
#define	TRUE	(1u)
#define	FALSE	(0u)

#define DBG_ON  		(0x01)
#define FILE_NAME 	"common:"


#define dbg_printf(fmt,arg...)		do{if(DBG_ON)fprintf(stderr,FILE_NAME"%s(line=%d)->"fmt,__FUNCTION__,__LINE__,##arg);}while(0)


#endif /*_COMMON_H*/