static char sccsid[]="@(#)36	1.1  src/cut/cut.c, aixlike.src, aixlike3  9/27/95  15:44:14";
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation, 1994.  All rights reserved.     */
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1 9/12/94                                                */
/*-----------------------------------------------------------------------*/

/* 
The cut command writes out selected fields from each line of a file.

Syntax

        cut -cList or -fList -dCharacter -s [file [file ...

(the default "Character" is tab.)

Description

The cut command cuts columns from a table or fields from each line of a file,
and writes these columns or fields to standard output.  If you do not 
specify a file name, the cut command reads standard input.

You must specify either the -c or -f flag.  The List parameter is a comma-
seperated and/or minus-seperated list of integer field numbers (in
increasing order).  The minus seperator indicates ranges.  Some sample
List parameters are
        1,4,7
        1-3,8
        -5,10   (short for 1-5,10)
        3-      (short for 3 to the last field)
The fields specified the by the List parameter can be a fixed number of
character positions, or the length can vary from line to line and be marked
with a field delimiter character, such as a tab character.

You can also use the grep command to make horizontal cuts through the file
and the paste commands to put the files back together.  To change the order
of columns in a file use the cut and paste commands.

Flags

        -cList          Specifies character positions.  For example, if you
                        specify -c1-72, the cut command writes out the first
                        72 characters in each line of the file.  Note that
                        there is no space between -c and the List parameter.
 
        -dCharacter     Uses the character specified by the Character parameter
                        as the field delimiter when you specify the -f flag.
                        You must put quotation marks around characters
                        with special meaning to the shell, such as the space
                        character.

        -fList          Specifies a list of fields assumed to be separated
                        in the file by a delimiter character, which is by 
                        default the tab character.  For example, if you
                        specify -f1,7, the cut command writes out only the 
                        first and seventh fields of each line.  If a line
                        contains no field delimiters, the cut command passes
                        them through intact (useful for table subheadings),
                        unless you specify the -s flag.

        -s              Suppresses lines that do not contain delimiter
                        characters (use only with the -f flag).
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>
#include "fmf.h"

#define ISAMBIGUOUS(x) ( (strchr(argv[optind], '*') != 0) || \
                         (strchr(argv[optind], '?') != 0) )
#define YES 1
#define NO 0
#define BAILOUT -1
#define CR  0x0D
#define LF  0x0A
#define TAB 0x09
#define DEFAULT_SEPERATOR 0x09
#define SPACE 0x20
#define MAXLINE 50000
#define COLON ':'

/* States for the List parser */
#define START 1
#define CBRANGE 2
#define ISRANGE 3
#define RANGEEND 4

struct fldtable {
                  int low;
                  int high;
                  struct fldtable *next;
                };

/* globals */
char *opstring = "c:d:f:s";
char inbuf[MAXLINE+1];
char outbuf[MAXLINE+1];
#define NOTSET 0
#define CHARMODE 1
#define FIELDMODE 2
unsigned int mode = NOTSET;        /* character or field */
unsigned int fldoutsep = COLON;
unsigned int fldinsep = DEFAULT_SEPERATOR;
unsigned int suppresslines = NO;
char *fldlist = NULL;
struct fldtable *pft = NULL;

/* Function Prototypes */

int init(int argc, char **argv);
unsigned int dofilespec(char *filespec);
int dofile(char *filename);
int dostream(FILE *stream, char *filename);
int buildfldtable(char *list);
struct fldtable *addentry();
int buildline(char *obuf, char *ibuf);
void transferfield(char **in, char **out, int field);
void advancefield(char **in);
void tell_usage(void);
extern int getopt(int argc, char **argv, char *ops);
extern int optind;
extern char *optarg;

int main(argc, argv, envp)                                      /* @2c */
   int argc;
   char *argv[];
   char *envp[];
{
   int fnindex;
   unsigned int filesdone = 0;

   if ( (fnindex = init(argc, argv)) != BAILOUT)
     if (fnindex)      /* if at least one filespec given */
       for (; fnindex < argc; fnindex++)
         filesdone += dofilespec(argv[fnindex]);
     else
         dostream(stdin, "STDIN");
   else                                                         /* @2a */
     return(BAILOUT);                                           /* @2a */
   return(0);                                                   /* @2a */
}

int init(int argc, char **argv)
{
   int c;
   int dspecified = NO;
   char *p;

   while ( (c = getopt(argc, argv, opstring)) != EOF)
          {
            switch (c)
            {
               case 'c'  :   mode = CHARMODE;
                             fldlist = optarg;
                         break;
               case 'f'  :   mode = FIELDMODE;
                             fldlist = optarg;
                         break;
               case 'd'  :   fldinsep = *optarg;
                             dspecified = YES;
                         break;
               case 's'  :   suppresslines = YES;
                         break;
               default   :   tell_usage();
                             return(BAILOUT);
            }  /* end switch */
          }


     if (mode == NOTSET ||
         (dspecified == YES && mode == CHARMODE) ||
         (suppresslines == YES && mode == CHARMODE) ||
         *fldlist == 0)
       {
          tell_usage();
          return (BAILOUT);
       }

     if (buildfldtable(fldlist) == BAILOUT)
       return(BAILOUT);


     if ( optind < argc)
       return(optind);
     else
       return(0);
}

unsigned int dofilespec(char *filespec)
{
   unsigned int files_in_filespec = 0;
   char filename[CCHMAXPATH];
   int attrib;

   if (fmf_init(filespec, FMF_ALL_FILES, 0) == FMF_NO_ERROR) // @1
     while (fmf_return_next(filename, &attrib) == NO_ERROR)
       {
         if (dofile(filename))
           files_in_filespec++;
       }
   return(files_in_filespec);
}

int dofile(char *filename)
{
   FILE *stream;

   if ( (stream = fopen(filename, "rb")) == NULL)
     return(NO);
   else
     {
       if (dostream(stream, filename) == BAILOUT)
         return(BAILOUT);
       fclose(stream);
       return(YES);
     }
}

int dostream(FILE * stream, char *filename)
{
    char *p;


    while (fgets(inbuf,MAXLINE,stream))
       {
          for (p = inbuf + strlen(inbuf) - 1; p > inbuf; p--)
            if (*p == '\r' || *p == '\n')
              *p = '\0';
            else
              break;
          if (buildline(outbuf,inbuf))
            printf("%s\n",outbuf);
       }
}

/**************************************************/
/* This routine takes a comma-seperated list of   */
/* field numbers and ranges and creates a linked  */
/* list of structures containing the low and high */
/* range for each                                 */
/**************************************************/
int buildfldtable(char *list)
{
   char *p, *q;
   int state;
   struct fldtable *pthis;

   p = list;
   state = START;
   while (*p)
      {
        switch (state)
        {
           case START:       pthis = addentry();
                             if (!pthis)
                               return(BAILOUT);
                             if (*p == '-')
                               pthis->low = 1;
                             else
                               while (isdigit(*p))
                                  {
                                     pthis->low *= 10;
                                     pthis->low += (*p - '0');
                                     p++;
                                  }
                             if (!*p)
                                pthis->high = pthis->low;
                             state = CBRANGE;
                             break;

           case CBRANGE:     switch (*p)
                             {
                                case ',': pthis->high = pthis->low;
                                          state = START;
                                          p++;
                                          break;
                                   
                                case '-': state = ISRANGE;
                                          p++;
                                          break;

                                default:  tell_usage();
                                          return(BAILOUT);
                                          break;
                             } /* end switch */
                             break;

           case ISRANGE:     if (*p != ',' && *p != '\0')
                               {
                                 pthis->high = 0;
                                 while (isdigit(*p))
                                    {
                                       pthis->high *= 10;
                                       pthis->high += (*p - '0');
                                       p++;
                                    }
                               }
                             if (pthis->high == 0)
                               {
                                  tell_usage();
                                  return(BAILOUT);
                               }
                             state = RANGEEND;
                             break;
 
           case RANGEEND:    switch (*p)
                             {
                                case ',': p++;
                                          state = START;
                                          break;
                                   
                                case '\0': break;

                                default:  tell_usage();
                                          return(BAILOUT);
                                          break;
                             } /* end switch */
                             break;
           } /* end switch */
 
      }  /* end while */
   return(0);
}


/**************************************************/
/* builds a new field table entry and puts it in  */
/* the list                                       */
/**************************************************/
struct fldtable *addentry()
{
   struct fldtable *root, *oldtail, *newtail;

   newtail = (struct fldtable *)malloc(sizeof(struct fldtable));
   if (newtail)
     {
        newtail->low = 0;
        newtail->high = MAXLINE;
        newtail->next = NULL;
     }
   if (pft == NULL)
      pft = newtail;
   else
      {
        for (oldtail = pft; oldtail->next; oldtail = oldtail->next);
        oldtail->next = newtail;
      }
   return(newtail);
}

/**************************************************/
/* Build an output line - extract the requested   */
/* fields from the input line until it is exhausted*/
/**************************************************/
int buildline(char *obuf, char *ibuf)
{
   char *ip, *op;
   int field = 0;
   int fieldfound = 0;

   if (mode==FIELDMODE && strchr(ibuf,fldinsep)==NULL)
     if (suppresslines)
       return(0);
     else
       {
          strcpy(obuf,ibuf);
          return(1);
       }
   op = obuf;
   ip = ibuf;
   while (*ip)
     if (fieldischosen(++field))
       {
         transferfield(&ip, &op,fieldfound);
         fieldfound++;
       }
     else
         advancefield(&ip);
   *op = 0;
}         
 
 
/**************************************************/
/* Is this field chosen?                          */
/**************************************************/
int fieldischosen(int fld)
{
   struct fldtable *p;

   for (p = pft; p; p = p->next)
      if (fld >= p->low && fld <= p->high)
         return(YES);
   return(NO);
}

/**************************************************/
/* This field is chosen: move it to the output buf*/
/**************************************************/
void transferfield(char **in, char **out, int field)
{
   char *p, *q, *end;
 
   p = *in;
   q = *out;

   if (mode == CHARMODE)
      *q++ = *p++;
   else
     {
          {
            if (field)
              *q++ = fldinsep;
         
            if ((end = strchr(p,fldinsep))==NULL)
              end = p + strlen(p);
            for (;p < end; *q++ = *p++);
            if (*p)     /* skip over the seperator */
              p++;
          }
     }
   
   *in =  p;
   *out = q;
}

/**************************************************/
/* This field was not chosen: skip over it        *.
/**************************************************/
void advancefield(char **in)
{
   char *p;

   p = *in;

   if (mode == CHARMODE)
     p++;
   else
     {
        for (; *p && *p != fldinsep; p++);
        if (*p)
          p++;
     }
   *in = p;
}


void tell_usage()
{
   printf("cut - Copyright IBM, 1994\n");
   printf("The cut command writes out selected fields from each line of a file.\n");
   printf("and writes these columns or fields to standard output.\n");
   printf("Usage:\n");
   printf("     cut -cList or -fList -dCharacter -s [file [file ...\n");
   printf("\nFlags:\n");
   printf("   -cList          Specifies character positions. .  Note that\n");
   printf("                   there is no space between -c and the List parameter.\n");
   printf("   -dCharacter     Uses the character specified by the Character parameter\n");
   printf("                   as the field delimiter when you specify the -f flag.\n");
   printf("    -fList         Specifies a list of fields\n");
   printf("    -s             Suppresses lines that do not contain delimiter\n");
   printf("                   characters (use only with the -f flag).\n");
   printf("The default \"Character\" is tab.  Some sample \"List\"s are:\n");
   printf("     1,4,7   (specifies 3 fields: field 1, field 3 and field 7)\n");
   printf("     1-3,8   (specified fields 1 through 3 and field 8)\n");
   printf("        -5,10   (short for 1-5,10)\n");
   printf("     3-      (short for 3 to the last field)\n");

}
