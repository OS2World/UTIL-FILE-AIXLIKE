static char sccsid[]="@(#)41	1.1  util/makepath/makepath.c, aixlike.src, aixlike3  9/27/95  15:53:12";
/* This subroutine makepath() creates the path passed to it as its parameter.
   It starts at the left side of the path.  When it finds a componant that
   does not exist, it creates it.

   Slashes or backslashes are supported in the input path name.

   MSC6.0 interfaces are used.  In particular

       _chdrive() to change the current default drive.
       chdir()    to change the current working directory
       mkdir()    to create a directory
*/

#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <errno.h>

#define BACKSLASH '\\'
#define SLASH     '/'
#define ISSEPARATER(x)  ((x==SLASH) || (x==BACKSLASH))

int makepath(char *mpath)
{
   char *p, c;
   int  rc;

   p = mpath;
   while (*p)                         /* find each componant of path */
     {
       if (ISSEPARATER(*p))
         p++;
       for (; *p && !ISSEPARATER(*p); p++);
       c = *p;
       *p = '\0';                     /* null terminate the componant */
       rc = mkdir(mpath);
       *p = c;
     }
   if (access(mpath, 0))
     return(ENOENT);
   else
     return(0);
}
