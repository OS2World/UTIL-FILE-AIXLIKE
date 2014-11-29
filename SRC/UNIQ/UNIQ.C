static char sccsid[]="@(#)02	1.1  src/uniq/uniq.c, aixlike.src, aixlike3  9/27/95  15:46:36";
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
/* A1  09.16.92  grb  Make return code explicit                          */
/* A2  05.03.93  grb  Modify for IBM C Set/2 compiler                    */
/*-----------------------------------------------------------------------*/
/*
    uniq -- Deletes repeated lines in a file.

    SYNTAX

          uniq [-c][-d][-u] [-Number [+Number] ...] InFile OutFile
               \          /
               one of these

    The uniq command reads standard input or the InFile parameter,
    compares adjacent lines, removes the second and succeeding occurances
    of a line, and writes to standard output or the file specified by
    the OutFile parameter.  The InFile and OutFile parameters should
    always be different files.  Repeated lines must be on consecutive
    lines to be found.  You can arrange them with the sort command before
    processing.

    FLAGS

        -c        Precedes each output line with a count of the number
                  of times each line appears in the file.  This flag
                  supersedes -d and -u.

        -d        Displays only the repeated lines.

        -u        Displays only the unrepeated lines.

        -Number   Skips over the first Number fields.  A field is a string
                  of non-space, non-tab characters separated by tabs
                  and/or spaces from adjacent data on the same line.

        +Number   Skips over the first Number characters.  Fields
                  specified by the Number parameter are skipped before
                  characters.
*/


#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>

#define BAILOUT -1
#define DEFAULT_NUMLINES 10
#define MAXPATHLEN 262
#define MAXLINE 1024

char *InFile;
char *OutFile;
FILE *OutFilef;
int  ShowCount, PrintWhich;
int  SkipFields, SkipChars;
#define   PRINTALL   0
#define   PRINTREP   1
#define   PRINTUNREP 2
char *masterstart;
extern char *myerror_pgm_name;

#define COLON     ':'
#define SLASH     '/'
#define BACKSLASH '\\'
#define TAB       0x09
#define SPACE     0x20
#define ISSEPARATER(x)  ((x == SLASH) || (x == BACKSLASH))

/* function prototypes */

int init(int argc, char **argv);
void do_a_file(char *filename);
void compare(FILE *f);
char *newmaster(char *buf);
char *findstart(char *buf);
void printcount(FILE *f, int count);
void printline(FILE *f, char *line);
FILE *openOutFile(char *fn);
void tell_usage(void);
extern void myerror(int rc, char *area, char *details);
extern int  makepath(char *path);
/***********************************************************************/
/*  main()                                                             */
/***********************************************************************/
int main(int argc, char **argv)                                /* @A1c */
{
   int i;

   myerror_pgm_name = "uniq";
   if ( (i = init(argc, argv)) != BAILOUT)
     if (InFile)
       do_a_file(InFile);
     else
       compare(stdin);
   return(i);                                                  /* @A1a */
}


/***********************************************************************/
/*  init()                                                             */
/***********************************************************************/
int init(int argc, char **argv)
{
   int i;
   char c, *p, *q;
                               /* Initialize the global status variables */
   InFile = NULL;              /* Name of input file */
   OutFile = NULL;             /* Name of output file */
   OutFilef = (FILE *)NULL;    /* file pointer for output file */
   ShowCount = 0;              /* Prefix each line with a count? */
   PrintWhich = PRINTALL;      /* Print all, just repeated or just non-rep lns*/
   SkipFields = 0;             /* Number of fields to skip before matching */
   SkipChars = 0;              /* Number of characters, ditto */

   for (i = 1; i < argc; i++)
     {
       p = argv[i];
       c = *p++;
       if (c == '-')
         {
           c = *p;
           switch (c)
           {
              case 'c':         ShowCount = 1;
                                PrintWhich = PRINTALL;
                                break;
              case 'd':         PrintWhich = PRINTREP;
                                break;
              case 'u':         PrintWhich = PRINTUNREP;
                                break;
              default:          for (q = p; *q && isdigit(*q); q++);
                                if (p == q || *q)
                                  {
                                    tell_usage();
                                    return(BAILOUT);
                                  }
                                else
                                  SkipFields = atoi(p);
                                break;
           }
         }
       else
         if (c == '+')
           {
              for (q = p; *q && isdigit(*q); q++);
              if (p==q || *q)
                {
                   tell_usage();
                   return(BAILOUT);
                }
              else
                SkipChars = atoi(p);
           }
         else                   /* this argv is not a switch */
           break;
     }
                      /* We're done with the switches */
   if (i < argc)
     {
       if (access(argv[i], 0))
         {
           fprintf(stderr,"%s: input file %s not found\n", myerror_pgm_name,
                                                           argv[i]);
           return(BAILOUT);
         }
       else
         InFile = argv[i++];
       if (i < argc)
         OutFile = argv[i++];
       if (i != argc)
         {
            tell_usage();
            return(BAILOUT);
         }
     }
  return(0);
}

/***********************************************************************/
/*  do_a_file()                                                        */
/***********************************************************************/
void do_a_file(char *fn)
{
   FILE *f;

   f = fopen(fn, "r");                         /* @A2c */
   if (!f)
     fprintf(stderr,"%s: access to %s denied.\n", myerror_pgm_name, fn);
   else
     compare(f);
}

/***********************************************************************/
/*  compare()                                                          */
/***********************************************************************/
void compare(FILE *f)
{
   char *p, *master, buf[MAXLINE+1];
   int matchcnt;

   master = NULL;
   matchcnt = 0;
   while (fgets(buf, MAXLINE, f))
     if (master == NULL)
       master = newmaster(buf);
     else
       {
         p = findstart(buf);
         if (strcmp(p, masterstart) == 0)
           matchcnt++;                /* we've found a matching line */
         else
           {
              p = (char *)NULL;
              switch (PrintWhich)
              {
                 case PRINTALL:      p = master;
                                     break;
                 case PRINTREP:      if (matchcnt)
                                       p = master;
                                     break;
                 case PRINTUNREP:    if (!matchcnt)
                                       p = master;
                                     break;
              }
              if (p)
                {
                  if (OutFilef == (FILE *)NULL)
                    {
                       OutFilef = openOutFile(OutFile);
                       if (!OutFilef)
                         return;
                    }
                  if (ShowCount)
                    printcount(OutFilef, matchcnt);
                  printline(OutFilef, p);
                }
              free(master);
              master = newmaster(buf);
              if (!master)
                {
                   myerror(ERROR_NOT_ENOUGH_MEMORY, "do_a_file",
                           itoa(strlen(buf), buf, 10));
                   return;
                }
              matchcnt = 0;
           }
       }
     p = (char *)NULL;
     if (PrintWhich == PRINTALL)
       p = master;
     else
       if (PrintWhich == PRINTREP)
         {
            if (matchcnt)
              p = master;
         }
       else
         if (!matchcnt)
           p = master;
     if (p)
       {
         if (OutFilef == (FILE *)NULL)
           {
              OutFilef = openOutFile(OutFile);
              if (!OutFilef)
                return;
           }
         if (ShowCount)
           printcount(OutFilef, matchcnt);
         printline(OutFilef, p);
       }


}

/***********************************************************************/
/*  newmaster()                                                        */
/***********************************************************************/
char *newmaster(char *buf)
{
   char *p;

   p = (char *)malloc(strlen(buf) + 1);
   if (p)
     {
       strcpy(p, buf);
       masterstart = findstart(p);
     }
   return(p);
}

/***********************************************************************/
/*  findstart()                                                        */
/***********************************************************************/
char *findstart(char *buf)
{
   char *p;
   int i;

   p = buf;
   for (i = 0; i < SkipFields; i++)
     {
        while (*p && (*p == TAB || *p == SPACE))
          p++;
        while (*p && *p != TAB && *p != SPACE)
          p++;
        while (*p && (*p == TAB || *p == SPACE))
          p++;
     }
   for (i = 0; i < SkipChars; i++)
     if (*p)
       p++;
   return(p);
}

/***********************************************************************/
/*  printcount()                                                       */
/***********************************************************************/
void printcount(FILE *f, int count)
{

   fprintf(f, "%4d ", count+1);
}

/***********************************************************************/
/*  printline()                                                        */
/***********************************************************************/
void printline(FILE *f, char *line)
{
   char *p;

   fprintf(f, "%s", line);
   for (p = line; *p; p++);
   if (p == line || *--p != '\n')
     fprintf(f,"\n");
}

/***********************************************************************/
/*  openOutFile()                                                      */
/***********************************************************************/
FILE *openOutFile(char *fn)
{
   char c, *p;
   FILE *f;
   int rc;

   if (!fn)
     return(stdout);
   for (p = fn; *p; p++);    /* find end of file spec */
   for  (--p; p != fn && !ISSEPARATER(*p); p--);  /* find end of path */
   if (p != fn)
     if (*(p-1) != (char)COLON)
       {
          c = *p;
          *p = '\0';
          rc = makepath(fn);
          if (rc)
            {
              fprintf(stderr,"%s: could not create path %s\n", myerror_pgm_name,
                                                               fn);
              return((FILE *)NULL);
            }
          *p = c;
       }
   f = fopen(fn, "w");                    /* @A2c */
   if (!f)
     fprintf(stderr,"%s: could not open file %s for output\n",
                                                          myerror_pgm_name, fn);
   return(f);
}

/***********************************************************************/
/*  tell_usage()                                                       */
/***********************************************************************/
void tell_usage()
{
  printf("uniq - Copyright IBM, 1991\n");
  printf("\nThe uniq compares adjacent lines in the input and writes just the\n");
  printf("first occurance to the output file (or stdout)\n");

  printf("\nSyntax\n");

  printf("\n        uniq [-cdu] [-NumFields] [+NumChars] [InFile [OutFile]]\n");
}
