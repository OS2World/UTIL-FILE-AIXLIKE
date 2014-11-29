static char sccsid[] = "@(#)91	1.1  src/what/what.c, aixlike.src, aixlike3  1/8/96  23:29:38";

/******************************************************************************/
/*                                                                            */
/*  Module:  what.c                                                           */
/*                                                                            */
/*  Purpose:  AIXLIKE what searches files for SCCS keywords.  Please see the  */
/*  what manpage for complete details.  This implementation is based on the   */
/*  AIX 3.2.5 what manpage.                                                   */
/*                                                                            */
/*  Author: George C. Wilson                                                  */
/*          GCWILSON AT AUSTIN                                                */
/*          gcwilson@dss1.austin.ibm.com                                      */
/*                                                                            */
/*                                                                            */
/*  The following code is property of the International Business Machines     */
/*  Corporation.                                                              */
/*                                                                            */
/*  Copyright International Business Machines Corporation, 1996.              */
/*  All rights reserved.                                                      */
/*                                                                            */
/******************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int optind;			/* for getopt()                       */
int silent = 0;                         /* silent flag                        */
 
char init_pattern[] = "@(#)";		/* initial pattern in keyword string  */
#define term_charset  "\">\n\\\0"       /* terminal character set             */

int _Optlink parse_args(int, char *[]); /* forwards                           */
int _Optlink what_files(int, char *[]);
int _Optlink what_stdin(void);


/******************************************************************************/
/*                                                                            */
/*  Call parse_args to parse the args.  If they're good, process the list of  */
/*  files, or stdin if the user supplied no file list.                        */
/*                                                                            */
/*  Parameteres                                                               */
/*    In:  argc--command line argument count                                  */
/*         argv--command line argument list                                   */
/*    Out: None                                                               */
/*                                                                            */
/*  Return Values                                                             */
/*    0:  At least one SCCS keyword was found                                 */
/*    1:  No SCCS keywords found                                              */
/*    2:  Bad argument                                                        */
/*                                                                            */
/*  Side Effects                                                              */
/*    Writes error message to stderr if parse_args returns non-zero.          */
/*    Others indirectly.  See other functions.                                */
/*                                                                            */
/******************************************************************************/

int main(int argc, char *argv[]) {

  int rc;				/* program return code                */

  rc = parse_args(argc, argv);
  
  if(rc == 0) {
    if (optind < argc)
      rc = what_files(argc, argv);
    else
      rc = what_stdin();
  }
  else {
    fprintf(stderr, "Usage: what [-s] [File] [File] . . .\n");
  } 

  exit(rc);
}


/******************************************************************************/
/*                                                                            */
/*  Parse the args using getopt.  The "s" argument is the only good one.  If  */
/*  it's given, set silent to true, otherwise return a value of 2.            */
/*                                                                            */
/*  Parameteres                                                               */
/*    In:   argc--command line argument count                                 */
/*          argv--command line argument list                                  */
/*    Out:  None                                                              */
/*                                                                            */
/*  Return Values                                                             */
/*    0:  Arguments were good                                                 */
/*    2:  Arguments were bad                                                  */
/*                                                                            */
/*  Side Effects                                                              */
/*    Sets silent to 1 if the "s" switch is given.                            */
/*    Calls getopt, which increments optind.                                  */
/*                                                                            */
/******************************************************************************/

int _Optlink parse_args(int argc, char *argv[]) {

  int rc = 0;				/* return code                        */
  int opt;				/* option from getopt                 */

  while ((opt = getopt(argc, argv, "s")) != EOF) {
    switch (opt) {
      case 's':
        silent = 1;
        break;
      default:
        rc = 2;
        break;
    }
  }
  return(rc);
}


/******************************************************************************/
/*                                                                            */
/*  For each filename given on the command line, redirect it to stdin.  If    */
/*  the filename can't be found, print an error to stderr.                    */
/*                                                                            */
/*  Parameteres                                                               */
/*    In:   argc--command line argument count                                 */
/*          argv--command line argument list                                  */
/*    Out:  None                                                              */
/*                                                                            */
/*  Return Values                                                             */
/*    0:  At least one call to what_stdin() was successful                    */
/*    1:  No calls to what_stdin() were successful                            */
/*                                                                            */
/*  Side Effects                                                              */
/*    Uses and increments optind.                                             */
/*    Writes found filenames to stdout.                                       */
/*    Writes unfound file errors to stderr.                                   */
/*    Redirects stdin.                                                        */
/*                                                                            */
/******************************************************************************/

int _Optlink what_files(int argc, char *argv[]) {

  int rc = 1;				/* return code                        */

  while (optind < argc) {
    if ((freopen(argv[optind], "rb", stdin)) != NULL) {
      printf("%s:\n", argv[optind]);
      rc &= what_stdin();
    }
    else {
      perror(argv[optind]);
    }
    optind++;
  }
  return(rc);
}


/******************************************************************************/
/*                                                                            */
/*  Read from stdin until EOF looking for SCCS keywords.  Write out the       */
/*  the keyword string until a terminating charcter is found.  Locals         */
/*  are ordered for register optimization.                                    */
/*                                                                            */
/*  Parameteres                                                               */
/*    In:   None                                                              */
/*    Out:  None                                                              */
/*                                                                            */
/*  Return Values                                                             */
/*    0:  Found at least one SCCS keyword string                              */
/*    1:  Found no SCCS keyword strings                                       */
/*                                                                            */
/*  Side Effects                                                              */
/*    Reads data from stdin.                                                  */
/*    Uses silent flag.                                                       */
/*    Writes SCCS keyword strings to stdout.                                  */
/*    Increments stdin file pointer.                                          */
/*                                                                            */
/******************************************************************************/

int _Optlink what_stdin(void) {

  register int   found_init_char    = 0;	/* found initial SCCS keyword */
                                                /*   pattern's 1st char       */
  register int   c; 				/* char from stdin            */
  register char *init_pattern_p;                /* pointer to initial pattern */
                                                /*   positon                  */
  register int   found_init_pattern = 0;	/* found initial SCCS keyword */
                                                /*   pattern                  */
           int   init_pattern_len   = 0;	/* initial pattern position   */                                                /*   pointer's string length  */
           int   rc                 = 1;	/* return code                */

  while ((c = getchar()) != EOF) {
    if (found_init_char) {
      if (init_pattern_len > 0) {
        if (c == *init_pattern_p) {
          init_pattern_p++;
          init_pattern_len--;
        }
        else {
          found_init_char = 0;
        }
      }
      else {
        if(!found_init_pattern) {
          putchar('\t');
          found_init_pattern = 1;
        }
        if (!strchr(term_charset,c)) {
          putchar(c);
        }
        else {
          putchar('\n');
          rc = 0;
          if (silent) {
            break;
          }
          found_init_char = 0;
          found_init_pattern = 0;
        }
      }
    }
    else {
      if (c == *init_pattern) {
        init_pattern_p = init_pattern + 1;
        init_pattern_len = strlen(init_pattern_p);
        found_init_char = 1;
      }
    }
  }
  return(rc);
}
