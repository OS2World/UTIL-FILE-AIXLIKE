static char sccsid[]="@(#)78	1.1  src/sed/sed.c, aixlike.src, aixlike3  9/27/95  15:45:43";
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
/* @1 05.08.91 changed fmf_init to look for r/o, hidden and sys files    */
/* @2 05.19.91 line numbers should be relative to 1, not 0               */
/* @3 02.17.92 Peter Schwaller noticed that -f <nothing> causes a trap   */
/*             if it is the last command line parm.  Same for -e.        */
/* @4 03/16/92 Bob Gibson noticed 'q' command doesn't work right.        */
/* @5 03/17/92 Bob Gibson noticed a/ wasn't quite right                  */
/* @6 03/17/92 Bob Gibson noticed short file names failed after w        */
/* @7 09/16/92 Make return code explicit.                                */
/* @8 05/03/93 Modify for IBM C-Set/2 compiler                           */
/* @12 09/05/95 Match $ addr                                             */
/*-----------------------------------------------------------------------*/

/*
   sed   -   provides a stream editor

   Syntax:
             sed [-n] [-e Script] [-f SourceFile]   File
                           |              |           |
                           |              |           |
                           ----------------------------
                                        |
                                 0 or more of each

   Description:
      The sed command modifies lines from the specified File according to an
      edit script and writes them to standard output.  The sed command
      includes many features for selecting lines to be modified and making
      changes only to the selected lines.

      The sed command uses two work spaces for holding the line being modified:
      the pattern space, where the selected line is held; and the hold space,
      where a line can be stored temporarily.

      An edit script consists of individual subcommands, each one on a
      seperate line.  The general form of sed subcommands is the following:

               [address-range] function [modifiers]

      The sed commadn process each input File by reading an input line into a
      pattern space, applying all sed subcommands in sequence whose addresses
      select that line, and writing the pattern space to standard output.
      It then clears the pattern space and repeats this process for each line
      in the input File.  Some of the subcommands use a hold space to save
      all or part of the pattern space for subsequent retrieval.

      When a command includes an address (either a line number or a search
      pattern), only the addressed line or lines are affected by the command.
      Otherwise, the command is applied to all lines.

      An address is either a decimal line number, a $ (dollar sign), which
      addresses the last line of input, or a context address.  A context
      address is a regular expression similar to those used in ed except
      for the following differences:

         .   You can select the character delimiter for patterns.  The general
             form of the expression is
                    ?pattern?
             where ? is a character delimiter you select.
             The default form for the pattern is  /pattern/.

         .   The \n sequence matches a new-line character in the pattern
             space, except the terminating new line.

         .   A dot (.) matches any character except a terminating new-line
             character.  That is, unlike ed, which cannot match a new-line
             character in the middle of a line, sed can match a new-line
             character in the pattern space.

      Certain commands allow you to specify one line or a range of lines
      to which the command should be applied.  These commands are called
      addressed commands.  The following rules apply to addresses commands:

         .   A command line with no address selects every line.

         .   A command line with one address, expressed in context form,
             selects each line that matches the address.

         .   A command line with two addresses separated by commas selects
             the entire range from the first line that matches the first
             address to next line that matches the second.  If the second
             address is a number less than or equal to the line number first
             selected, only one line is selected.  Thereafter, the process
             is repeated, looking again for the first address.

      NOTES:

         1.  The Text parameter accompanying the a\, c\ and i\ commands
             can continue onto more than one line provided all lines but
             the last end with a \ to quote the new-line character.
             Back slashes in text are treated like back slashes in the
             replacement string of an s command, and can be used to protect
             initial blanks and tabs against the stripping that is done on
             every script line.  The RFile and WFile parameters must end
             the command line and must be preceded by exactly one blank.
             Each WFile is created before processing begins.

         2.  The sed command can process up to 99 commands in a pattern file.

   Flags

     -e Script      Uses the Script string as the editing script.  If you
                    are using just one -e flag and no -f flag, the -e can
                    be omitted.

     -f SourceFile  Uses SourceFile as the source of the edit script.
                    SourceFile is a prepared set of editing commands to be
                    applied to File.

     -n             Suppresses all information normally written to standard
                    output.

   Commands

  (1)a\          Places Text on the output before reading the next input line.
     Text

  (2)b[label]    Branches to the : command bearing the label.  If label is
                 the null string, it brances to the end of the script.

  (2)c\          Deletes the pattern space; with 0 or 1 addresses, or at the
     Text        end of a 2-address range, places Text on the output.  Then
                 it starts the next cycle.

  (2)d           Deletes the pattern space.  Then it starts the next cycle.

  (2)D           Deletes the initial segment of the pattern space through
                 the first new-line character.  Then it starts the next
                 cycle.

  (2)g           Replaces the contents of the pattern space with the contents
                 of the hold space.

  (2)G           Appends the contents of the hold space to the pattern space.

  (2)h           Replaces the contents of the hold space with the contents
                 of the pattern space.

  (2)H           Appends the contents of the pattern space to the hold space.

  (1)i\          Writes Text to standard output before reading the next line
     Text        into the pattern space.

  (2)I           Writes the patttern space to standard output showing
                 non-displayable characters as 2- or 4-digit hex values.
                 Long lines are folded.

  (2)n           Writes the pattern space to standard output.  It replaces
                 the pattern space with the next line of input.

  (2)N           Appends the next line of input to the pattern space with an
                 embedded new-line character (the current line number
                 changes).  You can use this to search for patterns that are
                 split onto two lines.

  (2)p           Writes the pattern space to standard output.

  (2)P           Writes the initial segment of the pattern space through the
                 first new line character to standard output.

  (1)q           Branches to the end of the script.  It does not start a
                 new cycle.

  (2)r RFile     reads the contents of RFile.  It places contents on the
                 output before reading the next input line.

  (2)s/pattern/replacement/flags
                 Substitutes replacement string for the first occurance of
                 the pattern in the pattern space.  Any character that is
                 displayed after the s command can substitute for the /
                 separator.

                 You can add zero or more of the following flags:

                 g     Substitutes all nonoverlapping instances of the pattern
                       parameter, rather than just the first one.

                 p     Writes the pattern psace to standard out if a
                       replacement was made.

                 w WFile
                       Writes the pattern pace to WFile if a replacement
                       was made.  Appends the pattern space to WFile.
                       If (WFile was not already created by a previous write
                       by this sed script, sed creates it.

                 NOTE: in the replacement string, the unescaped character
                 '&' stands for the text matched by the pattern string.

  (2)tlabel      Branches to :label in the script file if any substitutions
                 were made since the most recent reading of an input line or
                 execution of a t subcommand.  If you do not specify label,
                 control transfers to the end of the script.

  (2)w WFile     Appends the pattern space to WFile.

  (2)x           Exchanges the contents of the pattern space and the hold
                 space.

  (2)y/pattern1/pattern2/
                 Replaces all occurances of characters in pattern1 with
                 the corresponding pattern2 characters.  They byte lengths
                 of pattern1 and pattern2 must be equal.

  (2)! sed-cmd   Applies the subcommand only to line NOT selected by the
                 address or addresses.

  (0):label      This script entry simply marks a branch point to be referenced
                 by the b and t commands.  This label can be any sequence
                 of 8 or fewer bytes.

  (1)=           Writes the current line number of standard output as a line.

  (2){subcmd ... ...}
                 Groups subcommands enclosed in {} (braces).

*/
#include <idtag.h>    /* package identification and copyright statement */
#define INCL_ERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fmf.h>
#include "sed.h"


struct subcmd *cmds[MAXCMDS+1];

extern struct cmd_table_entry cmd_table[];   /* table of valid edit commands */

/* -----------------------  individual globals ---------------------------*/
char readarea[MAXLINE];
int suppress_output;
int firstfile;
int num_subcmds = 0;
int num_files;

extern char *myerror_pgm_name;

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*   main()                                                                 */
/*      check out command line, getting the script cmds; it that's OK       */
/*        open any output files that were specified; it that's OK           */
/*          for each file specified on the command line                     */
/*            apply the script commands                                     */
/*          (of course, stdin as the input file is a special case).         */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int main(int argc, char **argv)                                     /* @7c */
{
   int i;
   int rc;                                                          /* @7a */

   rc = parse_cmd_line(argc, argv, cmds, &num_subcmds);             /* @7a */
   if (rc != BAIL_OUT)                                              /* @7c */
     if (open_files(cmds, argc, argv) != BAIL_OUT)
       if (num_files)
         {
           for (i = firstfile; i < argc; i++)
             if (process_file_spec(argv[i], cmds) == BAIL_OUT)
               {                                                    /* @7a */
                 rc = BAIL_OUT;                                     /* @7a */
                 break;
               }                                                    /* @7a */
         }
       else
         rc = process_file(NULL, cmds);                             /* @7c */
     else                                                           /* @7a */
       rc = BAIL_OUT;                                               /* @7a */
   return(rc);                                                      /* @7a */
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  parse_cmd_line  --  check out the command line, getting cmds and files. */
/*                                                                          */
/*  First, do the switches:                                                 */
/*      -n turns off automatic-write-to-stdout.  It it's on, you have to    */
/*         write each pattern space to stdout explicitely.                  */
/*                                                                          */
/*      -e specifies a script command.  Add it to the list.                 */
/*                                                                          */
/*      -f specified a file full of script commands.  Add its contents to   */
/*         the list.                                                        */
/*                                                                          */
/*  The next argument could be a script command if none had been specified  */
/*  earlier.  If that's the case, add it to the list.                       */
/*                                                                          */
/*  Whatever is left on the command line must be an input file.  Keep a     */
/*  pointer to the first one.                                               */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int parse_cmd_line (int argc, char ** argv, struct subcmd **cmdsp, int *numscp)
{
    int i, rc, num_es, num_fs;
    char *p;

    myerror_pgm_name = "sed";
    suppress_output = NO;
    *cmdsp = NULL;
    num_es = num_fs = 0;
    num_files = 0;                 /* first, do the switches */
    for (i = 1; i < argc && *argv[i] == '-'; i++)
      {
        p = argv[i];         /* there are 3 valid flags: e, f and n */
        if (*++p == 'n')
          suppress_output = YES;
        else
          if (*p == 'e')     /* an edit script specified on the cmd line */
            {
              num_es++;
              if (*++p != '\0')
                rc = add_subcmd(p, cmds, numscp);
              else
                if (i < argc)                                           //@4a
                  rc = add_subcmd(argv[++i], cmds, numscp);
                else                                                    //@4a
                  rc = BAIL_OUT;                                        //@4a
              if (rc == BAIL_OUT)
                return(BAIL_OUT);
              else
                *numscp += 1;
            }
          else
            if (*p == 'f')    /* a script file is specified on the cmd line */
              {
                num_fs++;
                if (*++p != '\0')
                  rc = process_script_file(p, cmds, numscp);
                else
                  if (i < argc)                                          //@4a
                    rc = process_script_file(argv[++i], cmds, numscp);
                  else                                                   //@4a
                    rc = BAIL_OUT;                                       //@4a
                if (rc == BAIL_OUT)
                  return(BAIL_OUT);
              }
            else              /* who knows what that thing is? */
              fprintf(stderr,"Unknown flag: %s\n", p);
      }  /* end FOR */

/* Now we're done with the switches.  One of three things has occured:    */
/*              1. no more command line arguments                         */
/*              2. an arg was found that didn't start with '-'            */
/*              3. an unknown switch was found.                           */

    if (i == argc)    /* if it's the first condition */
      return(OK);        /* we done */

    if ((num_es + num_fs) == 0)     /* if no -e and no -f, first non-switch */
                                    /* must be the edit script */
      if ( (rc = add_subcmd(argv[i++], cmds, numscp)) == BAIL_OUT)
        return(BAIL_OUT);
      else
        *numscp++;

/* everything else on the command line must be an input file spec         */

    num_files = argc - i;
    firstfile = i;
    return(OK);
}

/* commands begin and end on one line, and there can be only one command   */
/* per line.  A command may be continued if the new line character is      */
/* escaped.  It all gets kind of complicated when groups of subcommands    */
/* are involved.                                                           */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* process_script_file  --  extract script commands from the specified file.*/
/*                                                                          */
/* Open the file                                                            */
/* The first line can be a comment line; if it is, it can be a special      */
/*   comment "#n" which is the same as using the -n command line switch.    */
/*   The file must contain at least one more line after this.               */
/* For each non-blank line in the file                                      */
/*    Concatenate next line to any line that ends with a slash              */
/*    Treat each line thus constructed as a command - add it to the list.   */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int process_script_file(char *filename, struct subcmd **cmdsp, int *numscp)
{
  FILE *stream;
  char *mem, *lineend, *p;
  int  curlen;

  if ( (stream = fopen(filename, "r")) == NULL)           /* @8c */
    {
       fprintf(stderr,"sed: can't open script file %s\n", filename);
       return(BAIL_OUT);
    }
  if (fgets(readarea, MAXLINE, stream) == NULL)   /* read first line */
    {
       fprintf(stderr,"sed: unable to read script file %s\n", filename);
       return(BAIL_OUT);
    }

  if (readarea[0] == COMMENT_MARK)  /* The first line can be a comment line */
    {
      if (readarea[1] == 'n')
        suppress_output = YES;

      if (fgets(readarea, MAXLINE, stream) == NULL)   /* replace first line */
        {
          if (feof(stream))
            fprintf(stderr,
                    "sed: script file %s must contain one non-comment line\n",
                   filename);
          else
            myerror(-1, "read error", filename);
          return(BAIL_OUT);
        }
     }

  do
    {
      if (isblankline(readarea))  /* if this line is blank */
        continue;               /* go get another line */
      curlen = strlen(readarea) + 1;
      if ( (mem = (char *)malloc(curlen)) == NULL)
        {
          myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc line", "process_script_file");
          return(BAIL_OUT);
        }
      strcpy(mem, readarea);
      lineend = mem;
      do
      {
        for (p = lineend; *p && *p != '\n'; p++);
        if (*p == '\n' && *--p == BACKSLASH)
          {
            if (fgets(readarea, 512, stream) == NULL)   /* replace first line */
              {
                if (feof(stream))
                  fprintf(stderr,"sed: script file %s ends badly\n",
                         filename);
                else
                  myerror(-1, "read error", filename);
                return(BAIL_OUT);
              }
            curlen = curlen + strlen(readarea) + 1;
            if ( (mem = (char *)realloc(mem, curlen)) == NULL)
              {
                 myerror(ERROR_NOT_ENOUGH_MEMORY, "realloc", filename);
                 return(BAIL_OUT);
              }
            strcat(mem, readarea);
            lineend = p + 2;
          }
      } while(*p == BACKSLASH);
      if (add_subcmd(mem, cmdsp, numscp) == BAIL_OUT)
        return(BAIL_OUT);
      else
        *numscp += 1;
      free(mem);
    } while(fgets(readarea, 512, stream));
    if (ferror(stream))
      {
         myerror(-1, "read error", filename);
         return(BAIL_OUT);
      }
    fclose(stream);
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  add_subcmd  --  add an edit command to the list                         */
/*                                                                          */
/*  Allocate memory for a structure to hold the command.                    */
/*  A command has the structure                                             */
/*                                                                          */
/*     [addr1 [, addr2]] command [/from/to/[flags]]                         */
/*                                                                          */
/*  where the addresses can be specified as line numbers or context,        */
/*  and '/' can be any character acting as a delimiter.                     */
/*                                                                          */
/*  A context-based address has the form of a delimited regular expression. */
/*  The delimiter can be anything except a digit or one of the command      */
/*  characters.                                                             */
/*                                                                          */
/*  There can be only one command on a line, except that the negation       */
/*  operator (!) must immediately precede the command it applies to (on     */
/*  the same line), and the begin-subgroup command ({), must also be        */
/*  on the same line as the first command in the subgroup.                  */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

int add_subcmd(char *command_string, struct subcmd **cmdsp, int *numscp)
{
    char *p;
    struct subcmd *newsc;
    int  addrno, cmd_idx, negflag;

    if ( (newsc = (struct subcmd *)malloc(sizeof(struct subcmd))) == NULL)
      {
        myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "add_subcmd");
        return(BAIL_OUT);
      }

    addrno = 0;
    newsc->saddr[0].type    = newsc->saddr[1].type    = NO_ADDRESS;
    newsc->saddr[0].val.pch = newsc->saddr[1].val.pch = NULL;
    newsc->args[0] = newsc->args[1] = newsc->args[2] = NULL;
    p = command_string;
    p = skipwhite(p);
    if (isdigit(*p))          /* address is present, and it's a line number */
      p = get_lineno_address(p, newsc, &addrno);
    else
      if ( (cmd_idx = find_command_idx(*p)) == -1)
          if ( (p = get_context_address(p, newsc, &addrno)) == NULL)
            return(BAIL_OUT);

/* We're past the first address, if any; next is command or another address */
    p = skipwhite(p);
    if (addrno && *p == ',')   /* it is another address */
      {
        p = skipwhite(++p);
        if (isdigit(*p))
          p = get_lineno_address(p, newsc, &addrno);
        else
          if ( (cmd_idx = find_command_idx(*p)) == -1)
            if ( (p = get_context_address(p, newsc, &addrno)) == NULL)
              return(BAIL_OUT);
      }
                             /* if it's not a command, we're in trouble */
    p = skipwhite(p);
    if (*p == NEGATION)     /* exclamation is a prefix operator */
      {
        negflag = -1;
        skipwhite(++p);
      }
    else
        negflag = 1;

    if (*p == BEGIN_SUBGROUP)  /* left brace is also prefix, but right brace */
                                       /* goes on its own line! */
      {
        newsc->cmd_idx = find_command_idx(BEGIN_SUBGROUP) * negflag;
        newsc->status = READY;
        cmdsp[*numscp] = newsc;
        *numscp += 1;
        if ( (newsc = (struct subcmd *)malloc(sizeof(struct subcmd))) == NULL)
          {
            myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "add_subcmd");
            return(BAIL_OUT);
          }
        newsc->saddr[0].type    = newsc->saddr[1].type    = NO_ADDRESS;
        newsc->saddr[0].val.pch = newsc->saddr[1].val.pch = NULL;
        newsc->args[0] = newsc->args[1] = newsc->args[2] = NULL;
        addrno = 0;
        skipwhite(++p);
      }

    if ( (cmd_idx = find_command_idx(*p)) == -1)
      {
        fprintf(stderr, "sed: invalid script line: %s\n", command_string);
        return(BAIL_OUT);
      }
                             /* see if the number of addresses is appropriate */
    if (cmd_table[cmd_idx].max_addr < addrno)
      {
        fprintf(stderr,
                "sed: too many addresses for %c command in script line\n%s\n",
                cmd_table[cmd_idx].cmd_tag, command_string);
        return(BAIL_OUT);
      }

    newsc->cmd_idx = cmd_idx * negflag;
    newsc->status = READY;

    switch (*p)
    {
       case UNCONDITIONAL_BRANCH:
       case CONDITIONAL_BRANCH:
       case PUT_TEXT_FROM_FILE:
       case APPEND_PATTERN_TO_FILE:
       case LABEL_MARK:        p = store_next_string(p, &newsc->args[0]);
       break;

       case PUT_TEXT:
       case IPUT_TEXT:
       case REPLACE:           p = store_text(p, &newsc->args[0]);
       break;

       case CHARACTER_REPLACE: p = store_to_from(p, &newsc->args[0], NOCREP);
                               if (strlen(newsc->args[0])
                                           !=
                                                strlen(newsc->args[1]))
                                 {
                                   printf("sed: bad y command: %s\n",
                                                                command_string);
                                   return(BAIL_OUT);
                                 }
       break;

       case CHANGE:            if (p = store_to_from(p, &newsc->args[0], CREP))
                                 p = store_flags(p, &newsc->args[2]);
       break;
    }
    if (p == NULL)
      return(BAIL_OUT);
    cmdsp[*numscp] = newsc;
    return(OK);
}



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* open_files  --  open any files that might be needed during processing    */
/*                 (other than the input files specified on the cmd line )  */
/*                                                                          */
/* I've put some debug code in here.  If it's still there when you see it,  */
/* feel free to remove it yourself.                                         */
/*                                                                          */
/* Examine each edit command.  If it's 'r' or 'w', open the file associated */
/* with the command for read or write, respectively.  Replace the arg       */
/* pointer to the file name with the file handle.  Free the space being     */
/* used to store the file name.                                             */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int open_files(struct subcmd **cmdsp,        int argc, char **argv)
{
   struct subcmd *scp;
   struct wf     wrtfiles[MAXWF];    /* place to keep track of output files */
   FILE *f;
   char *p, *q, *hloc;
   int i;
   FILE *findwf(char *filename, struct wf *wftable);
   void         addwf(char *filename, FILE *stream, struct wf *wftable);

#ifdef DEBUG

   printf("%d input files.\n",num_files);
   if (firstfile)
     for (i = firstfile; i < argc; i++)
       printf("\t%s\n", argv[i]);
   printf("Commands: \n");
   if (*cmdsp == NULL)
     {
       printf("none\n");
       return(OK);
     }
   else
     for (scp = *cmdsp; scp; scp = *++cmdsp)
       {
          printf("\tcommand: ");
          if (scp->cmd_idx < 0)
            printf("(NOT)");
          printf("%s\n", cmdnames[abs(scp->cmd_idx)]);
          for (i = 0; i < 2; i++)
            {
               printf("\t\tAddress %d: ", i + 1);
               if (scp->saddr[i].type == NO_ADDRESS)
                 printf("none\n");
               else
                 if (scp->saddr[i].type == LINENO_ADDRESS)
                   printf("(lineno) %d\n", scp->saddr[i].val.lineno);
                 else
                   printf("(context) %s\n", scp->saddr[i].val.pch);
            };
          for (i = 0; i < 3; i++)
            {
               printf("\t\tArg %d: ", i + 1);
               if (scp->args[i] == NULL)
                 printf("NULL\n");
               else
                 printf("%s\n", scp->args[i]);
            }
       }
#endif
   for (i = 0; i < MAXWF; wrtfiles[i++].filename = NULL);

   for (scp = *cmdsp; scp; scp = *++cmdsp)
     {
        switch (cmd_table[abs(scp->cmd_idx)].cmd_tag)
        {
           case PUT_TEXT_FROM_FILE:
                          if ((f = fopen(scp->args[0], "r")) == NULL) /* @8c */
                            {
                               fprintf(stderr,"sed: could not read file %s\n",
                                      scp->args[0]);
                               return(BAIL_OUT);
                            }
                          free(scp->args[0]);
//                          (FILE *)scp->args[0] = f;
                          scp->args[0] = (char *)f;              /* @Z9a */
           break;

           case APPEND_PATTERN_TO_FILE:
                          if ( (f = findwf(scp->args[0], wrtfiles)) == NULL)
                            {
// @5d                          if ((f = fopen(scp->args[0], "at")) == NULL)
/* @5a */                     if ((f = fopen(scp->args[0], "wt")) == NULL)
                                {
                                   fprintf(stderr,"sed: could not write file %s\n",
                                          scp->args[0]);
                                   return(BAIL_OUT);
                                }
                              else
                                addwf(scp->args[0], f, wrtfiles);
                            }
                          free(scp->args[0]);
//                          (FILE *)scp->args[0] = f;
                          scp->args[0] = (char *)f;
           break;

           case CHANGE:
                          for (p = scp->args[2]; p && *p; p++)
                            {
                              if (*p == 'w')
                                {
                                  hloc = p+1;
                                  if (strlen(++p) < sizeof(FILE *)+2)
                                    {
                                      p = realloc(scp->args[2],
                                                  strlen(scp->args[2]) +
                                                  sizeof(FILE *) +2);
                                      scp->args[2] = p;
                                      for (; *p != 'w'; p++);
                                      p++;
                                    }
/*                                  else                                  @6d*/
                                  p = skipwhite(p);
                                  for (q = p; *q && !isspace(*q); q++);
                                  *q = '\0';
                                  if ( (f = findwf(p, wrtfiles)) == NULL)
                                    {
                                      if ((f = fopen(p, "w")) == NULL) /*@6c@8c*/
                                        {
                                           fprintf(stderr,
                                                   "sed: could not write file %s\n",
                                                   p);
                                           return(BAIL_OUT);
                                        }
                                      else
                                        addwf(p, f, wrtfiles);
                                     }
//                                  (FILE *)*hloc = f;
                                  break;
                                }
                            }
           break;

        }
     }

   for (i = 0; i < MAXWF && wrtfiles[i].filename; free(wrtfiles[i++].filename));

return(OK);
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  findwf  --  see if a handle is already open for the named file          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
FILE *findwf(char *name, struct wf wftable[])
{
   int i;

   for (i = 0; i < MAXWF && wftable[i].filename; i++)
     if (strcmp(name, wftable[i].filename) == 0)
       return(wftable[i].stream);
   return(NULL);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  addwf  --  add a newly opened file to the table of open write files     */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void addwf(char *name, FILE *str, struct wf wftable[])
{
   int i;

   for (i = 0; i < MAXWF; i++)
      if (wftable[i].filename == NULL)
        break;
   if (i == MAXWF)
     i = 0;
   wftable[i].filename = (char *) malloc(strlen(name) + 1);
   strcpy(wftable[i].filename, name);
   wftable[i].stream = str;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  process_file_spec  --  disambiguate the input file specs from the       */
/*                         command line.                                    */
/*                                                                          */
/*  Initialize the disambiguation routine.                                  */
/*  If no files match the spec, display an error message.                   */
/*  For each file that does match the file spec,                            */
/*    process the file.                                                     */
/*                                                                          */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int process_file_spec(char *FileSpec, struct subcmd **cmdsp)
{
   char filename[MAXPATH + 1];
   int  attr;

//   if (fmf_init(FileSpec, 0, 0) == FMF_NO_ERROR)                       @1
   if (fmf_init(FileSpec, FMF_ALL_FILES, 0) == FMF_NO_ERROR)         //  @1
     while (fmf_return_next(filename, &attr) == FMF_NO_ERROR)
       {
         if (process_file(filename, cmds) == BAIL_OUT)
           return(BAIL_OUT);
       }
   else
     fprintf(stderr,"No files matching  %s  were found.\n", FileSpec);
}



/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  process_file                                                            */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int process_file(char *FileName, struct subcmd **cmdsp)
{
    long lineno;
    FILE *f;
    char *ln;

//    lineno = 0L;                                              @2d
    lineno = 1;                                               //@2a
    if (FileName == NULL)
      f = stdin;
    else
      if ( (f = fopen(FileName, "r")) == NULL)             /*@8c*/
        {
          fprintf(stderr,"Cannot open input file %s for editing.\n");
          return(OK);
        }

    while (fgets(readarea, MAXLINE, f))
      {
        ln = (char *)malloc(strlen(readarea) + 1);
        if (ln == NULL)
          {
             myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "process_file");
             return(BAIL_OUT);
          }
        strcpy(ln, readarea);
                                                                   /* @4c2 */
        if ( (ln = applycmds(cmdsp, ln, f, &lineno)) == (CHAR *)BAIL_OUT)
            return(BAIL_OUT);
//        if (!suppress_output)     /*moved to applycmds() */      /* @5d2 */
//          fputs(ln, stdout);
        free(ln);
        if (lineno == -1L)                /* the 'q' command was encountered */
          return(OK);
        lineno++;
      }
    fclose(f);
    return(OK);
}
