static char sccsid[]="@(#)50	1.1  src/head/head.c, aixlike.src, aixlike3  9/27/95  15:44:45";
/* The head command writes to standard output the first Count lines of
   each of the specified files, or of the standard input.  If Count is
   omitted, it defaults to 10.

   Syntax

          head [-Count] File [File [File...

   -Count specifies the number of lines to be displayed.
*/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 3       5/21/92                                          */
/* @A1 09.16.92  grb  Make return codes explicit.                        */
/*-----------------------------------------------------------------------*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "fmf.h"

#define BAILOUT -1
#define DEFAULT_NUMLINES 10
#define MAXPATHLEN 262
#define MAXLINE 1024

int numlines;
extern char *myerror_pgm_name;

/* function prototypes */

int init(int argc, char **argv);
void do_a_filespec(char *filespec);
void do_a_file(FILE *stream);
void do_stdin(void);
void tell_usage(void);

int main(int argc, char **argv)                                 /* @A1c */
{
   int i;

   myerror_pgm_name = "head";
   if ( (i = init(argc, argv)) != BAILOUT)
     if (i > 0)
       for (; i < argc; i++)
         do_a_filespec(argv[i]);
     else
         do_stdin();
   else                                                         /* @A1a */
     return(BAILOUT);                                           /* @A1a */
   return(0);                                                   /* @A1a */
}

int init(int argc, char **argv)
{
   int rval;
   char *p;

   numlines = DEFAULT_NUMLINES;
   if (argc < 2)
     rval = 0;
   else
     {
       p = argv[1];                /* first arg is either count or first file */
       if (*p == '-')                 /* It's a count */
         {
           if (isdigit(*++p))
             {
               numlines = atoi(p);
               if (numlines > 0)
                 rval = (argc > 2) ? 2 : 0;  /* Are there file specs, too? */
             }
           else
             {
               tell_usage();
               return(BAILOUT);
             }
         }
       else                          /* It's a file spec */
         rval = 1;
     }
   return(rval);
}

void do_a_filespec(char *filespec)
{
   char filename[MAXPATHLEN];
   int  attrib;
   FILE *f;

   if (fmf_init(filespec, FMF_ALL_FILES, FMF_NO_SUBDIR) == FMF_NO_ERROR)
     while (fmf_return_next(filename, &attrib) == FMF_NO_ERROR)
       {
          if (f = fopen(filename, "r"))
            do_a_file(f);
          fclose(f);
       }
}

void do_stdin()
{
   do_a_file(stdin);
}

void do_a_file(FILE *f)
{
   int i;
   char *p;
   char buff[MAXLINE + 1];

   for (i = 0; i < numlines; i++)
     {
        if (fgets(buff, MAXLINE, f))
          {
             for (p = buff; *p; p++);
             if (*--p != '\n')
               printf("%s\n", buff);
             else
               printf("%s", buff);
          }
        else
          return;
     }
}

void tell_usage()
{
  printf("\nhead - Copyright IBM, 1991, 1992\n");
  printf("The head command writes to standard output the first Count lines of\n");
  printf("each of the specified files, or of the standard input.  If Count is\n");
  printf("omitted, it defaults to 10.\n");

  printf("\nSyntax\n");

  printf("\n        head [-Count] File [File [File...\n");
}
