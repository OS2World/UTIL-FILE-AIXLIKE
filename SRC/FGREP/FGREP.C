static char sccsid[]="@(#)43	1.1  src/fgrep/fgrep.c, aixlike.src, aixlike3  9/27/95  15:44:29";
/************************************************/                         
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1991.  All rights reserved.     */
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/* @1 05.08.91 changed fmf_init to find all files, even hidden           */
/* @2 10.19.91 upper casing a line of over 256 bytes caused Trap D       */
/* @3 05.22.92 failed on eof condition (trap d) reported by B. Kwan      */
/* @4 05.03.93 modified for IBM C/Set2 compiler                          */
/* @5 08.31.94 Trouble with 50000 byte boundries                         */
/*-----------------------------------------------------------------------*/

/* fgrep: searches a file for a pattern.

   Usage:
          fgrep [-v][-x][-h][-s][-i] [-c | -l] [-b | -n]
                [Pattern | -e Pattern | -f StringFile] File [File...
*/

/* The fgrep command searches the input files specified by the *File* parameter
(standard input by default) for lines matching a pattern.  The fgrep command
searches specifically for *Pattern* parameters that are fixed strings.  The
fgrep command displays the file containing the matched line if you specify more
than one file in the *File* parameter.

The exit values of this command are:
  0   A match was found
  1   No match was found
  2   A syntax error was found or a file was inaccessible (even if matches were
      found).

Flags:
      -b            Precedes each line by the block number on which it was
                    found. Use this flag to help find disk block numbers
                    by context. (In this implementation the relative allocation
                    unit number is shown);

      -c            Displays only a count of matching lines.

      -e Pattern    Specifies a pattern.  This works the same as a simple
                    pattern, but is useful when a pattern begins with a -.

      -f StringFile Specifies a file that contains strings to be matched.
                    (The file may specify up to 256 strings in this
                    implementation).

      -h            Suppresses file names when multiple files are being
                    processed.

      -i            Ignores the case of lettters when making comparisons.

      -l            Lists just the names of files (once) with matching
                    lines.  Each file name is separated by a new-line
                    character.

      -n            Precedes each line with its relative line number in the
                    file.

{      -s            Displays only error messages.  This is useful for checking
                    status. } (superceded in the OS/2 version).

      -s            Search all subdirectories below the specified path.

      -v            Displays all lines except those that match the specified
                    pattern.

      -x            Displays lines that match the pattern exactly with no
                    additional characters.

      -y            Ignore case of letters when making comparisons
*/

/* -------------------------------------------------------------------------- */
/*  Maintenance history:                                                      */
/*           Jan 15 1991: Fixed case-insensitive search                       */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* -------------------------------------------------------------------------- */

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>
#include "fmf.h"

// #define BUFSIZE    30000
#define BUFSIZE    50000
#define MAXPATTERNS  256
#define MAXLINE 255
#define MATCHFOUND     0
#define NOMATCHFOUND   1
#define SOMEERROR      2
#define YES            1
#define NO             0
#define BAILOUT       -1
#define INIT           0
#define NEXT           1
#define TAB           0x09
#define CR            0x0d
#define LF            0x0a
#define STDIN          0
#define NONPOLISH      0
#define REVERSE        1
#define BUFEND         (buf + BUFSIZE)


char *optstring = "bce:f:hilnsvxy";      /* valid option letters */
char *stdinC = "STDIN";
char buf[BUFSIZE + 1];                   /* input buffer */
char *pattern[MAXPATTERNS] = {NULL};     /* up to MAXPATTERNS pattern ptrs */

                   /*----------------------------*/
                   /* Options and their defaults */
                   /*----------------------------*/
char *filespec = NULL;          /* file(s) to be searched */
char *StringFile = NULL;        /* name of file containing patterns */
int  logic = NONPOLISH;         /* search for matching or non-matching files? */
int  exact = NO;                /* line must exactly match the pattern? */
int  showname = YES;            /* show name of file */
int  subtreesrch = NO;          /* search subtrees for matching files ? */
int  casesensitive = YES;       /* respect case in comparing to pattern? */
int  nameonly = NO;             /* show only names of files having pattern? */
int  countonly = NO;            /* show only a count of files having pattern? */
int  prtlines = YES;            /* print each line that matches pattern? */
int  showlinenum = NO;          /* show line number with matching line? */
int  showblocknum = NO;         /* show block number containing the line? */
ULONG bytes_per_allocunit;       /* used to calculate block number */
ULONG bytes_read_so_far;         /* keeps track of bytes read so far */
int  count = 0;                 /* counter of files containing pattern */
int  matchesinfile = 0;         /* number of matches in the current file */

                   /*----------------------------*/
                   /*    Function Prototypes     */
                   /*----------------------------*/
int init(int argc, char *argv[]);   /* Initialize from command line data */
int getpatterns(char *filename);    /* get patterns from string file */
void do_a_file(char *filename);     /* Logic for converting file to stream */
void do_STDIN(void);                /* Logic when input file is stdin      */
                                    /* Logic for matching lines in one file */
void do_the_stream(FILE *stream, char *filename);
                                    /* Fill the program's data buffer */
size_t getbuff(FILE *stream, int bufnum, char *filename, char **putbufend);
                                    /* Isolate a line withing the buffer */
char *getline(int action, FILE *stream, char *bufend);
                                    /* Print out a matching line */
void showline(char *line, char *filename, int lineno, long bytesread);
void showfname(char *filename);     /* Print matching file name only */
                                    /* Print count of matching lines */
void showcount(char *filename, long matchcount);
void tell_usage(void);              /* Explain program usage */
                                    /* Parse command line */
int  getopt(int argc, char *argv[], char *opstring);

extern int optind;                  /* data exported by getopt() */
extern char *optarg;
                                    /* put out errors in a standard format */
void myerror(int rc, char *area, char *details);
extern char *myerror_pgm_name;      /* data imported by myerror */



int  rtrnstatus = NOMATCHFOUND;     /* value returned on exit from fgrep */

/*----------------------------------------------------------------------------*/
/*  main                                                                      */
/*                                                                            */
/*  mainline logic:                                                           */
/*      Perform initialization                                                */
/*      For each filespec on the command line                                 */
/*         Initialize a search for files that match the spec                  */
/*         For every file that does match the spec                            */
/*            Try to match the pattern                                        */
/*         Close the search for matching files to prepare for the next one.   */
/*                                                                            */
/*  None of this holds true if no filespec was specified.  In that case we    */
/*  take input from STDIN and try to match that.                              */
/*----------------------------------------------------------------------------*/
main(argc, argv, envp)
int argc;
char *argv[];
char *envp[];
{
   char filename[CCHMAXPATH];
   int  attrib, i, filespecindx, rc;

   myerror_pgm_name = "fgrep";
   if ( (filespecindx = init(argc, argv)) == BAILOUT)
     return(SOMEERROR);
   else
     if (filespecindx > 0)
       {
         for (i = filespecindx; i < argc; i++)
           {
//            if ( (rc = fmf_init(argv[i], 0, subtreesrch)) == NO_ERROR)     @1
            if ( (rc = fmf_init(argv[i], FMF_ALL_FILES, subtreesrch))
                                                          == FMF_NO_ERROR) //@1
              {
                while (fmf_return_next(filename, &attrib) == NO_ERROR)
                  do_a_file(filename);
                fmf_close();
              }
            else
              myerror(rc, "Finding filespec", argv[i]);
           }
       }
     else
       do_STDIN();
   return(rtrnstatus);
}

/*----------------------------------------------------------------------------*/
/*  init                                                                      */
/*                                                                            */
/*  Do initialization:                                                        */
/*      Examine command line options and set internal variables accordingly.  */
/*      If a pattern wasn't specified with -f, take if from the next command  */
/*       line position; complain if there isn't one.                          */
/*      If there are remaining command line arguments, they are file specs.   */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*----------------------------------------------------------------------------*/
int init(int argc, char *argv[])
{
    int rtrnindx = 0;
    int args, i;
#ifdef I16
    char c;
#else
    int c;
#endif
    char *p;
    int cu;

    for (i = 0; i < MAXPATTERNS; pattern[i++] = NULL); /* initialize patterns */
#ifdef I16
    while ( (c = (char)getopt(argc, argv, optstring)) != EOF)
#else
    while ( (c = getopt(argc, argv, optstring)) != EOF)
#endif
    {
        cu = toupper(c);
        switch (cu)
        {
            case 'V':    logic = REVERSE;
               break;
            case 'X':    exact = YES;
               break;
            case 'H':    showname = NO;
               break;
            case 'S':    subtreesrch = YES;
               break;
            case 'I':    casesensitive = NO;
               break;
            case 'Y':    casesensitive = NO;
               break;
            case 'C':    if (nameonly == NO)
                           {
                             countonly = YES;
                             prtlines = NO;    /* don't print detail */
                           }
                         else
                           {
                              tell_usage();
                              return(BAILOUT);
                           }
               break;
            case 'L':    if (countonly == NO)
                           {
                             nameonly = YES;
                             prtlines = NO;
                           }
                         else
                           {
                              tell_usage();
                              return(BAILOUT);
                           }
               break;
            case 'B':    if (showlinenum == NO)
                           showblocknum = YES;
                         else
                           {
                              tell_usage();
                              return(BAILOUT);
                           }
               break;
            case 'N':    if (showblocknum == NO)
                           showlinenum = YES;
                         else
                           {
                              tell_usage();
                              return(BAILOUT);
                           }
               break;
            case 'E':    if (StringFile == NULL)
                           pattern[0] = optarg;
                         else
                           {
                              tell_usage();
                              return(BAILOUT);
                           }
               break;
            case 'F':    if (pattern[0] == NULL)
                           StringFile = optarg;
                         else
                           {
                              tell_usage();
                              return(BAILOUT);
                           }
               break;
            default:     tell_usage();
                         return(BAILOUT);
               break;

        } /* endswitch */
    }

    args = optind;
    if ( (pattern[0] == NULL) && (StringFile == NULL) )
      if  (args < argc)
        pattern[0] = argv[args++];

    if (StringFile != NULL)
      if (pattern[0] == NULL)
        {
          if (getpatterns(StringFile) == BAILOUT)
            return(BAILOUT);
        }
      else
        {
          tell_usage();
          return(BAILOUT);
        }

    if ( (pattern[0] == NULL) && (StringFile == NULL) )
      {
        tell_usage();
        return(BAILOUT);
      }

    if (args < argc)
      rtrnindx = args++;

    if (rtrnindx == 0)
      {
         showname = NO;
         showblocknum = NO;
      }
    else
       {
          if (args < argc)  /* there's more than one fspec on cmd line */
            showname = YES;       /* Always show file names */
          else                /* if there is only one, nonambiguous fspec */
            if ( (strchr(argv[rtrnindx], '?') == 0)
                  && (strchr(argv[rtrnindx], '*') == 0) )
              showname = NO;      /* No need to show file names */
       }

    if (casesensitive == NO)  /* upper case the patterns if search is */
      if (pattern != NULL)              /* insensitive */
        for (i = 0; pattern[i]; i++)
          for (p = pattern[i]; *p; p++)
            *p = (char)toupper(*p);
    return(rtrnindx);
}

/*----------------------------------------------------------------------------*/
/*  getpatterns                                                               */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*----------------------------------------------------------------------------*/
int getpatterns(char *filename)
{
   FILE *stream;
   char buff[MAXLINE], *p, *q;
   int i;

   i = 0;
   if ( (stream = fopen(filename, "r")) != NULL)           /* @4c */
     {
       while (fgets(buff, MAXLINE, stream) != NULL)
         {
           if (i >= MAXPATTERNS)
             {
               fprintf(stderr,
               "Only the first %d patterns in the pattern file will be used\n",
               MAXPATTERNS);
               break;
             }
           if ( (pattern[i] = (char *)malloc(strlen(buff) + 1)) != NULL )
             {
               for (p = pattern[i], q = buff;
                    *q && (*q != CR) && (*q != LF);
                    *p++ = *q++);
               *p = '\0';
               i++;
             }
           else
             {
               myerror(ERROR_NOT_ENOUGH_MEMORY, "getting space for patterns", "");
               return(BAILOUT);
             }
         }
       fclose(stream);
     }
   else
     {
       printf("Could not open StringFile %s\n", filename);
       return(BAILOUT);
     }
   return(YES);
}
/*----------------------------------------------------------------------------*/
/*  do_a_file                                                                 */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*----------------------------------------------------------------------------*/
void do_a_file(char *filename)
{
    USHORT driveno;
//    int   handle;
    FILE *stream;
    FSALLOCATE fsinfobuf;

    if (showblocknum == YES)
      {
         if (*(filename + 1) == ':')
           driveno = toupper(*filename) - 'A' + 1;
         else
           driveno = 0;
         DosQFSInfo(driveno, 1, (PBYTE)&fsinfobuf, sizeof(FSALLOCATE));
         bytes_per_allocunit = fsinfobuf.cbSector * fsinfobuf.cSectorUnit;
      }

//    if ( (handle = open(filename, O_BINARY | O_RDONLY)) != -1 )
    if ( (stream = fopen(filename, "rb"))  != NULL)
        {
          do_the_stream(stream, filename);
          fclose(stream);
        }
      else
        {
          rtrnstatus = SOMEERROR;
//          perror(filename);
          myerror(0, "File open error", filename);
        }
//    else
//      rtrnstatus = SOMEERROR;
}

/*----------------------------------------------------------------------------*/
/*  do_STDIN                                                                  */
/*                                                                            */
/*  Same thing as do_a_file(), really, except that we already know the handle.*/
/*                                                                            */
/*----------------------------------------------------------------------------*/
void do_STDIN()
{
   FILE *stream;

   bytes_per_allocunit = 999999;
   if ( (stream = fdopen(STDIN, "rb"))  != NULL)
      do_the_stream(stream, stdinC);
   else
      {
        rtrnstatus = SOMEERROR;
//        perror("stdin->stream");
        myerror(0, "Opening STDIN as stream", "");
      }
}

/*----------------------------------------------------------------------------*/
/*  do_the_stream                                                             */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*----------------------------------------------------------------------------*/
void do_the_stream(FILE *stream, char *filename)
{
   long bytes_read = 0; /* keeps track of bytes we've read so far */
   long matchcnt = 0;
   size_t line, bufnum, BytesInBuff;
   int  match, i;                                                    /*@2c2*/
   char *p, *q, *bufend, **pp;
   char cisbuf[MAXLINE + 1];

   line = 1;
   bufnum = 0;
   while ( (BytesInBuff = getbuff(stream, bufnum, filename, &bufend)) != EOF)
     {
       getline(INIT, stream, NULL);
       while ( (p = getline(NEXT, stream, bufend)) )
         {
           match = NO;
           for (pp = pattern; !match && *pp; pp++)
             {
               if (casesensitive == NO)  /* uppercase line for CaseInSensitive*/
                 {
// /*@2d*/           for (q = p, r = cisbuf; *q; *r++ = (char)toupper(*q++));
   /*@2a2*/        for (q=p, i=0; *q && i<MAXLINE;
                                               cisbuf[i++]=(char)toupper(*q++));
   /*@2c*/         cisbuf[i] = 0;
                   q = strstr(cisbuf, *pp);  /* match uppercased line */
                 }
               else
                 q = strstr(p, *pp);         /* match natural text */

               if (q)
                 {
                   if (exact == NO)
                     {
                       if (logic != REVERSE)
                         match = YES;
                     }
                   else
                     if (strcmp(p, *pp) == 0)
                       {
                         if (logic != REVERSE)
                           match = YES;
                       }
                     else
                       if (logic == REVERSE)
                         match = YES;
                 }
               else
                 if (logic == REVERSE)
                   match = YES;
             }
           if (match == YES)
             {
               matchcnt++;
               if (prtlines == YES)
                 showline(p, filename, line, bytes_read);
               else
                 if (nameonly == YES)
                   {
                     showfname(filename);
                     return;
                   }
             }
           line++;
         }  /* end WHILE fetching lines from the buffer */
       bufnum++;
       bytes_read += BytesInBuff;
     } /* end WHILE reading buffers from file */
   if (countonly == YES)
     showcount(filename, matchcnt);
   if (matchcnt > 0) rtrnstatus = MATCHFOUND;
}

/*----------------------------------------------------------------------------*/
/*  getline                                                                   */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*----------------------------------------------------------------------------*/
char *getline(int action, FILE *stream, char *bufend)
{
   static char *nextline;
   static char endchar, *endcharP;
   char *linestart, *linend, *p, c;

   if (action == INIT)
     {
       nextline = buf;
       endcharP = NULL;
       return(NULL);
     }

   if (nextline == bufend)
     return(NULL);

   if (nextline == endcharP)
     *endcharP = endchar;

   linestart = nextline;
   linend = ( (bufend - nextline) > MAXLINE ? nextline + MAXLINE : bufend);

//   for (p = nextline; *p && *p!=CR && *p!=LF && p != linend; p++);
//   for (p = nextline; *p > 0x1f && p != linend; p++);
//   for (p = nextline; (*p > 0x1f || *p == TAB) && p != linend; p++);
   for (p = nextline; p != linend && (*p > 0x1f || *p == TAB); p++);
   if (p == linend)
     {
       if (p == bufend)       /* we have come to the end of the buffer. */
         if (!feof(stream))   /* If there are more buffers */
           return(NULL);           /* pick up this line later on */
       endchar = *p;   /* save current contents of last byte*/
       endcharP = p;   /* so that we don't lose data when we */
       *p = 0;         /* null terminate the sting */
       nextline = p;   /* since we didn't really reach eol, start here next time*/
     }
   else
     {
       endcharP = NULL;
       c = *p;        /* save terminating character */
       *p++ = 0;      /* null terminate the line */
       nextline = p;
       if (nextline != bufend)                /* @3a */
         {
           if (c == CR)
             {
               if (*p == LF)
                 nextline++;
             }
           else
             if (c == LF)
               if (*p == CR)
                 nextline++;
         }
     }
   return(linestart);
}

/*----------------------------------------------------------------------------*/
/*  getbuff                                                                   */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*----------------------------------------------------------------------------*/
size_t getbuff(FILE *stream, int bufnum, char *filename, char **putbufend)
{
  size_t toread, bytesread, stublen;
  char  *p, *q;

  if (bufnum == 0)
    {
      toread = BUFSIZE;
      stublen = 0;
      bytesread = fread(buf, sizeof(char), toread, stream);
    }
  else
    {
      if (feof(stream))
        return(EOF);
      for (p = BUFEND, q = p - MAXLINE; /* find start of last line in buff */
           *p != CR && *p != LF && p != q;
           p--);
      stublen = BUFEND - p - 1;      //@5c
      p++;
      for (q = buf; p < BUFEND; *q++ = *p++); /* move it to head of buffer  @5c*/
      toread = (size_t)BUFSIZE - stublen;      /* append next read onto it */
      bytesread = fread(buf + stublen, sizeof(char), toread, stream);
    }
  if (bytesread < toread)
    if (feof(stream) && (bytesread == 0) )
      return(EOF);
    else
      if (!feof(stream))
        {
          myerror(0, "File read error", filename);
          return(EOF);
        }                                  /* compute the address of a point */
  *putbufend = buf + stublen + bytesread; /* one byte right of last byte in */
                                           /* the buffer.                    */
  return(bytesread);
}

/*----------------------------------------------------------------------------*/
/*  showline                                                                  */
/*----------------------------------------------------------------------------*/
void showline(char *line, char *filename, int lineno, long bytesread)
{
  long block;

  if (showname == YES)
    printf("%s:", filename);
  if (showlinenum == YES)
    printf("%d:", lineno);
  if (showblocknum == YES)
    {
      block = bytesread + (line - buf);
      block /= bytes_per_allocunit;
      printf("%ld:", ++block);
    }
  printf("%s\n", line);
}

/*----------------------------------------------------------------------------*/
/*  showname                                                                  */
/*                                                                            */
/*----------------------------------------------------------------------------*/
void showfname(char *filename)
{
   printf("%s\n", filename);
}

/*----------------------------------------------------------------------------*/
/*  showcount                                                                 */
/*                                                                            */
/*----------------------------------------------------------------------------*/
void showcount(char *filename, long matchcount)
{
   if (showname == YES)
     printf("%s:", filename);
   printf("%ld\n", matchcount);
}

/*----------------------------------------------------------------------------*/
/*  tell_usage                                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/
void tell_usage()
{
  printf("\n      Copyright IBM, 1990, 1992\n");
  printf("fgrep searches a file for a pattern.\n\n");
  printf("Usage:\n");
  printf("      fgrep [-vxhsi]  [c | -l]  [-b | -n]\n");
  printf("            [Pattern | -e Pattern | -f StringFile] [File [File...\n");

  printf("\nFlags:\n");
  printf("    -b            Precedes each line by block number where found\n");
  printf("    -c            Displays only a count of matching lines.\n");
  printf("    -e Pattern    Specifies a pattern.\n");
  printf("    -f StringFile Specifies a file that contains strings to be matched.\n");
  printf("    -h            Suppresses file names when multiple files are being\n");
  printf("                  processed.\n");
  printf("    -i            Ignores the case of letters when making comparisons.\n");
  printf("    -l            Lists just the names of files that contain matches.\n");
  printf("    -n            Precedes each line with its line number\n");
  printf("    -s            Search all subdirectories below the specified path.\n");
  printf("    -v            Displays all lines except those that match pattern.\n");
  printf("    -x            Displays lines that match the pattern exactly.\n");
  printf("    -y            Ignore case of letters when making comparisons.\n");
}
