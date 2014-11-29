static char sccsid[]="@(#)92	1.1  src/tail/tail.c, aixlike.src, aixlike3  9/27/95  15:46:14";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1990, 1991. All rights reserved.*/
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 2      26Nov91                                           */
/* A1  09.16.92  grb  Make return code explicit                          */
/*-----------------------------------------------------------------------*/
/* Writes a file to standard output, beginning at a specified point.

   Syntax

          head [+/-Number[suffix] [-f] [-r] File [File [File...

   where

          +/-Number indicates the starting point.  If Number is introduced
                    by +, then the starting point is Number units from the
                    beginning of the file.  If Number is introduced by -
                    then the starting point is Number units from the end
                    of the file.  Default is -10

          suffix    is one of l, b, c or k.  If l (the default), 'units' is
                    lines.  If b, then 'units' is 512-byte blocks.  If k,
                    then 'units' is 1024-byte blocks.  If c, then 'units'
                    is characters.

          -f        Specifies that tail not end after copying the input
                    file, but rather check at 1-second intervals for more
                    input.  Thus, it can be used to monitor the growth of a
                    file being written by another process.

          -r        Displays lines from the end of the file in reverse
                    order.  The default for the -r option is print
                    the entire file in reverse order.

   Description

       The tail command writes the named File (standard input by default)
       to standard output, beginning at the point you specify.  It begins
       reading at +Number lines from the beginning of File or -Number lines
       from the end of File.  The default Number is 10.  Number is counted
       in units of lines, block, or characters, according to the subflag
       appearing after Number.  The block size is either 512 bytes or
       1k bytes.
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <share.h>
#include <io.h>
#include "fmf.h"

#define YES      1
#define NO       0
#define BAILOUT -1
#define CR       0x0d
#define LF       0x0a
#define CTL_Z    0x1a
#define DEFAULT_NUMLINES 10
#define FROM_TOP          0
#define FROM_BOTTOM       1
#define FROM_TOP_NORMAL   0
#define FROM_TOP_POLISH   1
#define FROM_BOTTOM_NORMAL 2
#define FROM_BOTTOM_POLISH 3
#define DEFAULT_DIRECTION FROM_BOTTOM
#define LINEUNIT          1
#define B1UNIT            2
#define B2UNIT            3
#define CHARUNIT          4
#define DEFAULT_SUBFLAG   LINEUNIT
#define MAXPATHLEN 262
#define MAXLINE 1024
#define ONEK    1024
#define HALFK    512
#define BUFFSIZE (50000)
#define BUFFINC  (10000)
#define ERRVAL (off_t)-1

char buf[BUFFSIZE + 1];

int numlines;  /* Distance from file extremus where we are to start */
int direction; /* Extremus: start or end */
int subflag;   /* Units in which distance is measured */
int wait_at_end;  /* YES if program is to wait for more output at eof */
int polish;       /* YES if lines are displayed in reverse order */
char *eob;        /* end of buffer */
extern char *myerror_pgm_name;
char caseflag;

/* function prototypes */

int init(int argc, char **argv);
void do_a_filespec(char *filespec);
void do_a_file(FILE *stream);
void do_stdin(void);
void tell_usage(void);
char *findstart(FILE *stream);
off_t unitconv(int subflag, int numlines);
void printfrom(char *start, FILE *stream);
void putline(char *start);
void lineout(char *q);
char *backoneline(char *p, char *buf);
char *forwardoneline(char *p, char *buf);
void readbottom(FILE *f);
void readtop(FILE *f);
extern void myerror(int, char *, char *);
void wait_for_more(FILE *stream);
extern int sleep(int);

/* ----------------------------------------------------------------------- */
/*   main                                                                  */
/* ----------------------------------------------------------------------- */
int main(int argc, char **argv)                                    /* @A1c */
{
   int i;
   int rc;                                                         /* @A1a */

   rc = 0;                                                         /* @A1a */
   myerror_pgm_name = "tail";
   if ( (i = init(argc, argv)) > 0)
       for (; i < argc; i++)
         do_a_filespec(argv[i]);
   else
     if (i != BAILOUT)
       do_stdin();
     else                                                          /* @A1a */
       rc = BAILOUT;                                               /* @A1a */
   return(rc);                                                     /* @A1a */
}

/* ----------------------------------------------------------------------- */
/*   init                                                                  */
/* ----------------------------------------------------------------------- */
int init(int argc, char **argv)
{
   int i, rval;
   char *p, df;

   numlines = -1;
   direction = DEFAULT_DIRECTION;
   subflag = DEFAULT_SUBFLAG;
   wait_at_end = NO;
   polish = NO;

   if (argc < 2)
     rval = 0;
   else
     {
       for (i = 1; i < argc && (*argv[i] == '-' || *argv[i] == '+'); i++)
         {
           p = argv[i];
           df = *p;
           if (isdigit(*++p))      /* If this specifies the count */
             {
                for (numlines = 0; isdigit(*p); p++)
                  numlines = numlines * 10 + *p - '0';
                switch (*p)
                {
                  case  0 :       break;
                  case 'l':       subflag = LINEUNIT;
                                  break;
                  case 'b':       subflag = B1UNIT;
                                  break;
                  case 'k':       subflag = B2UNIT;
                                  break;
                  case 'c':       subflag = CHARUNIT;
                                  break;
                  default:
                                  tell_usage();
                                  return(BAILOUT);
                                  break;
                }
                if (df == '+')
                  direction = FROM_TOP;
                else
                  direction = FROM_BOTTOM;
             }
           else
             switch (*p)
             {
               case 'f':       if (polish == YES)
                                 {
                                   tell_usage();
                                   return(BAILOUT);
                                 }
                               else
                                 wait_at_end = YES;
                               break;

               case 'r':       if (wait_at_end == YES)
                                 {
                                   tell_usage();
                                    return(BAILOUT);
                                 }
                               else
                                 polish = YES;
                               break;

               default: tell_usage();
                               return(BAILOUT);
                               break;
             }
         }
       rval = (i == argc) ? 0 : i;
     }

   if ( (numlines == -1) && (polish == NO) )
     numlines = DEFAULT_NUMLINES;

   if (polish == YES && numlines == -1)
     {
       direction = FROM_BOTTOM;
       numlines = 0;
     }

   caseflag = 0;                 /* create a flag that says the direction */
   if (direction == FROM_BOTTOM) /* from which the starting point will be */
     caseflag = 2;               /* found, and whether we are reading     */
   caseflag |= polish;           /* in reverse (polish)                   */
   return(rval);
}

/* ----------------------------------------------------------------------- */
/*   do_a_filespec                                                         */
/* ----------------------------------------------------------------------- */
void do_a_filespec(char *filespec)
{
   char filename[MAXPATHLEN];
   int  attrib;
   FILE *f;
   int fh;

   if (fmf_init(filespec, FMF_ALL_FILES, FMF_NO_SUBDIR) == FMF_NO_ERROR)
     while (fmf_return_next(filename, &attrib) == FMF_NO_ERROR)
       {
          fh = sopen(filename, O_RDONLY|O_BINARY, SH_DENYNO, S_IWRITE);
          if (f = fdopen(fh, "rb"))
            do_a_file(f);
          fclose(f);
       }
}

/* ----------------------------------------------------------------------- */
/*   do_stdin                                                              */
/* ----------------------------------------------------------------------- */
void do_stdin()
{
   do_a_file(stdin);
}

/* ----------------------------------------------------------------------- */
/*   do_a_file                                                             */
/* ----------------------------------------------------------------------- */
void do_a_file(FILE *f)
{
   char *start;

   if ( (start =  findstart(f)) != (char *)ERRVAL) /* find the starting point */
     printfrom(start, f);               /* print from there to end */

}

/* ----------------------------------------------------------------------- */
/*   findstart                                                             */
/* ----------------------------------------------------------------------- */
char *findstart(FILE *f)
{
   off_t  units;
   char *start;
   int i;
                         /* In two cases, we can go directly to EOF: */
                         /* If we are reading FROM_BOTTOM, then we   */
                         /* must fill up the buffer as full as we can*/
                         /* with data right up to EOF                */
   switch (caseflag)
   {
      case FROM_BOTTOM_NORMAL:
      case FROM_BOTTOM_POLISH:   readbottom(f);
                                 if (subflag == LINEUNIT)
                                   {
                                     start = eob;
                                     for (i = 0; start && i < numlines; i++)
                                        start = backoneline(start, buf);
                                     if (start == NULL)
                                       start = buf;
                                   }
                                 else
                                   {
                                     units = unitconv(subflag, numlines);
                                     if (units > (eob - buf))
                                       start = buf;
                                     else
                                       start = eob - units;
                                   }
                                 break;
                         /* In the most common case, we can go       */
                         /* directly to the starting point.  For     */
                         /* this option, we will be reading line-by- */
                         /* line, so we don't even fill the buffer.  */
     case FROM_TOP_NORMAL:       if (subflag == LINEUNIT)
                                   for (i = 0; i < numlines; i++)
                                      fgets(buf, MAXLINE, f);
                                 else
                                   fseek(f, (long)unitconv(subflag, numlines),
                                         SEEK_SET);
                                 break;
                         /* In the last case, we read as much        */
                         /* as we can, starting from start-of-file.  */
     case FROM_TOP_POLISH:       readtop(f);
                                 if (subflag == LINEUNIT)
                                   {
                                     for (start=buf, i = 0; i < numlines; i++)
                                       {
                                         start = forwardoneline(start, eob);
                                         if (start == NULL)
                                           {
                                             start = eob;
                                             break;
                                           }
                                       }
                                   }
                                 else
                                   {
                                     units = unitconv(subflag, numlines);
                                     if (units > (eob - buf))
                                       start = eob;
                                     else
                                       start = buf + units;
                                   }
   }
   return(start);
}

/* ----------------------------------------------------------------------- */
/*   unitconv                                                              */
/* ----------------------------------------------------------------------- */
off_t unitconv(int subflag, int numlines)
{
   switch (subflag)
   {
      case CHARUNIT:               return((off_t)numlines);
                                   break;
      case B1UNIT:                 return((off_t)numlines * 512);
                                   break;
      case B2UNIT:                 return((off_t)numlines * 1024);
                                   break;
      default:                     return((off_t)0);
                                   break;
   }
}

/* ----------------------------------------------------------------------- */
/*   forwardoneline                                                        */
/* ----------------------------------------------------------------------- */
char *forwardoneline(char *start, char *eob)
{
   char *p,c;

   for (p = start; p!=eob && *p && *p-CR && *p-LF; p++);
   if ((p-start) > MAXLINE)
     p = start + MAXLINE;
   else
     if (p == eob)
       p = NULL;
     else
       {
         c = (char)((*p == CR) ? LF : CR);
         if (++p != eob)
           if (*p == c && p != eob)
             p++;
         if (p == eob)
           p = NULL;
       }
   return(p);
}

/* ----------------------------------------------------------------------- */
/*   backoneline     Given a starting point p, move backwards to the start */
/*                   of the previous line (that is, skip over CR/LF then   */
/*                   go back to the character after the previous CR/LF.    */
/*                   Stop if   you reach the beginning of the buffer.      */
/* ----------------------------------------------------------------------- */
char *backoneline(char *start, char *buftop)
{
   char *p;
   char c;
   int  len;

   if (start == buftop)
     return(NULL);
                         /* find the CR or LF that ends the previous line */
   p = start - 1;
   if (*p == CR || *p == LF)
     if (p != buftop)
       {
          c = (char)((*p == CR) ? LF : CR);
          p--;
          if (p != buftop)
            if (*p == c)
              p--;
       }
   if (p != buftop)
     {
       for (; p != buftop && *p != CR && *p != LF; p--);
       if (*p == CR || *p == LF)
         p++;
       len = start - p;
       if (len > MAXLINE)
         {
            len = len % MAXLINE;
            if (len == 0)
              len = MAXLINE;
            p = start - len;
         }
     }

   return(p);
}

/* ----------------------------------------------------------------------- */
/*    readtop()                                                            */
/* ----------------------------------------------------------------------- */
void readtop(FILE *f)
{
   size_t bytesread;

   bytesread = fread(buf, sizeof(char), (size_t)BUFFSIZE, f);
   eob = buf + bytesread;
//   if (*eob == CTL_Z)
//     eob--;
}

/* ----------------------------------------------------------------------- */
/*    readbottom()                                                         */
/* ----------------------------------------------------------------------- */
void readbottom(FILE *f)
{
   size_t bytesread;

   bytesread = fread(buf, sizeof(char), (size_t)BUFFSIZE, f);
   if (bytesread)
     if (feof(f))
       eob = buf + bytesread;
     else
       {
         while (!feof(f))
           {
             memcpy(buf, buf+BUFFINC, (size_t)BUFFSIZE-BUFFINC);
              bytesread = fread(buf+(BUFFSIZE-BUFFINC), sizeof(char),
                                BUFFINC, f);
           }
         eob = buf + (BUFFSIZE-BUFFINC) + bytesread;
       }
   else
     eob = buf;
//   if (eob != buf)
//     if (*eob == CTL_Z)
//       eob--;
}


/* ----------------------------------------------------------------------- */
/*   printfrom                                                             */
/* ----------------------------------------------------------------------- */
void printfrom(char *start, FILE *f)
{

   switch(caseflag)
   {
      case FROM_TOP_NORMAL:     start = buf;
                                while (fgets(buf, MAXLINE, f))
                                  putline(start);
                                if (wait_at_end == YES)
                                  wait_for_more(f); /*(never returns from here*/
                                break;
      case FROM_BOTTOM_NORMAL:  while (start)
                                  {
                                    putline(start);
                                    start = forwardoneline(start, eob);
                                  }
                                if (wait_at_end == YES)
                                  wait_for_more(f); /*(never returns from here*/
                                break;
      case FROM_TOP_POLISH:
      case FROM_BOTTOM_POLISH:  while (start)
                                  {
                                    putline(start);
                                    start = backoneline(start, buf);
                                  }
                                break;
   }
}

/* ----------------------------------------------------------------------- */
/*   putline                                                               */
/* ----------------------------------------------------------------------- */
void putline(char *start)
{
   char c, *p;
   int i;

   for (i = 0, p = start; *p && *p-CR && *p-LF && i < MAXLINE && p != eob; p++);
   c = *p;
   *p = 0;
   printf("%s\n", start);
   *p = c;
}

/* ----------------------------------------------------------------------- */
/*   wait_for_more                                                         */
/*                    wait forever for more data                           */
/* ----------------------------------------------------------------------- */
void wait_for_more(FILE *f)
{

   while (1)         /* Do Forever */
     {
        sleep(1);
        while (fgets(buf, MAXLINE, f))
          putline(buf);
     }
}             /* program can never reach this point */

/* ----------------------------------------------------------------------- */
/*   tell_usage                                                            */
/* ----------------------------------------------------------------------- */
void tell_usage()
{

   printf("tail - Copyright IBM, 1991\n");
   printf("Usage\n");

   printf("       tail [+/-Number[suffix]] [-f] [-r] File [File [File...\n");

   printf("where\n");

   printf("\n       +/-Number indicates the starting point.  If Number is introduced\n");
   printf("                 by +, then the starting point is Number units from the\n");
   printf("                 beginning of the file.  If Number is introduced by -\n");
   printf("                 then the starting point is Number units from the end\n");
   printf("                 of the file.  Default is -10\n");

   printf("\n       suffix    is one of l, b, c or k.  If l (the default), 'units' is\n");
   printf("                 lines.  If b, then 'units' is 512-byte blocks.  If k,\n");
   printf("                 then 'units' is 1024-byte blocks.  If c, then 'units'\n");
   printf("                 is characters.\n");

   printf("\n       -f        Specifies that tail not end after copying the input\n");
   printf("                 file, but rather check at 1-second intervals for more\n");
   printf("                 input.  Thus, it can be used to monitor the growth of a\n");
   printf("                 file being written by another process.\n");

   printf("\n       -r        Displays lines from the end of the file in reverse\n");
   printf("                 order.  The default for the -r option is print\n");
   printf("                 the entire file in reverse order.\n");
}
