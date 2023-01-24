#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#define ERROR 1
#define WARN  2
#define INFO 3
#define DEBUG 4
#define TRACE 5

#ifndef MYLOG_PRIORITY
#define MYLOG_PRIORITY 1
#endif 

#define MYLOG_FILE stderr

#define L(priority,format,a...) if(priority <= MYLOG_PRIORITY) fprintf(MYLOG_FILE, format "\n", ## a);

#endif 
