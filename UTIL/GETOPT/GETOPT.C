static char sccsid[]="@(#)37	1.2  util/getopt/getopt.c, aixlike.src, aixlike3  1/8/96  19:25:57";
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

/* -------------------------------------------------------------------------- */
/* getopt()                                                                   */
/*                                                                            */
/* The getopt() function is a command line parser.  It returns the next       */
/* option character in argv that matches an option character in opstring.     */
/*                                                                            */
/* The argv argument points to an array of argc+1 elements containing argc    */
/* pointers to character strings followed by a null pointer.                  */
/*                                                                            */
/* The opstring argument points to a string of option characters; if an       */
/* option character is followed by a colon, the option is expected to have    */
/* an argument that may or may not be seperated from it by white space.  The  */
/* external variable optarg is set to point to the start of the option        */
/* argument on return from getopt().                                          */
/*                                                                            */
/* The getopt() function places in optind the argv index of the next argument */
/* to be processed.  The system initializes the external variable optind to   */
/* 1 before the first call to getopt().                                       */
/*                                                                            */
/* When all options have been processed (that is, up to the first nonoption   */
/* argument), getopt() returns EOF.  The special option "--" may be used to   */
/* delimit the end of the options; EOF will be returned, and "--" will be     */
/* skipped.                                                                   */
/*                                                                            */
/* The getopt() function returns a question mark (?) when it encounters an    */
/* option character not included in opstring.  This error message can be      */
/* disabled by setting opterr to zero.  Otherwise, it returns the option      */
/* character that was detected.                                               */
/*                                                                            */
/* If the special option "--" is detected, or all options have been           */
/* processed, EOF is returned.                                                */
/*                                                                            */
/* No errors are defined.                                                     */
/* -------------------------------------------------------------------------- */
/* OS/2 implementation (by G.R. Blair)                                        */
/*                                                                            */
/* Options are marked by either a minus sign (-) or a slash (/).              */
/* -------------------------------------------------------------------------- */
/* Maintenance History:                                                       */
/* @1  02/17/92  grb  Peter Schwaller pointed out that the code walks off the */
/*                    end of argv in some circumstances.                      */
/*                                                                            */
/* -------------------------------------------------------------------------- */

#ifdef AIX_CORRECT
#include <stdlib.h>
#endif /* AIX_CORRECT */
#include <stdio.h>                   /* for EOF */
#include <string.h>                  /* for strchr() */


/* static (global) variables that are specified as exported by getopt() */
char *optarg = NULL;    /* pointer to the start of the option argument  */
int   optind = 1;       /* number of the next argv[] to be evaluated    */
int   opterr = 1;       /* non-zero if a question mark should be returned
                           when a non-valid option character is detected */
int   _aix_correct_set = -1;

#define COLON        ':'
#define QUESTIONMARK '?'
#define SLASH        '/'
#define MINUS        '-'
#ifdef AIX_CORRECT
#define ISOPT(p) (((*p == SLASH) && (!_aix_correct_set)) \
                  || (*p == MINUS))
#else
#define ISOPT(p) ((*p == SLASH) || (*p == MINUS))
#endif /* AIX_CORRECT */
#define BADOPTERROR(c)  optind++, return( opterr ? QUESTIONMARK : c )

int getopt(int argc, char *argv[], char *opstring)
{
  static char *indpos = NULL;
  char c, *p, *q;


#ifdef AIX_CORRECT
  if (_aix_correct_set == -1) {
    _aix_correct_set = (getenv("AIX_CORRECT") != NULL);
  }
#endif /* AIX_CORRECT */
  p = NULL;
  if (indpos)
    if (*(++indpos))
      p = indpos;
  if (p == NULL)
    {
      if (optind >= argc)
        {
          indpos = NULL;
          return(EOF);
        }
      p = argv[optind++];
      if (!ISOPT(p))         /* If the next argv[] is not an option */
        {                    /*   there can be no more options      */
          --optind;            /* we point to the current arg once we're done*/
          optarg = NULL;
          indpos = NULL;
          return(EOF);
        }
                             /* check for special end-of-flags markers */
      if ((strcmp(p, "-") == 0) || (strcmp(p, "--") == 0))
        {
           optarg = NULL;
           indpos = NULL;
           return(EOF);
        }
      p++;
    }
  if (*p == COLON)
     return(opterr ? QUESTIONMARK : c);
  else
     if ((q = strchr(opstring, *p)) == 0)
        {
           optarg = NULL;
           indpos = NULL;
           return(opterr ? QUESTIONMARK : c);
        }
     else
        {
          if (*(q + 1) != COLON)
             {
                optarg =  NULL;        /* no argument follows the option */
                indpos = p;
             }
          else
             {
                if (*(p + 1) != '\0')  /* Arg follows.  Is it in this argv? */
                   optarg = ++p;            /* Yes, it is */
                else
                  if (optind < argc)                               /* @1a */
                    optarg = argv[optind++]; /*No, it isn't--it is in the next*/
                  else                                             /* @1a */
                    {              /* or it doesn't exist at all      @1a */
                       optarg = NULL;                              /* @1a */
                       return(opterr ? QUESTIONMARK : c);          /* @1a */
                    }                                              /* @1a */
                indpos = NULL;
             }
          return(*q);
        }
}
