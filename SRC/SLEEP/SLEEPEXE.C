static char sccsid[]="@(#)85	1.1  src/sleep/sleepexe.c, aixlike.src, aixlike3  9/27/95  15:45:58";
/* UNIXLIKE sleep command.
   Sleeps for n seconds
   Usage:
           sleep n
   where n is the number of seconds

   Uses the external subroutine sleep() from UNIXLIKE\UTIL
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <stdlib.h>

void tell_usage(void);
extern int sleep(int n);

int main(int argc, char **argv)
{
   int n;

   if  (argc < 2)
     tell_usage();
   else
     {
        n = atoi(argv[1]);
        if (n <= 0)
          tell_usage();
        else
          sleep(n);
     }
   return(0);
}

void tell_usage()
{
   printf("\nsleep - Copyright IBM, 1991\n");
   printf("Usage:\n    sleep n\nwhere n is the number of seconds to sleep\n");
}
