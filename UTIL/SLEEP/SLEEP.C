static char sccsid[]="@(#)43	1.1  util/sleep/sleep.c, aixlike.src, aixlike3  9/27/95  15:53:15";
/* sleep(n) - suspends calling thread for n seconds */

#define INCL_DOSPROCESS
#include <os2.h>

/* Temporary mapping */

int sleep(unsigned int seconds)
{
   DosSleep((unsigned long)seconds * 1000);
   return(0);
}
