static char sccsid[]="@(#)53	1.1  src/kill/kill.c, aixlike.src, aixlike3  9/27/95  15:44:51";
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

/* The kill command sends a signal (by default, the sigterm signal) to a   */
/* running process.  This default action narmally stops processes that     */
/* ignore or do not catch the signal.  If you want to stop a process,      */
/* specify the ProcessID (process identification number, or PID).  The     */
/* shell reports the PID of each process that is running in the background */
/* (unless you start more than one process in a pipeline, in which case    */
/* the shell reports the number of the last process).  You can also use    */
/* the ps command to find the process ID number of commands.               */
/*                                                                         */
/* If you are not a root user, you must own the process you want to stop.  */
/* When operating as a root user, you can stop any process.                */
/*                                                                         */
/* Flags                                                                   */
/*    0 Sends the signal to all processes having a PID equal to the PID    */
/*      of the sender (except those with PIDs 0 and 1).                    */
/*                                                                         */
/*   -1 Sends the signal to all processes with a PID equal to the effective*/
/*      user ID of the sender (except to 0 and 1).                         */
/*                                                                         */
/*   ProcessID Sends the signal to all processes whose process-group       */
/*      number is equal to the absolute value of the ProcessID             */
/*                                                                         */
/*   -l Lists signal names                                                 */
/*                                                                         */
/*   -Signal The signal can be the signal number or the signal name.       */
/*                                                                         */
/* ----------------------------------------------------------------------  */
/*                                                                         */
/*  OK, that's the canon.  Now here's how it works in OS/2:                */
/*                                                                         */
/*  0 and -1 flags are not supported.                                      */
/*                                                                         */
/*  Only a subset of signals is supported:                                 */
/*                                                                         */
/*      KillProcess                                                        */
/*      SIGBREAK                                                           */
/*      SIGINTR                                                            */
/*      SIGUSER1                                                           */
/*      SIGUSER2                                                           */
/*      SIGUSER3                                                           */
/*                                                                         */
/*  The -l flag reports these, along with their numbers.                   */
/*                                                                         */
/*  The default signal is KillProcess.                                     */
/*                                                                         */
/***************************************************************************/
/*                                                                         */
/* Modification history                                                    */
/*                                                                         */
/* A1  09.16.92  grb  Make return code explicit.                           */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */


#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define INCL_BASE
#include <os2.h>


#define MAXPID   100
#define BAIL_OUT  -1
#define OK         0

struct sig_list  {
                    int sigval;           /* the number the user can type  */
                    int real_sigval;      /* the number we use internally */
                    char *signame;        /* the name the user can use */
                 };

#define SIG_KILL 9
struct sig_list SigList[] = {
                               { 9,  SIG_KILL,      "KILL"  },
#ifdef I16
                               {30,  SIG_PFLG_A,    "USR1"  },
                               {30,  SIG_PFLG_A,    "FLAGA" },
                               {31,  SIG_PFLG_B,    "USR2"  },
                               {31,  SIG_PFLG_B,    "FLAGB" },
                               {32,  SIG_PFLG_C,    "USR3"  },
                               {32,  SIG_PFLG_C,    "FLAGC" },
#endif
                               {0,   0,             ""},
                            };

struct kill_list {
                    int sigval;
                    PID pid[MAXPID];
                  };

int  parse_command_line(int argc, char ** argv, struct kill_list *klp);
void cide(struct kill_list *klp);
void tell_usage(void);
void showsigs(void);
int  edit_signal(char *txt, struct kill_list *klp);



int main(int argc, char **argv)                                 /* @A1c */
{

   struct kill_list KillList;

   if (parse_command_line(argc, argv, &KillList) != BAIL_OUT)
     cide(&KillList);
   else
     return(BAIL_OUT);                                          /* @A1a */
   return(0);                                                   /* @A1a */

}



int parse_command_line(int argc, char **argv, struct kill_list *klp)
{
   int i, kli;
   char *p;

   if (argc <= 1)
     {
       tell_usage();
       return(BAIL_OUT);
     }

   if (strcmp(argv[1], "0") == 0 || strcmp(argv[1], "-1") == 0)
     {
       printf("0 and -1 flags are not suported in OS/2 version\n");
       tell_usage();
       return(BAIL_OUT);
     }

   i = 1;
   if (*argv[i] == '-')         /* if a signal was specified */
     {
       if (strcmp(argv[i], "-l") == 0)
         {
            showsigs();
            return(BAIL_OUT);
         }
       if (edit_signal(argv[i], klp) == BAIL_OUT)
         return(BAIL_OUT);
       else
         if (++i == argc)       /* signal specified, but no pid */
           {
              tell_usage();
              return(BAIL_OUT);
           }
     }
   else
     klp->sigval = SIG_KILL;           /* default signal to send */

   for (kli = 0; i < argc && kli < MAXPID; i++)
     {
       for (p = argv[i]; *p; p++)
         if (!isdigit(*p))
           {
              printf("kill:  bad PID - %s\n", argv[i]);
              return(BAIL_OUT);
           }
       klp->pid[kli] = atoi(argv[i]);
       if (klp->pid[kli] == 0)
         printf("kill:  %s is not a valid PID\n", argv[i]);
       klp->pid[++kli] = 0;
     }
   if (kli == MAXPID)
     {
       printf("kill: too many PIDs requested.\n");
       return(BAIL_OUT);
     }

   return(OK);
}


void showsigs()
{
    struct sig_list *slp;

    printf("SIG NAME          SIG NUMBER\n\n");
    for (slp = SigList; slp->sigval; slp++)
       printf("%-17s %d\n", slp->signame, slp->sigval);
}


int edit_signal(char *spec, struct kill_list *klp)
{
  char *p;
  struct sig_list *slp;
  int tstval;

  p = spec + 1;
  if (isdigit(*p))
    {
      for (++p; *p && isdigit(*p); p++);
      if (!*p)
        tstval = atoi(p);
      else
        {
           printf("kill: invalid  signal number %s\n", p);
           return(BAIL_OUT);
        }
      for (slp = SigList; slp->sigval; slp++)
         if (tstval == slp->sigval)
            {
               klp->sigval = slp->real_sigval;
               return(OK);
            }
      printf("kill: Signal number %d can not be sent. Try kill -l\n", tstval);
    }
  else
    {
      for (slp = SigList; slp->sigval; slp++)
        if (stricmp(p, slp->signame) == 0)
          {
             klp->sigval = slp->real_sigval;
             return(OK);
          }
      printf("kill: Signal %s can not be sent.  Try kill -l\n", p);
    }

  return(BAIL_OUT);
}

void tell_usage()
{
    printf("kill - Copyright IBM, 1990, 1992\n");
    printf("Usage:\n");
#ifndef I16
    printf("    kill pid\n");
#else
    printf("    kill [-l] [-signal] pid\n");
    printf("Flags:\n");
    printf("    -l      lists the signals that may be sent.\n");
    printf("    -signal specifies the signal.  The signal may be entered as a\n");
    printf("            number or by name:\n");
    showsigs();
    printf("\nIf the -signal flag is not entered, the KILL signal is sent.\n");
#endif
    printf("\npid is the process ID of the process to receive the signal.\n");
}

void cide(struct kill_list *klp)
{
    int pidi, rc;

    if (klp->sigval == SIG_KILL)
      for (pidi = 0; klp->pid[pidi] && pidi < MAXPID; pidi++)
        if ( (rc = DosKillProcess(1, klp->pid[pidi])) )
          {
             printf("kill: DosKillProcess(1, %d) failed.  rc = %d\n",
                     klp->pid[pidi], rc);
             return;
          }
        else
          printf("KillProcess sent to process %d\n", klp->pid[pidi]);
#ifdef I16
    else
      if (klp->sigval == SIG_PFLG_A || klp->sigval == SIG_PFLG_B ||
          klp->sigval == SIG_PFLG_C)
        for (pidi = 0; klp->pid[pidi] && pidi < MAXPID; pidi++)
          if ((rc=DosFlagProcess(klp->pid[pidi],1,klp->sigval-SIG_PFLG_A,0)))
            {
               printf("kill: DosFlagProcess(%d, 1, %d, 0) failed. rc = %d\n",
                      klp->pid[pidi], rc);
               return;
            }
          else
            printf("User flag %c sent to process %d\n",
                   'A' + klp->sigval - SIG_PFLG_A, klp->pid[pidi]);
#endif
}
