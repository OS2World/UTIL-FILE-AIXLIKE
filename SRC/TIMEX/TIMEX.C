static char sccsid[]="@(#)96	1.1  src/timex/timex.c, aixlike.src, aixlike3  9/27/95  15:46:23";
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
/* @1 05.09.91 Peter Schwaller found bugs in the was commands were found,*/
/*             and suggested I just load cmd.exe                         */
/*-----------------------------------------------------------------------*/

/* timex  -  reports elapsed time for a command.

   Usage:

             timex [-o] [-s] [-p] Command
*/

/* The timex command reports, in seconds, the elapsed time for a command.
   In UNIX it does lots of other neat things, too, but those neat things
   are not supported by OS/2.  For compatibility, I support the same flags,
   but they don't do anything:

       -o         Reports total number of blocks read or written and
                  total characters transferred by a command and its
                  children.

       -p         Lists process accounting records for a command, and all
                  its children.

       -s         Reports total system activity during the execution of the
                  command.
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>

#define BAILOUT  -1
#define MAXCMDLINE 260
#define YES 1
#define NO  0

DATETIME dt1, dt2;
char *cmdname;
char *cmdexe = "CMD.EXE";
char args[MAXCMDLINE];
char *opstring = "ops";
char ObjNameBuf[100];
RESULTCODES ReturnCodes;

/* ----------------------------------- */
/*        Function Prototypes          */
/* ----------------------------------- */
int init(int argc , char **argv);  /* look at command line; build command line
                                      for the command to be executed. */
void tell_usage(void);             /* print usage information */
int hasblanks(char *string);       /* returns YES if string contains spaces */
                                   /* Parse command line */
                                   /* Try to find the command in PATH */
int hasextension(char *cmd);       /* see if command name has extension */
int  getopt(int argc, char *argv[], char *opstring);

extern int optind;                 /* data exported by getopt() */
extern char *optarg;
                                   /* routine to format error information */
void myerror(int rc, char *area, char *details);
extern char *myerror_pgm_name;     /* data imported by myerror()  */

/* ------------------------------------------------------------------------- */
/*    main                                                                   */
/* ------------------------------------------------------------------------- */

main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
{
   int  rc, cmdindx;
   clock_t start = 0;


   myerror_pgm_name = "timex";
   if ( (cmdindx = init(argc, argv)) != BAILOUT )
       {
             start = clock();
             rc = DosExecPgm( ObjNameBuf,
                              sizeof(ObjNameBuf),
                              0,                   /* execute synchronously */
                              args,
                              0,
                              &ReturnCodes,
                              cmdexe);
             if (rc != NO_ERROR)
               myerror(rc, "DosExecPgm", cmdname);
             printf("Elapsed time: %5.2f seconds\n",
                                           ((float)clock() - start) / CLK_TCK);
             return(ReturnCodes.codeResult);
       }
   else
     return(BAILOUT);
}


/* ------------------------------------------------------------------------- */
/*    init                                                                   */
/* ------------------------------------------------------------------------- */

int init(int argc, char *argv[])
{
   int i, c;
   char *opstart, *p;

   for (i = 0; i < MAXCMDLINE; args[i++] = '\0');

   while ( (c = getopt(argc, argv, opstring)) != EOF)
   {
     switch (c)
     {
        case 'o':
        case 'p':
        case 's':
           break;
        default:       tell_usage();
                       return(BAILOUT);
           break;
     }
   }

   i = optind;
   if (i < argc)
     {
       cmdname = argv[i];
       strcpy(args, cmdexe);           /* We are executing CMD.EXE */
       opstart = args + strlen(args) + 1;
       strncpy(opstart, "/C ", 3);     /* Its parms are /C */
       opstart += 3;                   /* followed by the program name */
       for (p = argv[i]; *p; *opstart++ = *p++);
       *opstart++ = ' ';
       i++;
     }
   else
     {
        tell_usage();
        return(BAILOUT);
     }

   while (i < argc)                    /* then followed by the its args */
     {
       if (hasblanks(argv[i]))  /* if there are embedded blanks */
         *opstart++ = '"';      /*   quote it */
       for (p = argv[i]; *p; *opstart++ = *p++);  /* copy to arg string */
       if (hasblanks(argv[i]))  /* if there are embedded blanks */
         *opstart++ = '"';      /*   quote it */
       *opstart++ = ' ';        /* seperate args by spaces */
       i++;
     }
   *opstart++ = '\0';
   *opstart   = '\0';

   return(YES);
}

/* ------------------------------------------------------------------------- */
/*    hasblanks                                                              */
/* ------------------------------------------------------------------------- */

int hasblanks(char *s)
{
   char *p;
   for (p = s; *p; p++)
     if (*p == ' ')
       return(YES);
   return(NO);
}


/* ------------------------------------------------------------------------- */
/*    tell_usage                                                             */
/* ------------------------------------------------------------------------- */

void tell_usage()
{
   printf("          Copyright IBM, 1990, 1991\n");
   printf("timex  -  reports elapsed time for a command.\n");

   printf("\nUsage:\n");

   printf("\n          timex [-o] [-s] [-p] Command cmdarg cmdarg ...\n");

   printf("\nThe timex command reports, in seconds, the elapsed time for a command.\n");
   printf("In UNIX it does lots of other neat things, too, but those neat things\n");
   printf("are not supported by OS/2.  For compatibility, I support the same flags,\n");
   printf("but they don't do anything:\n");

   printf("\n    -o         Reports total number of blocks read or written and\n");
   printf("               total characters transferred by a command and its\n");
   printf("               children.");

   printf("\n    -p         Lists process accounting records for a command, and all\n");
   printf("               its children.\n");

   printf("\n    -s         Reports total system activity during the execution of the\n");
   printf("               command.\n");
}
