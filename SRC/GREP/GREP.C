static char sccsid[]="@(#)47	1.1  src/grep/grep.c, aixlike.src, aixlike3  9/27/95  15:44:38";
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
/*  @1  05/02/91 Trap when using -i reported by Ron Schwabel             */
/*  @2  05/03/91 Trap when using -p reported by Lykle Shepers            */
/*  @3  05/06/91 grep didn't act as filter reported by Jim Shipman       */
/*  @4  05/07/91 Jim Shipman reported cmd line parsing was wrong         */
/*  @5  05/08/91 changed fmf_init to look for all files, including hidden*/
/*  @6  05/28/91 Zac Corbierre reported a trap D when a certain file was */
/*               piped into grep.  grep was reading past end of buffer.  */
/*  @7  12/18/91 Peter Schwaller noticed parts of lines were missing.    */
/*  @8  02/17/91 84894951 reported a trap D on long files using -p.      */
/*  @9  05/09/92 On case insensitive search there were spurious matches  */
/* @10  01/18/94 On case insensitive search with line > 512, trap        */
/* @11  02/14/94 -p never did work quite right                           */
/* @12  08/31/94 trouble finding stuff on 50000 byte boundries           */
/*-----------------------------------------------------------------------*/
/* grep: searches a file for a pattern.

   Usage:
          grep [-i][-w][-y][-q][-s][v][-pSeperator [-c | -l] [-b | -n]
                [Pattern | -e Pattern | File [File...
*/

/* The grep command searches fot the pattern specified by the Pattern parameter
and writes each matching line to standard output.  The patterns are limited
regular expressions in the style of the ed command (ALTHOUGH SUBPATTERNS ARE
NOT SUPPORTED IN THIS VERSION).

The grep command displays the name of the file containing the matched line if
you specify more than one name in the File parameter.  Characters with special
meaning to the OS2 shell must be quoted when they appear in the Pattern
parameter.  When the Pattern parameter is not a simple string, you usually
must enclose the entire pattern in double quotation marks.  In an expression
such as [a-z], the minus means through according to the current collating
sequence.  A collating sequence may define equivalence classes for use in the
character ranges.

The exit values of this command are:
  0   A match was found
  1   No match was found
  2   A syntax error was found or a file was inaccessible (even if matches were
      found).

Notes:
  1. Lines are limited to 512 characters; longer lines are broken into
     multiple lines of 512 or fewer characters.

  2. Paragraphs (under the -p flag) are currently limited to a length of 5000
     characters.

Flags:
      -b            Precedes each line by the block number on which it was
                    found. Use this flag to help find disk block numbers
                    by context. (In this implementation the relative allocation
                    unit number is shown);

      -c            Displays only a count of matching lines.

      -e Pattern    Specifies a pattern.  This works the same as a simple
                    pattern, but is useful when a pattern begins with a -.

      -i            Ignores the case of lettters when making comparisons.

      -l            Lists just the names of files (once) with matching
                    lines.  Each file name is separated by a new-line
                    character.

      -n            Precedes each line with its relative line number in the
                    file.

      -pSeperator   Displays the entire paragraph containing matched lines.
                    Paragraphs are delimited by paragraph seperators, as
                    specified by the Seperator parameter, which is a pattern
                    in the same form as the search pattern.  Lines containing
                    the paragrphs seperators are used only as seperators; they
                    are never included in the output.  The default paragraph
                    seperator is a blank line.

      -q            Suppresses error messages about files that are not
                    accessible.

{      -s            Displays only error messages.  This is useful for checking
                    status. } (superceded in the OS/2 version).

      -s            Search all subdirectories below the specified path.

      -v            Displays all lines except those that match the specified
                    pattern.

      -w            Does a word search.

      -y            Ignore case of letters when making comparisons
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>
#include "fmf.h"
#include "grep.h"

#define BUFSIZE    50000
#define MAXPARAGRAPH (unsigned)5000
/* define the default paragraph seperator  @11a */
#define DEFAULT_SEPERATOR "^[\t ]*$"
#define MAXLINE 512
#define MAXCPPATTERN 255
#define MAXCSPATTERN 255
#define MATCHFOUND     0
#define NOMATCHFOUND   1
#define SOMEERROR      2
#define YES            1
#define NO             0
#define BAILOUT       -1
#define INIT           0
#define NEXT           1
#define CR            0x0d
#define LF            0x0a
#define STDIN          0
#define NONPOLISH      0
#define REVERSE        1
#define BUFEND         (buf + BUFSIZE)


// char *optstring = "bce:ilnpqsvwy";                             // @4
char buf[BUFSIZE + 1];                   /* input buffer */
char *paragraph_start;                   /* start of current paragraph */
size_t offset_to_line_start = 0;         /* line start this far into paragraph*/
size_t stream_line = 0;                  /* line number within file  */
char *pattern = NULL;                    /* pointer to the search pattern */
char compiled_ppattern[MAXCPPATTERN];    /* buffer for compiled para sep ptrn*/
char compiled_spattern[MAXCSPATTERN];    /* ditto for compiled search pattern */

                   /*----------------------------*/
                   /* Options and their defaults */
                   /*----------------------------*/
char *filespec = NULL;          /* file(s) to be searched */
char *StringFile = NULL;        /* name of file containing patterns */
                                /* default paragraph seperator is blank line */
/* char *paragraph_seperator = "^ *$";                         @11d */
char *paragraph_seperator = DEFAULT_SEPERATOR;              /* @11a */
int  logic = NONPOLISH;         /* search for matching or non-matching files? */
int  showname = YES;            /* show name of file */
int  show_paragraph = NO;       /* display whole paragraph containing match */
int  subtreesrch = NO;          /* search subtrees for matching files ? */
int  casesensitive = YES;       /* respect case in comparing to pattern? */
int  nameonly = NO;             /* show only names of files having pattern? */
int  countonly = NO;            /* show only a count of files having pattern? */
int  prtlines = YES;            /* print each line that matches pattern? */
int  showlinenum = NO;          /* show line number with matching line? */
int  showblocknum = NO;         /* show block number containing the line? */
int  word_search = NO;          /* Do word search */
int  quiet = NO;                /* Suppress file-access error messages */
ULONG bytes_per_allocunit;       /* used to calculate block number */
ULONG bytes_read_so_far;         /* keeps track of bytes read so far */
int  count = 0;                 /* counter of files containing pattern */
int  matchesinfile = 0;         /* number of matches in the current file */

                   /*----------------------------*/
                   /*    Function Prototypes     */
                   /*----------------------------*/
int init(int argc, char *argv[]);   /* Initialize from command line data */
void do_a_file(char *filename);     /* Logic for converting file to stream */
void do_STDIN(void);                /* Logic when input file is stdin      */
                                    /* Logic for matching lines in one file */
void do_the_stream(FILE *stream, char *filename);
                                    /* Fill the program's data buffer */
size_t getbuff(FILE *stream, int bufnum, char *filename, char **putbufend);
                                    /* Isolate a line withing the buffer */
char *getline(int action, FILE *stream, char *bufend);
                                    /* Print out a matching line */
void showline(char *line, char *filename, size_t lineno, long bytesread);
void showpara(char *line, char *filename, size_t lineno, long bytesread,
              FILE *stream, char *bufend);
void showfname(char *filename);     /* Print matching file name only */
                                    /* Print count of matching lines */
void showcount(char *filename, long matchcount);
void tell_usage(void);              /* Explain program usage */
                                    /* Parse command line */
                                    /* put out errors in a standard format */
void myerror(int rc, char *area, char *details);
extern char *myerror_pgm_name;      /* data imported by myerror */


int  rtrnstatus = NOMATCHFOUND;     /* value returned on exit from grep */

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

   myerror_pgm_name = "grep";
   if ( (filespecindx = init(argc, argv)) == BAILOUT)
     return(SOMEERROR);
   else
     if (filespecindx > 0)
       {
         for (i = filespecindx; i < argc; i++)
           {
//            if ( (rc = fmf_init(argv[i], 0, subtreesrch)) == NO_ERROR)     @5
            if ( (rc = fmf_init(argv[i], FMF_ALL_FILES, subtreesrch))
                                                          == FMF_NO_ERROR) //@5
              {
                while (fmf_return_next(filename, &attrib) == NO_ERROR)
                  do_a_file(filename);
                fmf_close();
              }
            else
              myerror(rc, "Finding filespec", argv[i]);
           }
       }
     else                                                // @3
       do_the_stream(stdin, "stdin");                    // @3

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
    char c, *p;

    for (i = 1; i < argc; i++)
    {
       p = argv[i];
       if (*p != '-' && *p != '/')
         break;                    /* a non-switch was found. switches done*/
       for (++p; *p; p++)                                     // @4
       {                                                      // @4
//       c = *++p;                                               @4
        c = *p;                                               // @4
        switch (toupper(c))
        {
            case 'V':    logic = REVERSE;
               break;
            case 'S':    subtreesrch = YES;
               break;
            case 'I':    casesensitive = NO;
               break;
            case 'Y':    casesensitive = NO;
               break;
            case 'W':    word_search = YES;
               break;
            case 'Q':    quiet = YES;
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
            case 'E':    if (pattern == NULL)                   //@4
                           {                                    //@4
                             if (*(p+1))                          //@4
                               p++;                               //@4
                             else                                 //@4
                               if (++i < argc)                    //@4
                                 p = argv[i];                     //@4
                               else                               //@4
                                 {                                //@4
                                    tell_usage();                 //@4
                                    return(BAILOUT);              //@4
                                 }                                //@4
                             pattern = p;                         //@4
                             for (; *p; p++);                     //@4
                             p--;                                 //@4
                           }                                      //@4
                         else
                           {                                   // @4
                              tell_usage();                    // @4
                              return(BAILOUT);                 // @4
                           }
               break;
            case 'P':    if (*++p)
                           {                                   // @4
                             paragraph_seperator = p;
                             for (; *p; p++);                     //@4
                           }
                         p--;                                 //@4
                         show_paragraph = YES;
               break;
            default:     tell_usage();
                         return(BAILOUT);
               break;

        } /* endswitch */
       } /* end for all switches in this cmd line arg */
    }   /* end for all command line arguments that have switches */

    args = i;
    if ( pattern == NULL)
      if  (args < argc)
        pattern = argv[args++];
      else
        {
          tell_usage();
          return(BAILOUT);
        }

    if (args < argc)
      rtrnindx = args++;
    else
      rtrnindx = 0;                                      // @3
//      {                                                   @3
//        tell_usage();                                     @3
//        return(BAILOUT);                                  @3
//      }                                                   @3

    if (args < argc)  /* there's more than one fspec on cmd line */
      showname = YES;       /* Always show file names */
    else                /* if there is only one, nonambiguous fspec */
      if ( (strchr(argv[rtrnindx], '?') == 0)
                  && (strchr(argv[rtrnindx], '*') == 0) )
        showname = NO;      /* No need to show file names */

    if (casesensitive == NO)  /* upper case the patterns if search is */
      for (p = pattern; *p; p++)
        *p = (char)toupper(*p);

    if (compile_reg_expression(pattern, compiled_spattern, MAXCSPATTERN) == 0)
      {
         printf("grep: Invalid search pattern\n");
         return(BAILOUT);
      }
    else
      if (compile_reg_expression(paragraph_seperator, compiled_ppattern,
                                MAXCPPATTERN) == 0)
        {
           printf("grep: Invalid paragraph seperator pattern\n");
           return(BAILOUT);
        }

    return(rtrnindx);
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

    if ( (stream = fopen(filename, "rb"))  != NULL)
      {
        do_the_stream(stream, filename);
        fclose(stream);
      }
    else
      {
        rtrnstatus = SOMEERROR;
        if (!quiet)
          myerror(0, "File open error", filename);
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
   size_t bufnum, BytesInBuff;
   int  match, len;
   char *p, *q, *l, *bufend;
/*   char cisbuf[MAXLINE + 1];                                        @10d */
   char cisbuf[MAXLINE + 4];                                       /* @10a */

   stream_line = 1;
   bufnum = 0;
   while ( (BytesInBuff = getbuff(stream, bufnum, filename, &bufend)) != EOF)
     {
       getline(INIT, stream, NULL);

       while ( (l = getline(NEXT, stream, bufend)) )
         {
           if ( show_paragraph )
             {
               if (matches_reg_expression(l, compiled_ppattern) )  // @2
                 {
                   paragraph_start = NULL;
                   offset_to_line_start = 0;
                   continue;
                 }
               else
                 {
                    if (paragraph_start)                             // @8a
                      offset_to_line_start = l - paragraph_start;    // @8a
                    else                                             // @8a
                      {                                              // @8a
                         paragraph_start = l;                        // @8a
                         offset_to_line_start = 0;                   // @8a
                      }                                              // @8a
                 }                                                   // @8a
//                 offset_to_line_start = l - paragraph_start;       // @2 @8d
//               if (paragraph_start == NULL)                              @8d
//                 paragraph_start = l;                              // @2 @8d
             }

           if (casesensitive == NO)  /* if search is case insensitive */
             {
               p = cisbuf;    /* we put the upper-cased line into cisbuf @1 */
               q = l;         /* l is where the raw line is stored       @1 */
               for (; *q; p++, q++)  /* upper case each character in     @1 */
                 *p = (char)toupper(*q);   /* line                       @1 */
               *p = '\0';     /* terminate it                            @9a*/
               p = cisbuf;    /* use the upper-cased stuff to compare    @1 */
             }
           else
             p = l;
           match = NO;
           if (word_search)
             {
               while (match == NO)
                 {
                   if ( (q = find_reg_expression(p, compiled_spattern, &len)) &&
                        (!isalnum(q[-1]))                                     &&
                        (!isalnum(q[len])) )
                     if (logic != REVERSE)
                       match = YES;
                     else
                       if (*++q == 0) /* unless we're at the end of the line */
                         break;       /*     try again. */
                   else
                     if (logic == REVERSE)
                       match = YES;
                     else
                       break;         /* no more matches are possible */
                 }
             }
           else
             if ( matches_reg_expression(p, compiled_spattern) )
               {
                 if (logic != REVERSE)
                   match = YES;
               }
             else
               if (logic == REVERSE)
                 match = YES;

           if (match == YES)
             {
               matchcnt++;
               if (prtlines == YES)
                 if (!show_paragraph)
                   showline(l, filename, stream_line, bytes_read);
                 else
                   showpara(l, filename, stream_line, bytes_read,
                            stream, bufend);
               else
                 if (nameonly == YES)
                   {
                     showfname(filename);
                     return;
                   }
             }

           stream_line++;
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
       nextline = buf + offset_to_line_start;
       endcharP = NULL;
       return(NULL);
     }

   if (nextline == bufend)
     return(NULL);

   if (nextline == endcharP)
     *endcharP = endchar;

   linestart = nextline;
   linend = ( (bufend - nextline) > MAXLINE ? nextline + MAXLINE : bufend);

/*    for (p = nextline; *p > 0x1f && p != linend; p++);               @7d */
   for (p = nextline; *p!=CR && *p!=LF && p != linend; p++);        /* @7a */
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
       if (nextline != bufend)     /* Don't read past end of buffer    @6 */
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
      paragraph_start = NULL;
      offset_to_line_start = 0;                                    /* @8a */
      bytesread = fread(buf, sizeof(char), toread, stream);
    }
  else
    {
      if (feof(stream))
        return(EOF);
//      if (show_paragraph)   /* if we are paragraph-oriented */      @8d
      if (show_paragraph && paragraph_start)   /* if we are paragraph-oriented @8a*/
        if ( (unsigned)(BUFEND - paragraph_start) > MAXPARAGRAPH )    /* @8c */
          {
            p = BUFEND - MAXPARAGRAPH;
            stublen = MAXPARAGRAPH;
            offset_to_line_start = MAXPARAGRAPH;
            paragraph_start = buf;                                 /* @8a */
          }
        else
          {
            p = paragraph_start;
            stublen = BUFEND - paragraph_start;
            paragraph_start = buf;                                 /* @8a */
          }
     else                   /* if we are line oriented */
        {
          offset_to_line_start = 0;                                   /* @8a */
          for (p = BUFEND, q = p - MAXLINE; /* find start of last line in buff*/
              *p != CR && *p != LF && p != q;
               p--);
//          stublen = BUFEND - p;                        @12d
          stublen = BUFEND - p - 1;                   // @12a
          p++;
          paragraph_start = buf + stublen;            /* @8c: was at end */
        }
//      for (q = buf; p <= BUFEND; *q++ = *p++); /* move it to head of buffer */ @12d
      for (q = buf; p < BUFEND; *q++ = *p++); /* move it to head of buffer       @12a*/
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
void showline(char *line, char *filename, size_t lineno, long bytesread)
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
/*  showpara                                                                  */
/*----------------------------------------------------------------------------*/
void showpara(char *dline, char *filename, size_t lineno, long bytesread,
              FILE *stream, char *bufend)
{
  long block;
  char line[MAXLINE + 1];
  char *p, *q, *linend;

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
  /* we know that the whole paragraph, by definition, is in the current    */
  /* buffer. Once we get past the current line, we need to start looking   */
  /* for the end of the paragraph. */

  p = paragraph_start;
  do
    {
      while (!*p || *p == CR || *p == LF)                      /* @7c */
        p++;
      q = line;
      linend = p + MAXLINE;
      while (p < linend && *p && *p != CR && *p != LF)         /* @7c */
        *q++ = *p++;
      *q = '\0';
      printf("%s\n", line);
    }   while (p < dline);

  while (1)         /* we are now past the line that has the match. Do the */
    {                     /* rest of the paragraph */
      if ( (p = getline(NEXT, stream, bufend)) == NULL )
        break;
      stream_line++;
      if (matches_reg_expression(p, compiled_ppattern))
        {
          paragraph_start = NULL;
          offset_to_line_start = 0;
          break;
        }
      else
          printf("%s\n", p);
    }
  printf("\n");    /* seperate paragraphs with blank lines */

}
/*----------------------------------------------------------------------------*/
/*  showfname                                                                 */
/*----------------------------------------------------------------------------*/
void showfname(char *filename)
{
   printf("%s\n", filename);
}

/*----------------------------------------------------------------------------*/
/*  showcount                                                                 */
/*----------------------------------------------------------------------------*/
void showcount(char *filename, long matchcount)
{
   if (showname == YES)
     printf("%s:", filename);
   printf("%ld\n", matchcount);
}

/*----------------------------------------------------------------------------*/
/*  tell_usage                                                                */
/*----------------------------------------------------------------------------*/
void tell_usage()
{
  printf("     Copyright IBM, 1990, 1992\n");
  printf("grep searches a file for a pattern.\n\n");
  printf("Usage:\n");
  printf("      grep [-vxsiywq] [-pSeperator]  [c | -l]  [-b | -n]\n");
  printf("            [Pattern | -e Pattern] [File [File...\n");

  printf("\nFlags:\n");
  printf("    -b            Precedes each line by block number where found\n");
  printf("    -c            Displays only a count of matching lines.\n");
  printf("    -e Pattern    Specifies a pattern.\n");
  printf("    -i            Ignores the case of letters when making comparisons.\n");
  printf("    -l            Lists just the names of files that contain matches.\n");
  printf("    -n            Precedes each line with its line number\n");
  printf("    -s            Search all subdirectories below the specified path.\n");
  printf("    -v            Displays all lines except those that match pattern.\n");
  printf("    -x            Displays lines that match the pattern exactly.\n");
  printf("    -y            Ignore case of letters when making comparisons.\n");
  printf("    -pSeperator   Displays the entire paragraph containing matched lines.\n");
  printf("                  Paragraphs are delimited by paragraph seperators, as\n");
  printf("                  specified by the Seperator parameter, which is a pattern\n");
  printf("                  in the same form as the search pattern.\n");
  printf("\nPattern is a limited regular expression.\n");
}
