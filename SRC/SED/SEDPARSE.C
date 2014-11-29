static char sccsid[]="@(#)83	1.1  src/sed/sedparse.c, aixlike.src, aixlike3  9/27/95  15:45:54";
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

/* sedparse.c   Utility routines used when parsing the sed command line and */
/*              when compiling edit scripts.                                */

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/* @1 09.26.91 a 'from' or 'to' change involving a left bracket failed   */
/*-----------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sed.h"
#include "..\grep\grep.h"
#define INCL_ERRORS
#include <os2.h>

extern struct cmd_table_entry cmd_table[];

extern void myerror(int, char *, char *);

char *snullstr = "^";

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  skipwhite  --  skip over any white space                                */
/*                                                                          */
/*  If the pointer is to a non-white character, return; otherwise advance   */
/*  the pointer to the next non-white character                             */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *skipwhite(char *start)
{
   char *p;

   for (p = start; *p && isspace(*p); p++);
   return(p);
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*  isblankline  -- return YES if line is blank                             */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int isblankline(char *line)
{
   char *p;

   p = skipwhite(line);
   if (!*p || *p == '\n')
     return(YES);
   return(NO);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* get_lineno_address -- extract and store an address specified as line no. */
/*                                                                          */
/* Set the address type                                                     */
/* Extract the integer and store it                                         */
/* Advance the argument number                                              */
/* Return pointer to the next input string character after the number       */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *get_lineno_address(char *line, struct subcmd *scp, int *addr)
{
   long lineno;
   char *p;

   scp->saddr[*addr].type = LINENO_ADDRESS;
   for (p = line, lineno = 0l; *p && isdigit(*p); p++)
      lineno = lineno * 10 + (*p - '0');
   scp->saddr[*addr].val.lineno = lineno;
   *addr += 1;
   return(p);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* get_context_address -- extract and store a context address               */
/*                                                                          */
/* Set the address type                                                     */
/* Extract the text from between delimiters                                 */
/* Compile the text into a regular expression                               */
/* Advance the argument number                                              */
/* Return pointer to the next input string character after the delimiter    */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *get_context_address(char *line, struct subcmd *scp, int *addr)
{
   int i, in_bracket, true_size, crepsize;
   char delim;
   char *p, *q, *argstart;
   char *ss, *cs, *crep;

   in_bracket = 0;
   scp->saddr[*addr].type = CONTEXT_ADDRESS;
   p = line;
   p = skipwhite(p);
   if (*p == '$') {                                             /* @12a */
      scp->saddr[*addr].type = LASTLINE_ADDRESS;                /* @12a */
      (*addr)++;                                                /* @12a */
      return(++p);                                              /* @12a */
   }                                                            /* @12a */
   delim = *p;
   scp->saddr[*addr].val.pch = NULL;
   argstart = ++p;
   for (i = 0; *p; p++, i++)
     {
       if (*p == ESC_CHAR)                                         /* @1a */
         {                                                         /* @1a */
            if (*(p+1))                                            /* @1a */
              p++, i++;                                            /* @1a */
         }                                                         /* @1a */
       else                                                        /* @1c */
         if (in_bracket)
           {
             if (*p == ']')
               in_bracket = 0;
           }
         else
           if (*p == '[')
             in_bracket = 1;
           else
             if (*p == delim)
               break;
     }

   if (*p != delim)         /* if we somehow missed the delimiter */
     {
        fprintf(stderr,"Invalid context address: %s\n", line);
        return(NULL);
     }

   crep = (char *)malloc(i + 1);
   if (i > 1)
     crepsize = 3*i;
   else
     crepsize = 6;
   cs = (char *)malloc(crepsize);
   if (crep == NULL || cs == NULL)
     {
       myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "get_context_addr");
       return(NULL);
     }
   for (ss = crep, q = argstart; q < p; *ss++ = *q++);
   *ss = '\0';
   if ( (true_size = compile_reg_expression(crep, cs, crepsize)) == 0 )
     {
       fprintf(stderr, "sed: Invalid \'from\' reg expr: %s\n", crep);
       return(NULL);
     }
   else
     {
       free(crep);
       scp->saddr[*addr].val.pch = realloc(cs, true_size);
     }
   *addr += 1;
   return(++p);
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* find_command_idx  --  look up a command and return its table index       */
/*                                                                          */
/* Make a sequential search thru the table.  If the command letter is       */
/* found, return its table index; otherwise, return -1                      */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
int find_command_idx(char c)
{
   int i;

   for (i = 0; cmd_table[i].cmd_tag; i++)
      if (cmd_table[i].cmd_tag == c)
        return(i);
   return(-1);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* store_next_string  --  get the next blank-delimited string and store it  */
/*                                                                          */
/* Scan to the next white space (or end of string)                          */
/* Allocate memory to hold that string                                      */
/* Move the string there and null-terminate it                              */
/* Return pointer to the input character after the string (the blank,       */
/*   usually).                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *store_next_string(char *start, char **where)
{
   int len = 0;
   char *p, *q, *savp;

   p = start;
   p = skipwhite(++p);
   for (savp = p; *p && !isspace(*p) && *p != '\n'; p++, len++);
   if (len == 0)
     *where = NULL;
   else
     {
       *where = (char *)malloc(len + 1);
       if (*where == NULL)
         {
           myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "store_next_string");
           return(NULL);
         }
       else
         {
           for (q = *where; savp < p; *q++ = *savp++);
           *q = '\0';
         }
     }
   return(p);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* store_text  --  get the 'text' that follows a command and store it.      */
/*                                                                          */
/* The 'text' will follow either an escaped new line or the string '\\n'.   */
/* It will continue until a null, and may have imbedded newlines.           */
/*                                                                          */
/* Allocate memory to hold that string                                      */
/* Move the string there and null-terminate it                              */
/* Return pointer to the input character after the string (the blank,       */
/*   usually).                                                              */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char * store_text(char *start, char **where)
{
    char *p, *text, *mem;
    int  len;

    p = start;
    p = skipwhite(++p);
    if (*p++ != ESC_CHAR)
      {
         fprintf(stderr,
                 "Invalid %c command: requires escaped newline\n", start);
         return(NULL);
      }
    else
      if (*p == '\n')
        ++p;
      else
        if (*p++ != ESC_CHAR || *p != 'n')
          {
            fprintf(stderr,
                    "Invalid %c command: requires escaped newline\n", start);
            return(NULL);
          }

    text = p;
    len = strlen(p);
//    for (len = 0, ++p; *p; p++, len++);
    if (len == 0)
      (*where == NULL);
    else
      {
        if ( (mem = (char *)malloc(len + 1)) == NULL)
        {
           myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "store_text");
           return(NULL);
        }
        *where = mem;
        for (p = text; *p; p++, mem++)
           {
             if (*p == ESC_CHAR && *(p+1) == '\n')
               p++;
             *mem = *p;
           }
        *mem = '\0';
      }
    return(p);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* store_to_from  --  seperate a to-from pair from a s or y command         */
/*                                                                          */
/* A to-from pair is in the form                                            */
/*            /from text/to text/                                           */
/* where the vergules are any character.                                    */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *store_to_from(char *start, char **where, int flag)
{
    char *p, delim, *argstart, *ss, *cs, *q, *crep;
    int in_bracket, i, true_size, crepsize;

    p = start;
    p = skipwhite(++p);
    delim = *p;
    in_bracket = 0;
    argstart = ++p;
    for (i = 0; *p; p++, i++)
      {
        if (flag == CREP)
          {
            if (*p == ESC_CHAR)                                    /* @1a */
              {                                                    /* @1a */
                 if (*(p+1))                                       /* @1a */
                   {                                               /* @5a */
                     p++, i++;                                     /* @1a@*/
                     *p = getesctrx(*p);                           /* @5a */
                   }                                               /* @5a */
              }                                                    /* @1a */
            else                                                   /* @1c */
              if (in_bracket)
                {
                  if (*p == ']')
                    in_bracket = 0;
                }
              else
                if (*p == '[')
                  in_bracket = 1;
                else
                  if (*p == delim)
                    break;
          }
        else
          if (*p == delim)
            break;
      }

    if (*p != delim)         /* if we somehow missed the delimiter */
      {
         fprintf(stderr,"Invalid context address: %s\n", start);
         return(NULL);
      }

    if (i == 0)
      {
/*      where[0] == NULL;                                              @5d */
        where[0] = snullstr;     /* treat null str1 as 'beginning of line @5a */
        i = 1;                                                         /* @5a */
        crep = NULL;                                                   /* @5a */
      }
    else
      {                 /* allocate space for 'from', and move it there */
        crep = (char *)malloc(i + 1);
        if (crep == NULL)
          {
            myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "get_to_from1");
            return(NULL);
          }
        where[0] = crep;
        for (ss = crep, q = argstart; q < p; *ss++ = *q++);
        *ss = '\0';
      }                                                                /* @5a */
    if (flag == CREP)   /* if 'from' is a regular expression */
      {
        if (i > 1)
          crepsize = 3*i;
        else
          crepsize = 6;
        cs = (char *)malloc(crepsize);
        if (cs == NULL || ss == NULL)
          {
            myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "regexp_space");
            return(NULL);
          }
/*         where[0] = cs;                                                 @5d2*/
/*         if ( (true_size = compile_reg_expression(crep, cs, crepsize)) == 0 ) @5d */
        if ( (true_size = compile_reg_expression(where[0], cs, crepsize)) == 0 )
          {
            fprintf(stderr, "sed: Invalid \'from\' reg expr: %s\n", crep);
            return(NULL);
          }
        else
          {
            if (crep)                                                 /* @5a */
              free(crep);
            where[0] = realloc(cs, true_size);
          }
      }
                              /* now, start on the "to" side */
    argstart = ++p;
    for (i = 0; *p; p++, i++)
      {
        if (*p == ESC_CHAR)                                    /* @1a */
          {                                                    /* @1a */
             if (*(p+1))                                       /* @1a */
               {                                               /* @5a */
                 p++, i++;                                     /* @1a */
                 *p = getesctrx(*p);                           /* @5a */
               }                                               /* @5a */
          }                                                    /* @1a */
        else                                                   /* @1c */
          if (in_bracket)
            {
              if (*p == ']')
                in_bracket = 0;
            }
          else
            if (*p == '[')
              in_bracket = 1;
            else
              if (*p == delim)
                break;
      }
    if (*p != delim)         /* if we somehow missed the delimiter */
      {
         fprintf(stderr,"Invalid context address: %s\n", start);
         return(NULL);
      }

    if (i == 0)
      where[1] = NULL;
    else
      {
        ss = (char *)malloc(i + 1);
        if (ss == NULL)
          {
            myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "get_to_from2");
            return(NULL);
          }
        where[1] = ss;
        for (q = argstart; q < p; *ss++ = *q++);
        *ss = '\0';
      }
    return(++p);
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* store_flags  --  store away  the flags info from an s command            */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char *store_flags(char *start, char **where)
{
   char *p, *ss;
   int len;


   p = start;
   p = skipwhite(p);
   len = strlen(p);
   if (len == 0 || *p == '\n')
     *where = NULL;
   else
     {
       if ( (ss = (char *)malloc(len + 1)) == NULL)
          {
            myerror(ERROR_NOT_ENOUGH_MEMORY, "malloc", "get_context_addr");
            return(NULL);
          }
       strcpy(ss, p);
       *where = ss;
     }
   return(p + len);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* getesctrx()  --  return either the character passed, or the translation  */
/*                  appropriate when the passed character is preceded by    */
/*                  a backslash.                                            */
/*                  I.e.:  /t returns 0x09                                  */
/*                         /z returns z                                     */
/*                                                                          */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
char getesctrx(char inp)
{
   char c;

   switch(inp)
  {
      case 'n':   c = 0x0a;
      break;
      case 't':   c = 0x09;
      break;
      case 'r':   c = 0x0d;
      break;
      case 'f':   c = 0x0c;
      break;
      default:    c = inp;
      break;
   }
  return(c);
}
