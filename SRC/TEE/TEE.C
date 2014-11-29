static char sccsid[]="@(#)94	1.1  src/tee/tee.c, aixlike.src, aixlike3  9/27/95  15:46:18";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1992.       All rights reserved.*/
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 3      26Mar92                                           */
/* @1  06.24.93  grb  Do i/o character-by-char instead of line-by-line   */
/*-----------------------------------------------------------------------*/
/* tee - read from stdin, write to stdout and one or more other files */

/* The tee command reads standard input and writes the output of a
   program to standard output and copies it into the specified file or
   files at the same time.

   Flags

          -a Adds the output to the end of File instead of writing over it.

          -i Ignores interupts.  (This flag is accepted in the OS/2 version,
             but has no effect.)

   The flags have to be specified separately: -i and -a but not -ia.


Implementation notes:  this version handles up to 100 output files.

*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <string.h>
#include <fmf.h>            /* From unixlike\utils */

#define MAXOFILES 100       /* Handle up to 100 files on output */
#define CCHMAXPATH 262
#define APPEND    1
#define OVERWRITE 0
#define BUFFSIZE  1024      /* Handle lines up to 1024 characters */

char *append_mode = "ab";
char *overwrite_mode = "wb";

FILE *ofiles[MAXOFILES];    /* Here's where we keep the file handles */
char buff[BUFFSIZE];       /* Here's where we keep each input line */

/***************************************************************************/
/*   Function prototypes                                                   */
/***************************************************************************/
int isambiguous(char *filename);
void expandit(int mode, char *filename, int *indx);
void openit(int mode, char *filename, int *indx);
void tell_usage(void);


/***************************************************************************/
/*   MAIN                                                                  */
/***************************************************************************/
void main(int argc, char **argv)
{
   int i, fileindx, c;                                        //@1c
   char *p;
   int mode = OVERWRITE;
   FILE *ifile;

   for (fileindx = 0; fileindx < MAXOFILES; ofiles[fileindx++] = (FILE *)NULL);
   ofiles[0] = stdout;    /* always write to stdout */
   fileindx = 1;
   if (argc > 1)
     for (i = 1; i < argc; i++)
        {
           p = argv[i];
           if (*p == '-' || *p == '/')
             switch (*++p)
             {
                 case 'a':      mode = APPEND;
                                break;

                 case 'i':      break;

                 default:       tell_usage();
                                return;
                                break;
             }
           else
             {
                if (isambiguous(p))
                  if (mode == APPEND)
                    expandit(mode, p, &fileindx);
                  else
                    {
                       tell_usage();
                       return;
                    }
                else
                  openit(mode, p, &fileindx);
             }
         }
//    while (fgets(buff, BUFFSIZE, stdin))                      @1d
//      for (i = 0; i < fileindx; fputs(buff, ofiles[i++]));    @1d
    ifile = fdopen(0, "rb");                               //@1a
    while ((c = fgetc(ifile)) != EOF)                         //@1a
      {
      putchar(c);
      for (i = 1; i < fileindx; fputc(c, ofiles[i++]));       //@1a
      }
    if (fileindx > 1)
      for (i = 1; i < fileindx; fclose(ofiles[i++]));
}


/***************************************************************************/
/*   isambiguous                                                           */
/***************************************************************************/
int isambiguous(char *filename)
{
   if ((strchr(filename, '*') != 0) || (strchr(filename, '?') != 0))
     return(1);
   else
     return(0);
}


/***************************************************************************/
/*   openit                                                                */
/***************************************************************************/
void openit(int mode, char *filename, int *indx)
{
   FILE *f;
   char *modestr;

   if (mode == APPEND)
     modestr = append_mode;
   else
     modestr = overwrite_mode;

   if (*indx < MAXOFILES)
     {
       f = fopen(filename, modestr);
       if (f)
         {
            ofiles[*indx] = f;
            *indx = *indx + 1;
         }
       else
         fprintf(stderr,"tee: unable to open file %s for output\n", filename);
     }
   else
     fprintf(stderr,"tee: more than %d output files; %s not opened\n",
                                MAXOFILES,           filename);

}

/***************************************************************************/
/*   expandit()                                                            */
/***************************************************************************/
void expandit(int mode, char *filename, int *indx)
{
   int matchcnt = 0;
   char fnbuff[CCHMAXPATH];
   int attrib;

   if (fmf_init(filename, FMF_ALL_FILES, FMF_NO_SUBDIR) == 0)
     while (fmf_return_next(fnbuff, &attrib) == 0)
       {
          openit(mode, fnbuff, indx);
          matchcnt++;
       }
   if (matchcnt == 0)
     fprintf(stderr,"tee: no files matching %s found\n", filename);
}


/***************************************************************************/
/*   tell_usage()                                                          */
/***************************************************************************/
void tell_usage()
{
   printf("Usage:  tee [-a] [-i] File [File [File ...\n");
   printf("      Flags:\n");
   printf("             -a   Append output to file (default is overwrite)\n");
   printf("             -i   Ignore interupts (has no effect in OS/2)\n");
}
