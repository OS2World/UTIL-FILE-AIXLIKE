static char sccsid[]="@(#)35	1.1  util/fmf/wcmatch.c, aixlike.src, aixlike3  9/27/95  15:53:01";
/* match two strings: both may be ambiguous in the DOS (OS/2) sense: */
/* using a question mark for a place holder, and a * to mean "up to */
/* the next dot" */

/* returns 1 for match, 0 for not */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define NEXTDOT(p) for(; *p && (*p != '.'); p++)
#define MATCH   1
#define NOMATCH 0

wcmatch(char *string1, char *string2)
{
   char *p, *q;
   char *allfs = "*.*";

   if (!strcmp(string1, allfs) || !strcmp(string2, allfs))
     return(MATCH);
   if (!strcmp(string1, "*")   || !strcmp(string2, "*"))
     return(MATCH);

   for (p = string1, q = string2;
        *p && *q;
        p++, q++)
     {
       if ((*p == '?') || (*q == '?'))
         continue;
       else
         if ((*p == '*') || (*q == '*'))
           {
             NEXTDOT(p); NEXTDOT(q);
             if (!*p && !*q)
               return(MATCH);
             else
               if (!*p || !*q)
                 return(NOMATCH);
           }
         else
           if (toupper(*p) != toupper(*q))
             return(NOMATCH);
     }
     if (toupper(*p) == toupper(*q))
       return(MATCH);
     else
       if (*p == '*')
         NEXTDOT(p);
       else
         if (*q == '*')
           NEXTDOT(q);
       if (toupper(*p) == toupper(*q))
         return(MATCH);
     return(NOMATCH);
}

#ifdef DEBUG
main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
{
   if (argc < 3)
     printf("\nInvoke as wcmatch string1 string2\n");
   else
     if (wcmatch(argv[1], argv[2]))
       printf("\n%s  matches  %s\n", argv[1], argv[2]);
     else
       printf("\n%s  does NOT match  %s\n", argv[1], argv[2]);
   return(0);
}
#endif
