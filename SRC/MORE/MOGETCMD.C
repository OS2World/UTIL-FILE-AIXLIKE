static char sccsid[]="@(#)55	1.1  src/more/mogetcmd.c, aixlike.src, aixlike3  9/27/95  15:44:56";
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

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include "more.h"

/*  MORE.   This module contains the code to get commands from the user
            during program execution.
                                                                            */
/*--------------------------------------------------------------------------*
 *  In more, the program pauses after each screen-full is displayed.  The   *
 *  user enters one or more characters that determine what happens next.    *
 *  Here's what the AIX Commands Reference has to say about the commands:   *
 *                                                                          *
 * Command Sequences:                                                       *
 *     Other command sequences that can be entered when the more command    *
 *     pauses are as follows (/ is an optional integer option, defaulting   *
 *     to 1):                                                               *
 *                                                                          *
 *     i|<space>    Displays / more lines (or another screen full of text   *
 *                  if no option is given)                                  *
 *                                                                          *
 *     ctrl-D       Displays 11 more lines (a "scroll").  If i is given,    *
 *                  the scroll size is set to i/.   Same as d.              *
 *                                                                          *
 *     d            Displays 11 more lines (a "Scroll").  Same as ctrl-D.   *
 *                                                                          *
 *     .i|z         Same as typing a space except tht if I/ is present, it  *
 *                  becomes the new window size.                            *
 *                                                                          *
 *     i|s          Skips I/ lines and prints a full screen of lines.       *
 *                                                                          *
 *     i|f          Skips I screens and prints a full screen of lines       *
 *                                                                          *
 *     i|b          Skips back I screens and prints a full screen of lines  *
 *                                                                          *
 *     i|ctrl-B     Skips back / screens and prints a full screen of lines. *
 *                                                                          *
 *     q or Q       Exits from more command.                                *
 *                                                                          *
 *     =            Displays the current line number                        *
 *                                                                          *
 *     v            Invokes vi editor <Not implemented in OS/2 version >    *
 *                                                                          *
 *     h            This is the help command.  Provides a description for   *
 *                  all the more commands.                                  *
 *                                                                          *
 *     i|/expr      Searches for the i/-th occurance of the regular expression
 *                  Expression.  If there are less than I occurances of     *
 *                  Expression/, and the input is a file (rather than a pipe),
 *                  the position in the file remains unchanged.  Otherwise, *
 *                  a full screen is displayed, starting two lines prior to the
 *                  location of the expression.                             *
 *                                                                          *
 *     i|n          Searches for the I/th occurance of the last regular     *
 *                  expression entered.  Goes to the point from which the   *
 *                  last search started.  If a search has not been performed*
 *                  in the current file, this sequence causes the more      *
 *                  command to go to the beginning of the file.             *
 *                                                                          *
 *     !command     Invokes a shell.  The characters '%' and '!' in the     *
 *                  command sequence replaced with the current file name    *
 *                  and the previous shell command respectively.  If there  *
 *                  is no current file name, '%' is not expanded.  The      *
 *                  sequences "\%" and "\!" are replaced by"%" and "!"      *
 *                  respectively.                                           *
 *                                                                          *
 *     i:n          Skips to the I/th next file given in the command line.  *
 *                  If (n is not valid, skips to the last file.             *
 *                                                                          *
 *     .i:p         Skips to the I/th previous file given in the command line.
 *                  If this sequence is entered while the more command is in*
 *                  the middle of printing a file, the more command goes back
 *                  to the beginning of the file.                           *
 *                                                                          *
 *     :f           Displays the current file name and line number.         *
 *                                                                          *
 *     :q or :Q     Exits (same as q or Q).                                 *
 *                                                                          *
 *     .            (dot)Repeats the previous command.                      *
 *                                                                          *
 *     When any of the command sequences are entered, they begin to process *
 *     immediately.  It is not necessary to type a carriage return.         *
 *                                                                          *
 * The more command sets terminals to Noecho mode.  This enables the output to
 * be continuous and only displays the / and ! commanda on your terminal    *
 * as your type.                                                            *
 *                                                                          *
 * To the AIX-specified command set, I have added PgUp, PgDn, UpArrow       *
 * and DownArrow, all with the intuitive meanings.                          *
 *--------------------------------------------------------------------------*/

char mgcexpressbuff[257];

int getcmd(unsigned int *qtyP, struct buff_info *bi, struct window_parms *wp)
{
   unsigned char c;
   char *get_express(struct buff_info *, struct window_parms *, char);
   char *cmd;

   *qtyP = 0;
   c = (unsigned char)getch();

   switch  (c)
   {
                    /* if it begins with a digit, it must be a quantity */
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':   do
                    {
                      *qtyP *= 10;
                      *qtyP +=  (unsigned int)c - 48;
                      c = (unsigned char)getch();
                    }    while (isdigit(c));

      break;
   }
                 /* Now we have the quantity, if any.  Look for the command. */
   switch (c)
   {             /* first, lets look at the extended combinations */
      case 0x00:
      case 0xE0:  c = (unsigned char)getch();
                  switch (c)
                  {
                     case PGUP:    return(MOVE_UP_SCREEN);
                     break;

                     case PGDN:    return(MOVE_DOWN_SCREEN);
                     break;

                     case UPARROW: if (*qtyP == 0)
                                     *qtyP = 1;
                                   return(MOVE_UP);
                     break;

                     case DNARROW: if (*qtyP == 0)
                                     *qtyP = 1;
                                   return(MOVE_DOWN);
                     break;

                     default:      putchar(BEL);
                                   return(NOCMD);
                     break;
                  }
      break;  /* case 0x00 or 0xE0 */

                 /* second, look at those that start with a colon */
      case COLON: c = (unsigned char)getch();
                  switch (c)
                  {
                     case 'n':     return(FSKIP_DOWN);
                     break;

                     case 'p':     return(FSKIP_UP);
                     break;

                     case 'f':     return(DFILENAME);
                     break;

                     case 'Q':
                     case 'q':     return(QUIT);
                     break;

                     default:      putchar(BEL);
                                   return(NOCMD);
                     break;
                  }
      break;

      case SPACE: if (*qtyP == 0)
                    return(MOVE_DOWN_SCREEN);
                  else
                    return(MOVE_DOWN);
      break;

      case LF:
      case CR:    if (*qtyP == 0)
                    *qtyP = 1;
                  return(MOVE_DOWN);
      break;

      case CTLD:
      case 'd':   return(SCROLL_DOWN);
      break;

      case 'z':   return(CHG_SIZE);
      break;

      case 'f':   return(SSKIP_DOWN);
      break;

      case 's':   return(LSKIP_DOWN);
      break;

      case 'b':   return(SSKIP_UP);
      break;

      case CTLB:  return(SSKIP_UP);
      break;

      case DEL:
      case 'q':
      case 'Q':   return(QUIT);
      break;

      case '=':   return(DLINENO);

      case 'h':   return(HELP);
      break;

      case 'n':   if (wp->lastexpr == NULL)
                    return(NOCMD);
                  else
                    return(FINDNTH);
      break;

      case SLASH: cmd = get_express(bi, wp, (char)SLASH);
                  if (strlen(cmd))
                    {
                      if (wp->lastexpr)
                        free(wp->lastexpr);
                      wp->lastexpr = (char *)malloc(strlen(cmd) + 1);
                      if (wp->lastexpr)
                        strcpy(wp->lastexpr, cmd);
                      return(FINDEXPR);
                    }
                  return(NOCMD);
      break;

      case XCLM:  cmd = get_express(bi, wp, (char)XCLM);
                  if (strlen(cmd))
                    if (strcmp(cmd, XCLMS) == 0)
                      return(EXECCMD);
                    else
                      {
                        if (wp->lastcmd)
                          free(wp->lastcmd);
                        wp->lastcmd = (char *)malloc(strlen(cmd) + 1);
                        if (wp->lastcmd)
                          strcpy(wp->lastcmd, cmd);
                        return(EXECCMD);
                      }
                  else
                    if (bi->file_size == 0)
                      return(EXECCMD);
                    else
                      return(NOCMD);
      break;

      case DOT:   return(REPEAT);
      break;

      default:    putchar(BEL);
                  return(NOCMD);
      break;
   }

   putchar(BEL);
   return(NOCMD);
}


/* ------------------------------------------------------------------------ */
/* get_express                                                              */
/*   This uses gets() to get the expression data, which really is not       */
/*   acceptable.  It will have to do until I write some simple editing      */
/*   functions.                                                             */
/* ------------------------------------------------------------------------ */
char *get_express(struct buff_info *bi, struct window_parms *wp, char c)
{
   if (bi->stream == stdin)    /* return null string if input is stdin */
     {
       *mgcexpressbuff = 0;
       return(mgcexpressbuff);
     }
   setcursorpos(wp->window_rows - 1, 0);
   clear_eol(wp->window_rows - 1, 0, wp);
   putchar(c);
   setcursorpos(wp->window_rows - 1, 1);
   return (gets(mgcexpressbuff));
}
