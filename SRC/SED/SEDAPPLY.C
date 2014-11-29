static char sccsid[]="@(#)82	1.1  src/sed/sedapply.c, aixlike.src, aixlike3  9/27/95  15:45:52";
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
/* @1 06.07.91 Change (s) command would not allow you to insert something*/
/*             between the last wildcard character and a terminating EOL */
/* @2 08.05.91 Trap D on change when "to" is null                        */
/* @4 03/17/92 BRANCH_TO_END wasn't working right: it should terminate   */
/* @5 03/28/92 Never-ending search to process the a flag correctly       */
/* @7 03/17/92 Line-matching should be based on pattern_space.           */
/* @8 06/29/92 Escape sequences were being passed to fputs() after change*/
/* @9 07/01/92 Conditional branch was not working                        */
/*@10 07/20/93 Some search patterns would make change() trap             */
/*@11 06/22/94 Pattern of ^something was not testing the ^               */
/*@12 09/05/95 Match $ addr                                              */
/*-----------------------------------------------------------------------*/

/* sedapply.c  -  routines to execute commands against the input line */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sed.h"
#include <grep.h>
#define  INCL_ERRORS
#include <os2.h>

#define NEWLINE 10
#define TAB 8

// extern char *readarea;
char areadarea[MAXLINE];
extern int suppress_output;

/* Here's a very important global: it is referred to only within this */
/* source module:                                                     */

char *hold_space = NULL;


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*   applycmds  --  if the line selected is addressed, apply the command   */
/*                  to it.                                                 */
/*                                                                         */
/*   The big case structure is the meat of sed.                            */
/*                                                                         */
/*   Returns pointer to whatever buffer is being used for the pattern      */
/*   space by the time we get done.                                        */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* line is a malloc'd space.  We can increase it with realloc if needed */

char *applycmds(struct subcmd **scpp, char *line, FILE *f, long *linenop)
{
    struct subcmd **lscpp;
    char *pattern_space, *p, *q, c;
    char *atext;                                                /* @5a */
    FILE *fi;
    int in_subgroup, chg, linechanged, hs_size;                 /* @9c */

    in_subgroup = NO;
    pattern_space = line;
    atext = NULL;                                               /* @5a */
    lscpp = scpp;
    chg = NO;                                                   /* @9a */
    while (*lscpp)
      {
/*        chg = NO;                                                @9d */
/*         if (select_line(*lscpp, line, in_subgroup, *linenop) == YES)  @7d */
         if (select_line(*lscpp, pattern_space, in_subgroup, f, *linenop) == YES)  /* @7a */ /* @12c */
          {
            switch (cmd_table[abs((*lscpp)->cmd_idx)].cmd_tag)
            {
//             case PUT_TEXT:                                             @5d
               case IPUT_TEXT:
                                 fputs((*lscpp)->args[0], stdout);     /* @5a7*/
//                                 if (!suppress_output)
//                                   fputs(pattern_space, stdout);
//                                 free(pattern_space);
//                                 pattern_space = (char *)malloc(
//                                                  strlen((*lscpp)->args[0]) + 1);
//                                 strcpy(pattern_space, (*lscpp)->args[0]);
                                 lscpp++;
               break;

             case PUT_TEXT:
//             case IPUT_TEXT:                                         /*@5d */
                               atext = (char *)malloc(                 /*@5a3*/
                                                 strlen((*lscpp)->args[0]) + 1);
                               strcpy(atext, (*lscpp)->args[0]);
                               lscpp++;
             break;

             case UNCONDITIONAL_BRANCH:
                               lscpp = find_branch_address(lscpp, scpp,
                                                                  &in_subgroup);
             break;

             case REPLACE:
                               chg = YES;
                               *pattern_space = '\0';  /* erase the pattern space */
                               if ((*lscpp)->saddr[1].val.pch == NULL ||
                                   (*lscpp)->status == READY)
                                 fputs((*lscpp)->args[0], stdout);
                               lscpp++;
             break;

             case DELETE:
                               chg = YES;
                               *pattern_space = '\0';
                               lscpp++;
             break;

             case DELETE_FIRST_LINE:
                               chg = YES;
                               for (p = pattern_space; *p && *p != '\n'; p++);
                               if (*p)
                                 p++;
                               for (q = pattern_space; *p; *q++ = *p++);
                               *q = '\0';
                               lscpp++;
             break;

             case REPLACE_WITH_HOLD_DATA:
                               chg = YES;
                               if (hold_space == NULL)
                                 *pattern_space = '\0';
                               else
                                 {
                                   hs_size = strlen(hold_space);
                                   if ( (pattern_space =
                                         realloc(pattern_space, hs_size + 1))
                                         == NULL)
                                     {
                                        myerror(ERROR_NOT_ENOUGH_MEMORY,
                                                "applycmds",
                                                "replace pattern");
                                        return((char *)BAIL_OUT);   /* @4c */
                                     }
                                   strcpy(pattern_space, hold_space);
                                 }
                               lscpp++;
             break;

             case APPEND_WITH_HOLD_DATA:
                               chg = YES;
                               if (hold_space != NULL)
                                 {
                                   hs_size = strlen(pattern_space) +
                                             strlen(hold_space) + 1;
                                   if ( (pattern_space = (char *)
                                         realloc(pattern_space, hs_size + 1))
                                         == NULL)
                                     {
                                        myerror(ERROR_NOT_ENOUGH_MEMORY,
                                                "realloc",
                                                "append hold");
                                        return((char *)BAIL_OUT);   /* @4c */
                                     }
                                   strcat(pattern_space, hold_space);
                                 }
                               lscpp++;
             break;

             case REPLACE_HOLD_DATA:
                               if (hold_space != NULL)
                                 free(hold_space);
                               hold_space = (char *)malloc(
                                                     strlen(pattern_space) + 1);
                               if (hold_space == NULL)
                                 {
                                    myerror(ERROR_NOT_ENOUGH_MEMORY,
                                            "malloc",
                                            "replace hold");
                                    return((CHAR *)BAIL_OUT);   /* @4c */
                                 }
                               strcpy(hold_space, pattern_space);
                               lscpp++;
             break;

             case APPEND_TO_HOLD_DATA:
                               hs_size = strlen(pattern_space) + 1;
                               if (hold_space == NULL)
                                 {
                                   if (hold_space = (char *)malloc(hs_size))
                                     *hold_space = '\0';
                                 }
                               else
                                 {
                                   hs_size += strlen(pattern_space);
                                   hold_space = (char *)realloc(hold_space,
                                                                hs_size);
                                 }
                               if (hold_space == NULL)
                                 {
                                    myerror(ERROR_NOT_ENOUGH_MEMORY,
                                            "malloc",
                                            "replace hold");
                                    return((char *)BAIL_OUT);   /* @4c */
                                 }
                               strcat(hold_space, pattern_space);
                               lscpp++;
             break;


             case WRITE_WITH_HEX:
                               dump_with_expansion(pattern_space);
                               lscpp++;
             break;

             case WRITE_AND_REFRESH:
                               if (!suppress_output)
                                 fputs(pattern_space, stdout);
                               free(pattern_space);
                               chg = NO;
                               if (fgets(areadarea, MAXLINE, f))
                                 {
                                   *linenop += 1;
                                   pattern_space =
                                           (char *)malloc(strlen(areadarea) + 1);
                                   if (pattern_space == NULL)
                                     {
                                       myerror(ERROR_NOT_ENOUGH_MEMORY,
                                               "malloc", "write/refresh");
                                       return((CHAR *)BAIL_OUT);    /* @4c */
                                     }
                                   strcpy(pattern_space, areadarea);
                                   lscpp++;
                                 }
                               else
                                 return(pattern_space);
             break;

             case APPEND_TO_PATTERN:
                               chg = YES;
                               if (fgets(areadarea, MAXLINE, f))
                                 {
                                   *linenop += 1;
                                   pattern_space =
                                           (char *)realloc(pattern_space,
                                           strlen(areadarea) +
                                           strlen(pattern_space) + 1);
                                   if (pattern_space == NULL)
                                     {
                                       myerror(ERROR_NOT_ENOUGH_MEMORY,
                                               "realloc", "write/refresh");
                                       return((char *)BAIL_OUT);   /* @4c */
                                     }
                                   strcat(pattern_space, areadarea);
                                   lscpp++;
                                 }
                               else
                                 return(pattern_space);
             break;

             case WRITE_TO_STDOUT:
                               fputs(pattern_space, stdout);
                               lscpp++;
             break;

             case WRITE_FIRST_LINE_TO_STDOUT:
                               for (p = pattern_space; *p && *p != '\n'; p++);
                               if (*p)
                                 {
                                    c = *++p;
                                    *p = '\0';
                                 }
                               fputs(pattern_space, stdout);
                               *p = c;
                               lscpp++;
             break;

             case BRANCH_TO_END:
                               while(*lscpp)
                                 lscpp++;
                               in_subgroup = NO;
                               *linenop = -1L;            /* @4a */
             break;

             case PUT_TEXT_FROM_FILE:
                               fi = (FILE *)(*lscpp)->args[0];
                               rewind(fi);
                               while (fgets(areadarea, MAXLINE, fi))
                                 fputs(areadarea, stdout);
                               lscpp++;
             break;

             case CHANGE:                                             /* @9c */
                               if ( (linechanged = change(*lscpp, &pattern_space))
                                                                   == BAIL_OUT)
                                 return((char *)BAIL_OUT);   /* @4c */
                               else
                                 if (linechanged == YES)              /* @9a */
                                   chg = YES;
                               lscpp++;
             break;

             case CONDITIONAL_BRANCH:
                               if (chg == YES)
                                 {                                    /* @9a */
                                   lscpp = find_branch_address(lscpp, scpp,
                                                                  &in_subgroup);
                                   chg = NO;                          /* @9a */
                                 }                                    /* @9a */
                               else
                                 lscpp++;
             break;

             case APPEND_PATTERN_TO_FILE:
                                fi = (FILE *)(*lscpp)->args[0];
                                fputs(pattern_space, fi);
                                lscpp++;
             break;

             case EXCHANGE_WITH_HOLD_DATA:
                                p = pattern_space;
                                if (hold_space == NULL)
                                  {
                                    hold_space = (char *)malloc(10);
                                    *hold_space = '\0';
                                  }
                                pattern_space = hold_space;
                                hold_space = p;
                                chg = YES;
                                lscpp++;
             break;

             case CHARACTER_REPLACE:
                                if (replace_chars(*lscpp, pattern_space))
                                  chg = YES;
                                lscpp++;
             break;

             case PUT_LINE_NUMBER:
                                printf("%ld\n", *linenop);
                                lscpp++;
             break;

             case LABEL_MARK:
                                lscpp++;
             break;

             case BEGIN_SUBGROUP:
                                in_subgroup++;
                                lscpp++;
             break;

             case END_SUBGROUP:
                                in_subgroup--;
                                lscpp++;
             break;
            }   /* end switch  and while */
          }    /* end if line was selected */
        else  /* line not selected */
          if (cmd_table[abs((*lscpp)->cmd_idx)].cmd_tag == BEGIN_SUBGROUP)
            lscpp = find_subgroup_end(lscpp);
          else
            lscpp++;
      }  /* end while more subcommands to process */

   if (!suppress_output)                       /* moved from process_file()@5a*/
     fputs(pattern_space, stdout);                                       /*@5a*/
   if (atext)                                                            /*@5a*/
     {                                                                   /*@5a*/
       fputs(atext, stdout);                                             /*@5a*/
       atext = NULL;                                                     /*@5a*/
     }                                                                   /*@5a*/
   return(pattern_space);
}



/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  select_line  --  see if the current line is addressed by this command. */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* int select_line(struct subcmd *scp, char *line, int in_subgroup, long lineno) */
int select_line(struct subcmd *scp, char *line, int in_subgroup, FILE *f, long lineno)  /* @12c */
{
   int selected;

   selected = NO;
   if (in_subgroup == YES)
     return(YES);    /* line already selected for this subgroup */
   if (scp->saddr[0].type == NO_ADDRESS)
     return(YES);    /* 0 addresses specified: select all lines */

   if (scp->status == IN_RANGE)
     if (scp->saddr[1].type == LINENO_ADDRESS)
       if (lineno >= scp->saddr[1].val.lineno)
         {
           scp->status = READY;
           selected = YES;
         }
       else
         selected = YES;
     else if(scp->saddr[1].type == LASTLINE_ADDRESS) {		/* @12a */
       if(feof(f)) {						/* @12a */
         scp->status = READY;					/* @12a */
         selected = YES;					/* @12a */
       }							/* @12a */
     else							/* @12a */
       selected = YES;						/* @12a */
     }								/* @12a */
     else
       if (matches_reg_expression(line, scp->saddr[1].val.pch))
         {
           scp->status = READY;
           selected = YES;
         }
       else
         selected = YES;
   else                  /* status is not IN_RANGE */
     if (scp->saddr[0].type == LINENO_ADDRESS)
       if (scp->saddr[0].val.lineno == lineno)
         {
           if (scp->saddr[1].type != NO_ADDRESS)
             if (scp->saddr[1].type == LINENO_ADDRESS &&
                 scp->saddr[1].val.lineno <= lineno)
               scp->status = READY;
             else
               scp->status = IN_RANGE;
           else
             scp->status = READY;
           selected = YES;
         }
       else
         {
//           if (scp->saddr[1].type != NO_ADDRESS &&                 @zd
           if (scp->saddr[1].type == LINENO_ADDRESS &&             //@za
               scp->saddr[0].val.lineno < lineno &&
               scp->saddr[1].val.lineno >= lineno)
             {
                scp->status = IN_RANGE;
                selected = YES;
             }
         }
     else if(scp->saddr[0].type == LASTLINE_ADDRESS) {		/* @12a */
       if(feof(f)) {						/* @12a */
         scp->status = READY;					/* @12a */
         selected = YES;					/* @12a */
       }							/* @12a */
     }								/* @12a */
     else
       if (matches_reg_expression(line, scp->saddr[0].val.pch))
         {
           if (scp->saddr[1].type != NO_ADDRESS)
             {
               scp->status = IN_RANGE;
               if (scp->saddr[1].type == LINENO_ADDRESS &&
                   scp->saddr[1].val.lineno <= lineno)
                 scp->status = READY;
             }
           else
             scp->status = READY;
           selected = YES;
         }
   if (scp->cmd_idx < 0)    /* remember: logic can be reverse */
     selected ^= YES;

   return(selected);
}


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  find_branch_address  --  find the subcmd that contains the label       */
/*                           referred to in the current subcommand.        */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
struct subcmd **find_branch_address(struct subcmd **scpp,
                                    struct subcmd **topcmdp,
                                    int *ingrpflgp)
{
    struct subcmd **lpp, **lastscpp;
    int i, sbc = 0;

/* first, look forward */
    for (lpp = scpp + 1; *lpp; lpp++)
      {
         i = abs((*lpp)->cmd_idx);
         if (cmd_table[i].cmd_tag == LABEL_MARK)
           {
             if ( strcmp((*lpp)->args[0], (*scpp)->args[0]) == 0)
               return(++lpp);
           }
         else
           if (cmd_table[i].cmd_tag == END_SUBGROUP)
             *ingrpflgp -= 1;
      }

/* if label not found, start at top and work down */
    lastscpp = lpp;
    for (lpp = topcmdp; lpp != scpp; lpp++)
      {
         i = abs((*lpp)->cmd_idx);
         if (cmd_table[i].cmd_tag == LABEL_MARK)
           {
             if ( strcmp((*lpp)->args[0], (*scpp)->args[0]) == 0)
               {
                 for (; lpp != scpp && sbc; lpp++)
                   {
                     i = abs((*lpp)->cmd_idx);
                     if (cmd_table[i].cmd_tag == BEGIN_SUBGROUP)
                       sbc++;
                     else
                        if (cmd_table[i].cmd_tag == END_SUBGROUP)
                          sbc--;
                   }
                 *ingrpflgp == sbc;
                 return(++lpp);
               }
           }
         else
           if (cmd_table[i].cmd_tag == BEGIN_SUBGROUP)
             sbc++;
           else
             if (cmd_table[i].cmd_tag == END_SUBGROUP)
               sbc--;
     }
  return(lastscpp);
}


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* dump_with_expansion  --  output the current line with any non-printable */
/*                          characters exapanded to hex sequences.  A      */
/*                          binary zero would come out <0x00>.             */
/*                                                                         */
/*                                                                         */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
void dump_with_expansion(char *pattern_space)
{
   char line[MAXLINE];
   char *p, *q;
   int len;

   p = pattern_space;
   while (*p)
     {
       len = 0;
       for (q = line; *p && *p != '\n'; p++, q++, len++)
          if (isprint(*p))
            {
              *q = *p;
              len++;
              if ( (MAXLINE - len) == 1)
                break;
            }
          else
            {
              if ( (MAXLINE - len) < 7)
                break;
              strcpy(q, "<0x");
              q += 3;
              xtoa(q, p);
              q += 2;
              strcat(q, ">");
            }
       *q = '\0';
       printf("%s\n", line);
       q = line;
       if (*p == '\n')
         p++;
     }
}


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*   replace_chars  --  if any character in the line is also in arg1,      */
/*                      replace it with the corresponding char in arg2.    */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
int replace_chars(struct subcmd *scp, char *line)
{
   int i, chg;
   char *from, *to, *p;


   chg = NO;
   from = scp->args[0];
   to   = scp->args[1];

   for (p = line; *p; p++)
      for (i = 0; from[i]; i++)
         if (*p == from[i])
           {
             *p = to[i];
             chg = YES;
             break;
           }
   return(chg);
}


/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/*  change  --  if arg1 matches a substring in the line, replace that      */
/*              substring with arg2.                                       */
/*                                                                         */
/*                                                                         */
/*                                                                         */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* this routine invokes the regular expression parser */

int change(struct subcmd *scp, char **line)
{
   subpat spd;
   char *p, *q, *nextP, *matchstart, *matchend, *newstr, *restart_point;
   char *pattern;                                      /* @11a */
   int chgall, putit, writeit, changed, matchlen, replacelen, newlen, i;
   int hasBol = 0;                                     /* @11a */
   FILE *fo;

   chgall = putit = writeit = changed = NO;
   for (p = scp->args[2]; p && *p; p++)                /* get flags */
     {
       if (*p == 'g')
         {                                                          /* @2a */
                           /* NOTE: these lines are a temporary        @2a */
                           /* workaround to prevent a loop caused by   @2a */
                           /* the @1 changes immediately below.        @2a */
                           /* Basically, the 'g' flag is a NOP if      @2a */
                           /* EOL is part of the search pattern        @2a */
            for (q = scp->args[0]; *q!=3 && *q; q++);               /* @2a */
            if (!*q)                                                /* @2a */
              chgall = YES;
         }                                                          /* @2a */
       else
         if (*p == 'p')
           putit = YES;
         else
           if (*p == 'w')
             {
                writeit = YES;
                memcpy((void *)&fo, (void *)++p, sizeof(FILE *));
                break;
             }
     }


   pattern = scp->args[0];                 /* @11a */
   if (*pattern == 0x02)                    /* @11a pattern has BOL */
      {                                    /* @11a */ 
         hasBol = 1;                       /* @11a */ 
         pattern++;                        /* @11a */ 
      }                                    /* @11a */ 
   for (p = *line; *p; p++)
     {
       spd.numsubpats = spd.activesubpats = 0;
       if ( nextP = pmatch(p, scp->args[0], &spd) )
         {                        /* match found */
           matchstart = p;
           matchlen = nextP - p;
           matchend = nextP;
           if ( (*nextP == '\0') && (*(nextP-1) == '\n') )          /* @1a */
             {                                                      /* @1a */
               matchlen--;                                          /* @1a */
               matchend--;                                          /* @1a */
             }                                                      /* @1a */
           replacelen = 0;       /* how long will the replacement text be ? */
           for (p = scp->args[1]; p && *p; p++)                     /* @2c */
             {
                if (*p == '\\')
                  if (!*++p)
                    {
                      replacelen++;
                      break;
                    }
                  else
                            /* Subpattern length may include a         @1a */
                            /* terminating new line.  This code strips @1a */
                            /* that byte so that data can be inserted  @1a */
                            /* in front of the terminating $           @1a */
                    if (*p && isdigit(*p) && *p != '0')
                      {                                             /* @1a */
                        i = *p - '1';                               /* @1a */
                        if (i < spd.numsubpats)                     /* @1a4 */
                          {
                            q = spd.spdata[i].start + spd.spdata[i].length;
                            if ( (*q == '\0') && (*(q-1) == '\n') )
                              spd.spdata[i].length -= 1;             /* @1a */
                            replacelen += spd.spdata[*p - '1'].length;
                          }
                        else                                         /* @1a */
                          {                                          /* @8a */
                            replacelen++;                            /* @1a */
//                          replacelen++;                            /* @8a */
                          }                                          /* @8a */
                       }                                             /* @1a */
                    else
                      {                                              /* @8a */
                        replacelen++;
//                      replacelen++;                                /* @8a */
                      }                                              /* @8a */
                else
                  if (*p == '&')
                    replacelen += matchlen;
                  else
                    replacelen++;
             }
                                  /* how long will the changed line be? */
                                  /* Search patterns in the form x*$    @10a */
                                  /* are not handled exactly right by   @10a */
                                  /* grep.  In effect, it returns one   @10a */
                                  /* byte too many for matchlen.  So we @10a */
                                  /* always add an extra byte to the    @10a */
                                  /* malloced area.                     @10a */
//           newlen = strlen(*line) + replacelen - matchlen + 1;        @10d 
           newlen = strlen(*line) + replacelen - matchlen + 2;       /* @10a */
                                  /* allocate that much space */
           if ( (newstr = (char *)malloc(newlen)) == NULL)
             {
                myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "change");
                return(BAIL_OUT);
             }
                                  /* copy over the unmatched head */
           for (p = *line, q = newstr; p < matchstart; *q++ = *p++);
                                  /* copy over the replacement text */
           for (p = scp->args[1]; p && *p; p++)                     /* @1a */
             {
                if (*p == '\\')
                  if (!*++p)
                    break;
                  else
                    if (*p == 'n')
                      *q++ = NEWLINE;
                    else
                      if (*p == 't')
                        *q++ = TAB;
                      else
                        if (isdigit(*p) && *p != 0)
                          {
                             i = *p - '1';
                             if (i < spd.numsubpats)                /* @1a */
                               {                                    /* @1a */
                                 strncpy(q, spd.spdata[i].start,
                                            spd.spdata[i].length);
                                 q += spd.spdata[i].length;
                               }                                    /* @1a */
                             else                                   /* @1a */
                               {                                    /* @8a */
//                               *q++ = '\\';                       /* @8a */
                                 *q++ = *p;                         /* @1a */
                               }                                    /* @8a */
                          }
                        else
                          {                                         /* @8a */
//                           *q++ = '\\';                           /* @8a */
                             *q++ = *p;
                          }                                         /* @8a */
                else
                  if (*p == '&')
                    {
                       strncpy(q, matchstart, matchlen);
                       q += matchlen;
                    }
                  else
                    *q++ = *p;
             }
           restart_point = q;  /* Here's where the next grep would start */
                               /* finally copy over the unmatched tail */
           for (p = matchend; *p; *q++ = *p++);
           *q = '\0';          /* and null terminate the whole thing */
           free(*line);        /* free space occupied by original line */
           *line = newstr;     /* because we have a new improved line */
           changed = YES;
           if (chgall)         /* Do we want to do all occurances? */
             p = restart_point - 1;   /* Yes.  */
           else
             break;                   /* No.   */

         } /* endif (no match up to this point) */
       if (hasBol)             /* @11a if matching only on Bol */
          break;               /* @11a */
     }  /* endfor each character in the pattern space */

   if (changed == YES)
     {
       if (putit)          /* Are we supposed to display changed lines? */
         fputs(newstr, stdout);       /* Yes */
       if (writeit)        /* Are we supposed to write it to a file? */
         fputs(newstr, fo);
     }

   return(changed);
}


struct subcmd **find_subgroup_end(struct subcmd **scpp)
{
   struct subcmd **pp;

   for (pp = scpp; *pp; pp++)
      if (cmd_table[abs((*pp)->cmd_idx)].cmd_tag == END_SUBGROUP)
        break;
   return(++pp);
}


void xtoa(char *where, char *p)
{
    char *q;
    char *r = "0123456789abcdef";
    int i;

    q = where;
    i = (unsigned)*p >>4;
    *q++ = r[i];
    i = *p & 15;
    *q = r[i];
}
