static char sccsid[]="@(#)98	1.2  src/touch/touch.c, aixlike.src, aixlike3  10/30/95  13:56:24";
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

/* touch - updates the access and modification times of a file. */

/* Usage:

        touch [-a | -m] [-c] [-f] [s] [time] Directory|File Directory|File ...

   time is in the format  MMDDHHmmyy, where

       MM is the month number (required)
       DD is the day number   (required)
       HH is the hour based on a 24-hour clock (required)
       mm is the minute       (required if yy is specified (defaults to 0))
       yy is the last 2 digits of the year (optional, defaults to current year)

   The touch command updates the access and modification times of each
   File or Directory names to the one specified on the command line.  If
   you do not specify Time, the touch command uses the current time.  If
   you specify a file that does not exist, the touch command creates a
   file with that name unless you request otherwise with the -c flag.

   The format of the data and time are specified in the config.sys.

   The return code from the touch command is the number of files for
   which the times could not be successfully modified (including files
   that did not exist and were not created).

   Flags:

        -a       Changes only the access time.
        -c       Does not create the file if it does not already exist
        -f       Attempts to force the touch in spite of read and write
                   permissions on a file.
        -m       Changes only the modification time.
        -s       Apply changes to all matching files in subtrees (OS/2 only)
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>
#include "fmf.h"


#define YES 1
#define NO  0
#define BAILOUT -1
#define ALLFILES FILE_DIRECTORY | FILE_SYSTEM | FILE_HIDDEN

struct settime  {
                  int smonth;
                  int sday;
                  int syear;
                  int shour;
                  int smin;
                  int stwosec;
                };

char *optstring = "acfms";
char *changeTimeTo = NULL;
int mo, day, yr, hour, min;
int subtreesrch, onlyModifyTime, onlyAccessTime, forceTouch, createNewFile;

/* FUNCTION PROTOTYPES */

int init(int argc, char **argv);
void getchgToTime(struct settime *st, char *clt);
void tell_usage(void);
int do_a_file(char *filename, int attribute, struct settime *st);
int isunambiguous(char *filename);
                                    /* Parse command line */
int  getopt(int argc, char *argv[], char *opstring);

extern int optind;                  /* data exported by getopt() */
extern char *optarg;


/* ------------------------------------------------------------------------- */
/*    main                                                                   */
/* ------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
   int attrib, failures, i;
   char filename[CCHMAXPATH];
   struct settime chgToTime;

   failures = 0;
   if ( (i = init(argc, argv)) != BAILOUT)
     if (i > 0)
       {
         getchgToTime(&chgToTime, changeTimeTo);
         for (; i < argc; i++)
           {
            if (isunambiguous(argv[i])) /* if this is an unambiguous file name*/
              failures += do_a_file(argv[i],0,&chgToTime);
            else
              if (fmf_init(argv[i],
//                           FILE_SYSTEM | FILE_HIDDEN | FILE_DIRECTORY,
                           FMF_ALL_DIRS_AND_FILES,
                           subtreesrch) == NO_ERROR)
                {
                  while (fmf_return_next(filename, &attrib) == NO_ERROR)
                    if ( !(attrib & FILE_DIRECTORY) )
                      failures += do_a_file(filename, attrib, &chgToTime);
                  fmf_close();
                }
              else
                failures++;
           }
       }
   return(failures);
}


/* ------------------------------------------------------------------------- */
/*    init                                                                   */
/* ------------------------------------------------------------------------- */
int init(int argc, char *argv[])
{
#ifdef I16
    char c;
#else
    int c;
#endif
    char *p;

    createNewFile = YES;         /* set cmd line option defaults */
    forceTouch = NO;
    subtreesrch = NO;
    onlyModifyTime = NO;
    onlyAccessTime = NO;

#ifdef I16
    while ( (c = (char)getopt(argc, argv, optstring)) != EOF)
#else
    while ( (c = getopt(argc, argv, optstring)) != EOF)
#endif
    {
        switch (toupper(c))
        {
            case 'C':    createNewFile = NO;
               break;
            case 'F':    forceTouch = YES;
               break;
            case 'S':    subtreesrch = YES;
               break;
            case 'A':    if (onlyModifyTime == NO)
                           onlyAccessTime = YES;
                         else
                           {
                              tell_usage();
                              return(BAILOUT);
                           }
               break;
            case 'M':    if (onlyAccessTime == NO)
                           onlyModifyTime = YES;
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

    mo = day = 0;                 /* initialize work areas for date */
    hour = min = yr = -1;
    if ((argc - optind) >= 2)     /* if time might have been specified */
      {
         p = argv[optind];                          // Minimum MMDDHH
         if ( (strlen(p) > 5) && (strlen(p) < 11) ) // maximum MMDDHHmmyy
           {
             for (;isdigit(*p); p++);
             if (!*p)
               {
                  p = argv[optind];
                  mo = 10*(*p++ - '0'); mo += (*p++ - '0');
                  if (mo > 0 && mo < 13)
                    {
                       day = 10*(*p++ - '0'); day += (*p++ - '0');
                       if (day > 0 && day < 32)
                         {
                            hour = 10*(*p++ - '0'); hour += (*p++ - '0');
                            if (hour >= 0 && hour < 24)
                              if (!*p)
                                changeTimeTo = argv[optind++];
                              else
                                {
                                   min = 10*(*p++ - '0'); min += (*p++ - '0');
                                   if (min >= 0 && min < 60)
                                     if (!*p)
                                       changeTimeTo = argv[optind++];
                                     else
                                       {
                                         yr = 10*(*p++ - '0'); yr += (*p - '0');
                                         if (yr >= 0 && yr < 100)
                                           {
                                             yr -= 80;
                                             changeTimeTo = argv[optind++];
                                           }
                                       }
                                }
                         }
                    }
               }
           }
      }

    if (optind == argc)
      {
        tell_usage();
        return(BAILOUT);
      }
    else
      return(optind);
}


/* ------------------------------------------------------------------------- */
/*    getchgToTime                                                           */
/* ------------------------------------------------------------------------- */
void getchgToTime(struct settime *st, char *clt)
{
   DATETIME dt;

   if (yr == -1)
     DosGetDateTime(&dt);

   if (clt == NULL)    /* if time wasn't specified on the command line */
     {
       st->smonth  = dt.month;
       st->sday    = dt.day;
       st->shour   = dt.hours;
       st->smin    = dt.minutes;
       st->syear   = dt.year - 1980;
       st->stwosec = dt.seconds / 2;
     }
   else
     {
       st->smonth = mo;
       st->sday = day;
       st->shour = hour;
       if (min == -1)
         st->smin = 0;
       else
         st->smin = min;
       if (yr == -1)
         st->syear = dt.year - 1980;
       else
         st->syear = yr;
       st->stwosec = 0;
     }
}


/* ------------------------------------------------------------------------- */
/*    do_a_file                                                              */
/* ------------------------------------------------------------------------- */
int do_a_file(char *filename, int attrib, struct settime *st)
{
  int   rc;
  HDIR handle1 = 1;
  HFILE fhandle;
#ifdef I16
  USHORT action_taken = 2;
  FILESTATUS fs;
  FILEFINDBUF ffb;
  int cnt = 1;
#else
  ULONG action_taken = 2;
  FILESTATUS3 fs;
  FILEFINDBUF3 ffb;
  ULONG cnt = 1;
#endif

#ifdef I16
  if ((rc = DosFindFirst(filename, &handle1, ALLFILES, &ffb,
                            sizeof(FILEFINDBUF), &cnt, 0l)) != NO_ERROR)
#else
  if ((rc = DosFindFirst(filename, &handle1, ALLFILES, &ffb,
                            sizeof(FILEFINDBUF), &cnt, 1l)) != NO_ERROR)
#endif
    if (createNewFile && isunambiguous(filename))
      {
        if ( (rc = DosOpen(filename, &fhandle, &action_taken, 0l, 0x0020,
                                             0x0011, 0x0012, 0)) != NO_ERROR )
          {
//            printf("rc from DosOpen: %d\n",rc);
            return(1);    /* Tried to create file, failed */
          }
        DosClose(fhandle);  /* Tried to create file, succeeded */
        handle1 = 1;        /* reinitialize the handle            @za */
#ifdef I16
        if ((rc = DosFindFirst(filename, &handle1, ALLFILES, &ffb,
                            sizeof(FILEFINDBUF), &cnt, 0l)) != NO_ERROR)
#else
        if ((rc = DosFindFirst(filename, &handle1, ALLFILES, &ffb,
                            sizeof(FILEFINDBUF), &cnt, 1l)) != NO_ERROR)
#endif
          return(1);     /* Something weird must have happened */
      }
    else
      return(1);         /* doesn't exist, they don't want it created */

  if ( (ffb.attrFile & FILE_READONLY) && (forceTouch == NO) )
    return(1);

  fs.fdateCreation.day       = ffb.fdateCreation.day;
  fs.fdateCreation.month     = ffb.fdateCreation.month;
  fs.fdateCreation.year      = ffb.fdateCreation.year;
  fs.ftimeCreation.hours     = ffb.ftimeCreation.hours;
  fs.ftimeCreation.minutes   = ffb.ftimeCreation.minutes;
  fs.ftimeCreation.twosecs   = ffb.ftimeCreation.twosecs;
  fs.fdateLastAccess.day     = ffb.fdateLastAccess.day;
  fs.fdateLastAccess.month   = ffb.fdateLastAccess.month;
  fs.fdateLastAccess.year    = ffb.fdateLastAccess.year;
  fs.ftimeLastAccess.hours   = ffb.ftimeLastAccess.hours;
  fs.ftimeLastAccess.minutes = ffb.ftimeLastAccess.minutes;
  fs.ftimeLastAccess.twosecs = ffb.ftimeLastAccess.twosecs;
  fs.fdateLastWrite.day      = ffb.fdateLastWrite.day;
  fs.fdateLastWrite.month    = ffb.fdateLastWrite.month;
  fs.fdateLastWrite.year     = ffb.fdateLastWrite.year;
  fs.ftimeLastWrite.hours    = ffb.ftimeLastWrite.hours;
  fs.ftimeLastWrite.minutes  = ffb.ftimeLastWrite.minutes;
  fs.ftimeLastWrite.twosecs  = ffb.ftimeLastWrite.twosecs;
  fs.cbFile                  = ffb.cbFile;
  fs.cbFileAlloc             = ffb.cbFileAlloc;
  fs.attrFile                = ffb.attrFile;
  if (onlyModifyTime == NO)
    {
      fs.fdateLastAccess.day     = st->sday;
      fs.fdateLastAccess.month   = st->smonth;
      fs.fdateLastAccess.year    = st->syear;
      fs.ftimeLastAccess.hours   = st->shour;
      fs.ftimeLastAccess.minutes = st->smin;
      fs.ftimeLastAccess.twosecs = st->stwosec;
    }
  if (onlyAccessTime == NO)
    {
      fs.fdateLastWrite.day     = st->sday;
      fs.fdateLastWrite.month   = st->smonth;
      fs.fdateLastWrite.year    = st->syear;
      fs.ftimeLastWrite.hours   = st->shour;
      fs.ftimeLastWrite.minutes = st->smin;
      fs.ftimeLastWrite.twosecs = st->stwosec;
    }
#ifdef I16
  if ( (rc = DosSetPathInfo(filename, 1, (PBYTE)&fs, sizeof(FILESTATUS),
                                                           0, 0l)) != NO_ERROR )
#else
  if ( (rc = DosSetPathInfo(filename, 1, (PBYTE)&fs, sizeof(FILESTATUS3),
                                                           0)) != NO_ERROR )
#endif
    return(1);

  return(0);
}

/* ------------------------------------------------------------------------- */
/*    isunambiguous                                                          */
/* ------------------------------------------------------------------------- */
int isunambiguous(char *fn)
{
   char *p;

   for (p = fn; *p && *p!='?' && *p!='*'; p++);
   if (*p)
     return(NO);
   else
     return(YES);
}

/* ------------------------------------------------------------------------- */
/*    tell_usage                                                             */
/* ------------------------------------------------------------------------- */
void tell_usage()
{
  printf("        Copyright IBM, 1990\n");
  printf("touch - updates the access and modification times of a file\n");

  printf("Usage:\n");

  printf("     touch [-a | -m] [-c] [-f] [-s] [time] Directory|File Directory|File ...\n");

  printf("\ntime is in the format  MMDDHHmmyy, where\n");

  printf("\n    MM is the month number (required)\n");
  printf("    DD is the day number   (required)\n");
  printf("    HH is the hour based on a 24-hour clock (required)\n");
  printf("    mm is the minute       (required if yy is specified (defaults to 0))\n");
  printf("    yy is the last 2 digits of the year (optional, defaults to current year)\n");

  printf("\nThe touch command updates the access and modification times of each\n");
  printf("File or Directory names to the one specified on the command line.  If\n");
  printf("you do not specify Time, the touch command uses the current time.  If\n");
  printf("you specify a file that does not exist, the touch command creates a\n");
  printf("file with that name unless you request otherwise with the -c flag.\n");
  printf("The return code from the touch command is the number of files for\n");
  printf("which the times could not be successfully modified (including files\n");
  printf("that did not exist and were not created).\n");

  printf("\nFlags:\n");

  printf("     -a       Changes only the access time.\n");
  printf("     -c       Does not create the file if it already exists\n");
  printf("     -f       Attempts to force the touch on readonly files.\n");
  printf("     -m       Changes only the modification time.\n");
  printf("     -s       Apply changes to all matching files in subtrees (OS/2 only)\n");

}
