static char sccsid[]="@(#)24	1.1  src/at/at.c, aixlike.src, aixlike3  9/27/95  15:43:48";
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
/* A1 09/16/92  grb  Make the return code explicit.                      */
/* A2 05/03/93  grb  Port to OS/2 v2                                     */
/* C3 02/22/95  gcw  Fix rc misassignment that resulted in segv          */
/*-----------------------------------------------------------------------*/
/*
   at  --  Runs commands at a later time

   SYNTAX

    at  [-c | -k | -s] [-m] Time [Today | Date] [Increment] [-l | -r job]

   The at command reads from standard input the names of commands to be run at
   a later time and allows you to specify when the commands should be run.

   The at command mails you all output from standard output and standard error
   for the scheduled commands, unless you redirect that output.  It also
   writes the job number and the scheduled time to standard error.

   When the at command is executed, it retains the current process environment.
   It does not retain open file descriptors, traps, and priority.

   You can use the at command if your name appears in the file
   /usr/adm/cron/at.allow.  If that file does not exist, the at command checks
   the file /usr/adm/cron/at.deny to determine if you should be denied access
   to the at command.  If neither file exists, only someone with root authority
   can submit a job.  The allow/deny files contain one user name per line.
   If (the at.allow file does exist, the root user's login nmae must be
   included in it for that person to be able to use the command.

   The required Time parameter can be one of the following:

      1. A number followed by an optional suffix.  The at command
         interprets one- and two-digit numbers as hours.  It interprets
         four digits as hours and minutes.  The NLTIME environment
         variable specifies the order of hours and minutes.  The default order
         is the hour followed by the minute.  You can also separate hours and
         minutes with a &gml. (colon).  The default order is
         Hour&gml.Minute.

      2. The at command also recognizes the following keywords as special
         Times:  noon, midnight, now, A for AM, P for PM, N for noon, and
         M for midnight.   Note that you can use the special word now only if
         you also specify a Date or an Increment.  Otherwise, the at command
         tells you: too late.  The NLTSTRS environment variable controls the
         additional keywords that the at command recognizes.

   You may specify the Date parameter as either a month name and a day number
   (and possibly a year number preceded by a comma), or a day of the week.
   The NLDATE environment variable specifies the order of the month name
   and day number (by default, month followed by day).  The NLLDAY environment
   variable specifies long day names; NLSDAY and NLSMONTH specify short day
   and month names.   (By default, the long name is fully spelled out; the
   short name is appreviated to two or more characters for weekdays, and
   three characters for months).  The at command recognizes two special
   "days", today and tomorrow by default.  (The NLSTRS environment variable
   specifies these special days.)  today is the default Date if the specified
   time is later than the current hour; tomorrow is the default if the time is
   earlier than the current hour.  If the specified month is less than the
   current month (and a year is not given), next year is the default year.
   The optional Increment can be one of the following:

      1. A + (plus sign) followed by a number and one of the following
         words: minute[s], hour[s], day[s], week[s], month[s], year[s] (or
         their non-English equivalents).

      2. The special word next followed by one of the following words:
         minute[s], hour[s], day[s], week[s], month[s], year[s] (or
         their non-English equivalents).

   FLAGS

      -c     Requests that the csh command be used for executing this job

      -k     Requests that the ksh command be used for executing this job

      -l     Reports your scheduled jobs

      -m     Mails a message to the user about the successful completion
             of the command

      -r Job... Removes Jobs previously scheduled by the at or batch commands,
             where job is the number assigned by the at or batch commands.
             If (you do not have root user authority (see the su command), you
             can only remove your own jobs.  The atrm command is available
             to the root user to remove jobs issued by other users or all
             jobs issued by a specific user.

      -s     Requests the bsh command (Bourne shell) be used for executing
             this job.

   EXAMPLES

      1. To schedule the command from the terminal, enter a command similar
         to one of the following:

               at 5 pm Friday uuclean
               Ctrl-D

               at now next week uuclean
               Ctrl-D

               at now + 2 days uuclean
               Ctrl-D

               at now + 2 days
               uuclean
               Ctrl-D

      2. To run the uuclean command at 3:00 in the afternoon on the 24th
         of January, enter any one of the following commands:

               echo uuclean | at 3:00 pm January 24

               echo uuclean | at 3pm Jan 24

               echo uuclean | at 1500 jan 24

      3. To have a job reschedule itself, invoke the at command from within
         the shell by including code similar to the following within
         the shell file:

               echo "ksh shellfile" | at now tomorrow

      4. To list the jobs you have sent to be run later, enter:

               at -l

      5. To cancel a job, enter:

               at -r ctw.635677200.a

         This cancels job ctw.635677200.a.  Use the at -l command to list
         the job numbers assigned to your jobs.

*/
#include <idtag.h>    /* package identification and copyright statement */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#define INCL_DOSMEMMGR
#define INCL_DOSSESMGR
#include <os2.h>
#include <fmf.h>
#include "at.h"

/*****************************   GLOBALS ***************************/

time_t attime;
char *badname = "BADNAME";
int function, shell, mail_output;
char *NLstring;
char **remlist;
char *cmdline_cmd;       /* Place to hold a command entered as part of the */
                         /* command line */


/*********************** FUNCTION PROTOTYPES ***********************/
int init(int argc, char **argv);
time_t extract_time(int *argno, int argc, char **argv);
time_t extract_date(int *argno, int argc, char **argv);
int daysinmonth(int mo);
int getNLwday(char *dayname);
int getshortNLmonth(char *moname);
int getlongNLmonth(char *moname);
int getlongNLmonth(char *moname);
int findNLname(char *name, int indx, char *dscr, char *evar);
char *getNLstring(int which);
char *extNLstring(char *evar, char *dstr, int indx);
int getNLDATE(void);
char *GetatjobsPath(void);
void makefilename(char *fn, char *atjobs_path, time_t tt);
void tell_usage(void);
void writestdin(FILE *f, char *cmdline_cmd);
void load_atd(char **argv);
int MakeThePath(char *path);
FILE *OpenOutputFile(time_t tt);
void getpath(char *path, char *fspec);
void remjobs(char **list);
int listjobs(void);

/****************************    MAIN    *****************************/
int main(int argc, char **argv)
{
   FILE *f;
   int rc;                                                      /* @A1a */

   rc = init(argc, argv);                                       /* @A1a */
   if (rc != BAILOUT)                                           /* @A1c */
     {
       if (function == ATLIST)
         {
            if (listjobs())      /* If there are any jobs */
              load_atd(argv);    /* make sure the daemon is loaded */
         }
       else
         if (function == ATDELETE)
           {
              if (remlist)
                remjobs(remlist);
           }
         else
           if (function == ATSET)
             {
#ifdef DEBUG
  printf("\nAlarm time is   %ld (%s)\n", attime, asctime(localtime(&attime)));
  printf("Current time is %ld\n", time(NULL));
#endif
               f = OpenOutputFile(attime);
               if (f)
                 {
                   writestdin(f, cmdline_cmd);
                   load_atd(argv);
                 }
               else                                             /* @A1a */
                 rc = BAILOUT;                                  /* @A1a */
             }
           else
             printf("Uhoh: didn't set the function flag in init()\n");
     }
  return(rc);                                                   /* @A1a */
}

/***************************************************************************/
/* Returns either 0 (for "OK, go on") or BAILOUT
   Side effects:
         function set to ATSET (set an alarm)
                         ATDELETE (delete an alarm)
                         ATLIST (list pending alarms)
         shell set to    USECSHELL (use the C shell)
                         USEKORNSHELL (use the Korn shell)
                         USEBOURNESHELL (use the Bourne shell)
         mail_output set to YES or NO
         (time_t)attime  set to the alarm time.
         cmdline_cmd     set to NULL, or the part of the scheduled command
                         that was entered on the command line.
*/
/***************************************************************************/

int init(int argc, char **argv)
{
   int argno, now_used, date_used, i;
   char c, *p;
   struct tm *ptm;
   time_t   default_date, curtime, curdate, ttime, date, increment;

   if (argc < 2)
     {
        tell_usage();
        return(BAILOUT);
     }

   now_used = NO;
   function = ATSET;
   mail_output = NO;
   shell = NOTSET;
   cmdline_cmd = NULL;
   argno = 1;
   p = argv[argno];

   while (*p == HYPHEN)
     {
        c = *++p;
        switch (c)
        {
           case 'c':           if (shell == NOTSET)
                                 shell = USECSHELL;
                               else
                                 {
                                    tell_usage();
                                    return(BAILOUT);
                                 }
                               break;

           case 'k':           if (shell == NOTSET)
                                 shell = USEKORNSHELL;
                               else
                                 {
                                    tell_usage();
                                    return(BAILOUT);
                                 }
                               break;

           case 's':           if (shell == NOTSET)
                                 shell = USEBOURNESHELL;
                               else
                                 {
                                    tell_usage();
                                    return(BAILOUT);
                                 }
                               break;

           case 'm':           mail_output = YES;
                               break;

           case 'r':           function = ATDELETE;
                               break;

           case 'l':           function = ATLIST;
                               break;
        }  /* end switch */
      if (++argno < argc)
        p = argv[argno];
    }  /* end while */
    if (function == ATLIST)
      return(OK);
    else
      if (function == ATDELETE)    /* make a list of the jobs to be deleted */
        {
           if (argno >= argc)
             {
                tell_usage();
                return(BAILOUT);
             }
           remlist = (char **)malloc((argc - argno + 1) * sizeof(char *));
           if (remlist)
             {
                for (i = 0; argno < argc; remlist[i++] = argv[argno++]);
                remlist[i] = NULL;
             }
           return(OK);
        }

/*  Function is ATSET */

/* Get Time (which is required) */

    if (argno >= argc)
      {
         tell_usage();
         return(BAILOUT);
      }
    curtime = time(NULL);        /* get seconds since 1980 */
    ptm = localtime(&curtime);       /* convert to date and time */
    ptm->tm_min = 0;             /* set time to midnight */
    ptm->tm_hour = 0;
    ptm->tm_sec = 0;
    curdate = mktime(ptm);       /* get seconds from 1980 at midnight */
    curtime = curtime - curdate; /* get seconds just today */
    if (stricmp(p, getNLstring(NOW)) == 0)
      {
        ttime = curtime;
        argno++;
        now_used = YES;
      }
    else
      {
         if (stricmp(p, getNLstring(NOON)) == 0)
           {
             ttime = NOONTIME;
             argno++;
           }
         else
           if (stricmp(p, getNLstring(MIDNIGHT)) == 0)
             {
               ttime = MIDNIGHTTIME;
               argno++;
             }
           else
             {
                ttime = extract_time(&argno, argc, argv);
                if (ttime == BADTIME)
                  {
                     tell_usage();
                     return(BAILOUT);
                  }
             }
      }        /* Done with the time */
/* get date */

    if (ttime >= curtime)
      default_date = curdate;
    else
      default_date = curdate + DAYSTIME;

    if (argno >= argc)
      if (now_used == YES)
        {
          tell_usage();
          return(BAILOUT);
        }
      else
        date = default_date;
    else
      {
        p = argv[argno];
        if ( (*p != PLUS) && (stricmp(p, getNLstring(NEXT)) != 0) )
          {
            date_used = YES;
            if (stricmp(p, getNLstring(TODAY)) == 0)
              {
                date = curdate;
                argno++;
              }
            else
              if (stricmp(p, getNLstring(TOMORROW)) == 0)
                {
                   date = curdate + DAYSTIME;
                   argno++;
                }
              else
                {   /* Month and day: may be in either order, and may not be there */
                  date = extract_date(&argno, argc, argv);
                  if (date == BADDATE)
                    {
                      tell_usage();
                      return(BAILOUT);
                    }
                }
          }
        else
          date = default_date;
      }
    date += ttime;
    ptm = localtime(&date);   /* convert to convenient format */
/* get increment */
    if (argno >= argc)
      {
        if (now_used == YES && date_used == NO)
          {
            tell_usage();
            return(BAILOUT);
          }
      }
    else
      {
        p = argv[argno];
        if (stricmp(p, getNLstring(NEXT)) == 0)
          {
             increment = 1;
             date = curdate + ttime;
             ptm = localtime(&date);
          }
        else
          if (*p++ != PLUS)
            {
               tell_usage();
               return(BAILOUT);
            }
          else
            {
               if (*p == '\0')
//                 p++;
                 increment = atoi(argv[++argno]);
               else
                 increment = atoi(p);
               if (increment < 0)
                 {
                    tell_usage();
                    return(BAILOUT);
                 }
             }
        p = argv[++argno];
        if ( (stricmp(p, getNLstring( MINUTE)) == 0) ||
             (stricmp(p, getNLstring( MINUTES)) == 0) )
          ptm->tm_min += increment;
        else
          if ( (stricmp (p, getNLstring(HOUR)) == 0) ||
               (stricmp (p, getNLstring(HOURS)) == 0) )
            ptm->tm_hour += increment;
          else
            if ( (stricmp (p, getNLstring(DAY)) == 0) ||
                 (stricmp (p, getNLstring(DAYS)) == 0) )
              ptm->tm_mday += increment;
            else
              if ( (stricmp (p, getNLstring(WEEK)) == 0) ||
                   (stricmp (p, getNLstring(WEEKS)) == 0) )
                ptm->tm_mday += (7*increment);
              else
                if ( (stricmp (p, getNLstring(MONTH)) == 0) ||
                     (stricmp (p, getNLstring(MONTHS)) == 0) )
                  ptm->tm_mon += increment;
                else
                  if ( (stricmp (p, getNLstring(YEAR)) == 0) ||
                       (stricmp (p, getNLstring(YEARS)) == 0) )
                    ptm->tm_year += increment;
                  else
                    {
                       tell_usage();
                       return(BAILOUT);
                    }
      }
   attime = mktime(ptm);
   if (++argno < argc)    /* They could have entered a command, too */
     cmdline_cmd = argv[argno];
   return(OK);
}

/***************************************************************************/
/* possible forms:                                                         */
/*      800 | 8:00 | 0800 | 08:00 | 8 pm | 8p | 8:00p | 8:00 pm            */
/***************************************************************************/
time_t extract_time(int *argno, int argc, char **argv)
{
   char *p, *q, ampm;
   int len, argn;
   time_t hr, min;

   ampm = '\0';
   argn = *argno;
   if (argn >= argc)
     return(BADTIME);
   p = argv[argn++];
   q = strchr(p, COLON);
   if (q)
     {
       *q++ = '\0';
       hr = atoi(p);
       min = atoi(q);
       p = q;
     }
   else
     {
        hr = atoi(p);
        if (hr > 100)
          {
             min = hr % 100;
             hr = hr / 100;
          }
        else
          min = 0;
     }
   for (; *p && isdigit(*p); p++);
   len = strlen(p);
   if (len)
     {
        q = getNLstring(AM);
        if (stricmp(p, q) == 0 || *p == (char)toupper(*q) ||
                                  *q == (char)toupper(*p))
          ampm = 'a';
        else
          {
             q = getNLstring(PM);
             if (stricmp(p, q) == 0 || *p == (char)toupper(*q) ||
                                       *q == (char)toupper(*p))
               ampm = 'p';
          }
     }
   if (!ampm)
     if (argn < argc)
       {
         p = argv[argn++];
         if (stricmp(p, getNLstring(AM)) == 0)
           ampm = 'a';
         else
           if (stricmp(p, getNLstring(PM)) == 0)
             ampm = 'p';
           else
             argn--;
       }
   if (ampm == 'p')
     hr += 12;
   *argno = argn;
   return((time_t)60*(60*hr +  min));
}

/***************************************************************************/
time_t extract_date(int *argno, int argc, char **argv)
{
   int order, arg, yearfollows, wday, day, mo, yr;
   time_t date;
   char *p;
   struct tm *ptm;

   arg = *argno;
   yearfollows = NO;
   date = time(NULL);        /* get current time and date */
   ptm = localtime(&date);
   order = getNLDATE();
   p = argv[arg];

   wday = getNLwday(p);
   if (wday != BADWDAY)              /* if time is Monday, Tuesday, etc. */
     {
        wday = wday - ptm->tm_wday;  /* how many days in the future? */
        if (wday < 0)
          wday += 7;
        date += (time_t)(wday * 86400);
        ptm = localtime(&date);
     }
   else                         /* otherwise date is mo day, yr or day mo, yr */
     {
       if (order == DAYFIRST)
         {
           day = atoi(p);
           if (day <= 0)
             return(BADDATE);
           for (p = argv[arg]; *p && *p != COMMA; p++);
           if (*p == COMMA)
             {
               yearfollows = YES;
               *p = '\0';
             }
           mo = getshortNLmonth(argv[arg]);
           if (mo == BADMONTH)
             {
               mo = getlongNLmonth(argv[arg]);
               if (mo == BADMONTH)
                 return(BADDATE);
             }
         }
       else
         {
            mo = getshortNLmonth(argv[arg]);
            if (mo == BADMONTH)
              mo = getlongNLmonth(argv[arg]);
            if (mo == BADMONTH)
              return(BADDATE);
            day = atoi(argv[++arg]);
            if (day <= 0)
              return(BADDATE);
            for (p = argv[arg]; *p && *p != COMMA; p++);
            if (*p == COMMA)
              yearfollows = YES;
         }
       if (yearfollows == YES)
         {
           if (!*++p)
             if (++arg >= argc)
               return(BADDATE);
             else
               p = argv[arg];
           yr = atoi(p);
           if (yr > 79 && yr < 100)
             yr += 1000;
           else
             if (yr < 1980)
               return(BADDATE);
         }
       if (day > daysinmonth(mo))
         return(BADDATE);
       ptm->tm_mday = day;
       ptm->tm_mon = mo;
       if (yearfollows == YES)
         ptm->tm_year = yr;
     }

   ptm->tm_sec = 0;
   ptm->tm_min = 0;
   ptm->tm_hour = 0;
   date = mktime(ptm);
   *argno = ++arg;
   return(date);
}

/***************************************************************************/
int daysinmonth(int mo)
{
     int modays[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

     if (mo < 0 || mo > 11)
       return(-1);
     else
       return(modays[mo]);
}

/***************************************************************************/
int getNLwday(char *dayname)
{
   int i;

   i = findNLname(dayname, 7, sdnames, "NLSDAY");
   if (i < 7)
     return(i);
   else
     {
       i = findNLname(dayname, 7, ldnames, "NLLDAY");
       if (i < 7)
         return(i);
     }
   return(BADWDAY);
}

/***************************************************************************/
int getshortNLmonth(char *moname)
{
   int i;

   i = findNLname(moname, 12, smnames, "NLSMONTH");
   if (i < 7)
     return(i);
   return(BADMONTH);
}

/***************************************************************************/
int getlongNLmonth(char *moname)
{
   int i;

   i = findNLname(moname, 12, lmnames, "");
   if (i < 12)
     return(i);
   return(BADMONTH);
}
/***************************************************************************/
/*  findNLname: returns the index of a string in a list of comma-seperated */
/*  strings.  name is the string to be found.  indx is the number of       */
/*  strings to compare.  dscr is the default string to search.  evar is    */
/*  the name of an environment varible that might contain the search       */
/*  string.                                                                */
/*  Returns the index of the string if found, the passed value indx        */
/*  otherwise.                                                             */
/***************************************************************************/
int findNLname(char *name, int indx, char *dscr, char *evar)
{
   char *p, *q, *wk, *varlist;   /* longer than any possible day name */
   int i, len;

   p = NULL;
   if (evar)
     p = getenv(evar);
   if (!p)
     p = dscr;
   if (!p)
     return(indx);
   varlist = p;
   for ( len = 0, i = 0; i < indx; i++)
     {
        for (; *p && isspace(*p); p++);    /* skip leading blanks */
        for (q = p; *p && *p != ','; *p++);
        len = (len > (p-q)) ? len : p-q;
        if (*p)
          p++;                             /* skip the comma */
     }
   wk = (char *)malloc(len + 1);
   if (wk)
     {
       for (p = varlist, i = 0; i < indx; i++)
         {
            for (; *p && isspace(*p); p++);    /* skip leading blanks */
            for (q = wk; *p && *p != ','; *q++ = *p++);
            *q = '\0';
            if (stricmp(name, wk) == 0)
              break;
            if (*p)
              p++;                             /* skip the comma */
         }
       free(wk);
     }
   return(i);
}

/***************************************************************************/
char *getNLstring(int which)
{
   if (NLstring && NLstring != badname)
     free(NLstring);
   switch (which)
   {
     case TODAY :      return(extNLstring("NLSTSTRS", ttnames, 0)); break;
     case TOMORROW:    return(extNLstring("NLSTSTRS", ttnames, 1)); break;
     case NOON:        return(extNLstring("NLSTSTRS", ttnames, 2)); break;
     case MIDNIGHT:    return(extNLstring("NLSTSTRS", ttnames, 3)); break;
     case NOW:         return(extNLstring("NLSTSTRS", ttnames, 4)); break;
     case NEXT:        return(extNLstring("NLTUNITS", utnames, 0)); break;
     case MINUTE:      return(extNLstring("NLTUNITS", utnames, 1)); break;
     case MINUTES:     return(extNLstring("NLTUNITS", utnames, 2)); break;
     case HOUR:        return(extNLstring("NLTUNITS", utnames, 3)); break;
     case HOURS:       return(extNLstring("NLTUNITS", utnames, 4)); break;
     case DAY:         return(extNLstring("NLTUNITS", utnames, 5)); break;
     case DAYS:        return(extNLstring("NLTUNITS", utnames, 6)); break;
     case MONTH:       return(extNLstring("NLTUNITS", utnames, 7)); break;
     case MONTHS:      return(extNLstring("NLTUNITS", utnames, 8)); break;
     case YEAR:        return(extNLstring("NLTUNITS", utnames, 9)); break;
     case YEARS:       return(extNLstring("NLTUNITS", utnames, 10)); break;
     case WEEK:        return(extNLstring("NLTUNITS", utnames, 11)); break;
     case WEEKS:       return(extNLstring("NLTUNITS", utnames, 12)); break;
     case AM:          return(extNLstring("NLTMISC",  mtnames, 0)); break;
     case PM:          return(extNLstring("NLTMISC",  mtnames, 1)); break;
     case ZULU:        return(extNLstring("NLTMISC",  mtnames, 2)); break;
   }
   return(badname);
}

/***************************************************************************/
char *extNLstring(char *evar, char *dstr, int indx)
{
   char *p, *q;
   int i;

   p = NULL;
   if (evar)
     p = getenv(evar);
   if (!p)
     p = dstr;
   if (!p)
     return(badname);
   for (i = 0; i < indx; i++)
     {
        for (; *p && isspace(*p); p++);
        for (; *p && *p != COMMA; p++);
        if (*p)
          p++;
     }
   for (; *p && isspace(*p); p++);
   q = p;
   for (i = 0; *p && *p != COMMA; p++);
   NLstring = malloc(p-q+1);
   if (NLstring)
     {
       for (p = q, q = NLstring; *p && *p != COMMA; *q++ = *p++);
       *q = '\0';
     }
   else
     NLstring = badname;
   return(NLstring);
}


/***************************************************************************/
int getNLDATE()
{
   char *p;

   p = getenv("NLDATE");
   if (p && *p)
     return(atoi(p));
   else
     return(DAYLAST);
}

/***************************************************************************/
/***************************************************************************/
int listjobs()
{
   char *atjobs_path;
   char pattern[CCHMAXPATH];
   char fn[CCHMAXPATH], *p;
   int jobcnt, attrib;

   jobcnt = 0;
   atjobs_path = GetatjobsPath();
   if (atjobs_path)
     {
        strcpy(pattern, atjobs_path);
        strcat(pattern, "\\*.*");
        if (fmf_init(pattern, FMF_ALL_FILES, FMF_NO_SUBDIR) == 0)
          while (fmf_return_next(fn, &attrib) == 0)
            {
               for (p = fn; *p; p++);
               for (--p; *p != BACKSLASH && *p != COLON && p != fn; p--);
               printf("%s\n", ++p);
               jobcnt++;
            }
     }
   if (jobcnt == 0)
     printf("No jobs\n");
   return (jobcnt);
}
/***************************************************************************/
/***************************************************************************/
void remjobs(char **list)
{
   char fn[CCHMAXPATH];
   char *atjobs_path;
   int  i, rc;

   atjobs_path = GetatjobsPath();
   if (atjobs_path)
     {
        for (i = 0; list[i]; i++)
          {
            strcpy(fn, atjobs_path);
            strcat(fn, "\\");
            strcat(fn, list[i]);
            rc = remove(fn);
            if (rc)
              if (errno == ENOENT)
                printf("Job %s not found\n", list[i]);
              else
                if (errno == EACCES)
                  printf("Access to job %s was denied.\n", list[i]);
          }
     }
}
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
FILE *OpenOutputFile(time_t tt)
{
   char fn[CCHMAXPATH];
   char *atjobs_path;
   FILE *f;
   struct stat buf;

   f = (FILE *)NULL;
   atjobs_path = GetatjobsPath();
   if (atjobs_path)
     do
       {
         makefilename(fn, atjobs_path, tt);
         if (stat(atjobs_path, &buf))
           if (MakeThePath(atjobs_path) == BADPATH)
             return(NULL);
         if (stat(fn, &buf) == 0)    /* file already exists */
           tt++;                     /* try next time */
         else
           f = fopen(fn, "w");
       }
         while (f == NULL);
   return(f);
}

/***************************************************************************/
/***************************************************************************/
void makefilename(char *fn, char *atjobs_path, time_t tt)
{
   char *p, *q;

       strcpy(fn, atjobs_path);
       strcat(fn, "\\");
       p = fn + strlen(fn);
       sprintf(p, "%010lu", tt);
       p = fn + strlen(fn) + 1;
       q = p - 1;
       *p-- = *q--;
       *p-- = *q--;
       *p = *q;
       *q = PERIOD;
}
/***************************************************************************/
/* return either the default directory for queued jobs (AtJobDir) or the        */
/* directory in the Daemon's shared segment (AtSharedSeg) or the one in the        */
/* environment variable atjobs.
/***************************************************************************/
char *GetatjobsPath()
{
#ifdef I16                                                          /* @A2a */
   SEL sel;
#else                                                               /* @A2a */ 
   PVOID sel;                                                       /* @A2a */ 
#endif                                                              /* @A2a */ 
   USHORT rc;
   char *p;
   char envv[CCHMAXPATH+8];

#ifdef I16                                                          /* @A2a */
   rc = DosGetShrSeg(AtSharedSeg, &sel);
#else                                                               /* @A2a */
   rc = DosGetNamedSharedMem(&sel, AtSharedSeg, (ULONG)PAG_READ);   /* @C3  */
#endif
   if (rc == 0)
     {
#ifdef I16                                                          /* @A2a */
       p = (char *)MAKEP(sel, 0);
#else                                                               /* @A2a */ 
       p = sel;                                                     /* @A2a */ 
#endif                                                              /* @A2a */ 
       strcpy(envv, atjobs);
       strcat(envv, "=");
       strcat(envv, p);
       putenv(envv);
       return(p);
     }
   else
     {
       p = getenv(atjobs);
       if (p!= NULL)
         return(p);
       else
         {
            putenv(AtJobEnvStr);
            return(AtJobDir);
         }
     }
}
/***************************************************************************/
/***************************************************************************/
void writestdin(FILE *f, char *cmdline_stuff)
{
   char linebuff[256];
   char *lb;

   if (cmdline_stuff && (strlen(cmdline_stuff) < 254))
     {
       sprintf(linebuff, "%s\n", cmdline_stuff);
       fputs(linebuff, f);
     }
   do
     {
        linebuff[255] = 0;
        lb = fgets(linebuff, 255, stdin);
        if (lb)
          fputs(lb, f);
     }
       while (lb);
   fclose(f);
}
/***************************************************************************/
/* atd.exe is on the same path as at.exe
/***************************************************************************/
void load_atd(char **argv)
{
   USHORT rc;
#ifdef I16                                                           /* @1a */
   SEL    sel;
#else                                                                /* @1a */ 
   PVOID sel;                                                        /* @1a */ 
#endif                                                               /* @1a */
   STARTDATA sd;                                                   
#ifdef I16                                                           /* @1a */ 
   USHORT sid, pid;                                                  
#else                                                                /* @1a */ 
   ULONG  sid;                                                       /* @1a */ 
   PID    pid;                                                       /* @1a */ 
#endif                                                               /* @1a */ 
   char fqDaemonName[CCHMAXPATH];
   char fqIconName[CCHMAXPATH];
   char *p;
   struct stat buf;

#ifdef I16                                                           /* @1a */ 
   rc = DosGetShrSeg(AtSharedSeg, &sel);
#else                                                                /* @1a */ 
   rc = DosGetNamedSharedMem(&sel, AtSharedSeg, (ULONG)PAG_READ);    /* @1a */ 
#endif                                                               /* @1a */ 
   if (rc)                  /* couldn't access shared segment */
     {
       getpath(fqDaemonName, argv[0]);
       strcat(fqDaemonName, AtDaemonFile);
       if (stat(fqDaemonName, &buf))
         {
           rc = DosSearchPath((USHORT)7, (PSZ)"PATH", (PSZ)AtDaemonFileName,
                              (PBYTE)fqDaemonName, (USHORT)CCHMAXPATH);
           if (rc)
            {
               printf("Could not locate program %s\n", AtDaemonFileName);
               return;
            }
         }
       p = fqIconName;
       getpath(fqIconName, argv[0]);
       strcat(fqIconName, AtIconFile);
       if (stat(fqIconName, &buf))
         {
            rc = DosSearchPath((ULONG)7, (PSZ)"DPATH", (PSZ)AtIconFileName,
                               (PBYTE)fqIconName, (ULONG)CCHMAXPATH);
            if (rc)
              p = NULL;
         }
       sd.Length = (USHORT)50;
       sd.Related = (USHORT)0;        /* new process will be unrelated */
       sd.FgBg = (USHORT)1;           /* It will be started in background */
       sd.TraceOpt = (USHORT)0;       /* No tracing */
       sd.PgmTitle = AtDaemonProgramTitle;             /* This is the title */
       sd.PgmName = fqDaemonName;              /* This is the program to load */
       sd.PgmInputs = NULL;           /* No program inputs */
       sd.TermQ = NULL;               /* No queue name */
/*---*/
       sd.Environment = NULL;         /* No special environment block */
       sd.InheritOpt = (USHORT)1;     /* Inherit at's environment */
/*---*/
       sd.SessionType = (USHORT)2;    /* Vio application in window */
       sd.IconFile = p;               /* name of icon file */
       sd.PgmHandle = (ULONG)0;
       sd.PgmControl = (USHORT)4;     /* start invisible */
       sd.InitXPos  = (USHORT)0;
       sd.InitYPos  = (USHORT)0;
       sd.InitXSize = (USHORT)0;
       sd.InitYSize = (USHORT)0;
       rc = DosStartSession(&sd, &sid, &pid);
       if (rc )
         printf("Could not load at daemon.  Rc = %d\n", rc);
     }
}


/***************************************************************************/
/***************************************************************************/
int MakeThePath(char *path)
{
   char *p,  c;
   struct stat buf;

   p = path + 1;
   if (*p == COLON)
     {
        p++;
        if (*p == BACKSLASH || *p == SLASH)
          p++;
     }
   while (*p)
     {
       for (; *p && *p != BACKSLASH && *p != SLASH; p++);
       c = *p;
       *p = '\0';
       mkdir(path);
       *p = c;
       if (*p)
         p++;
     }
   if (stat(path, &buf))
     return(BADPATH);
   return(PATHOK);
}
/***************************************************************************/
/* getpath() - extracts drive and path from a  file spec                   */
/***************************************************************************/
void getpath(char *path, char *fspec)
{
   char *p;

   strcpy(path, fspec);
   for (p = path; *p; p++);
   for (--p; p != path && *p != BACKSLASH && *p != COLON; p--);
   *p = '\0';
}
/***************************************************************************/
/***************************************************************************/
void tell_usage(void)
{
   printf("at - Copyright IBM, 1992\n");
   printf("Usage:\n");
   printf(" at  [-c | -k | -s] [-m] Time [Today | Date] [Increment] [-l | -r job]\n");
   printf("\nFLAGS\n");
   printf("    -l     Reports your scheduled jobs\n");
   printf("    -r Job... Removes jobs previously scheduled by the at command,\n");
   printf("          where Job is the number assigned by the at command.\n");
   printf("    -c, -k, -m and -s have no effect.\n");
}
