static char sccsid[]="@(#)61	1.1  src/more/moregrep.c, aixlike.src, aixlike3  9/27/95  15:45:08";
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

/* moregrep.c */

/*  These routines handle searching for a regular expression within a
    file being browsed using MORE.  The search direction is always down.

    If gsrch() is called, the search starts with the current Tols.
    If gnsrch is called, the search starts on the third line below
    current Tols (this is so that we don't keep finding the same one
    over and over -- each search tries to put its target on the third
    line of the screen.)
*/

#include <stdio.h>
#include <sys/types.h>
#include "more.h"
#include <greputil.h>
#include <grep.h>

/* -------------------------------------------------------------------------- */
/*   gsrch                                                                    */
/* -------------------------------------------------------------------------- */
long gsrch(char *pattern, int n, struct buff_info *bi, struct window_parms *wp)
{
    int i, found;
    unsigned char *line;
    off_t saveTols, saveTob, savePbols, stub;
    long linecnt = 0;
    unsigned char patbuf[256];

    saveTols = bi->Tols;
    saveTob  = bi->Tob;
    savePbols = bi->Pbols;
    if ( (i = compile_reg_expression(pattern, patbuf, 256)) > 256)
      {
        myerror(0, "gsrch", "compiling pattern");
        return(0l);
      }
    for (i = 0; i < n; i++)
      {
        while (1)
          {
            line = bi->bufaddr + (bi->Tols - bi->Tob);
            if (found = natural_reg_expression((unsigned char *)line,
                                               (unsigned char *)patbuf,
                                               (int)wp->display_cols))
              break;
            linecnt++;
            if ( (stub = oneLineDown(bi->Tols, bi, wp)) == LEOF)
              {
                restoreTols(saveTob, saveTols, savePbols, bi);
                break;
              }
            else
              bi->Tols = stub;
          }

        if (!found)
          return(0);
        break;
      }
    stub = bi->Tols;
    moveTolsUp(bi, wp, 2l);
    for (i = 2; i < wp->display_rows; i++)
       {
          if ( (stub = oneLineDown(stub, bi, wp)) == LEOF)
            {
               bi->Pbols = bi->file_size;
               break;
            }
          bi->Pbols = stub;
       }
    return( (linecnt > 0) ? linecnt : 1);
}


/* -------------------------------------------------------------------------- */
/*   gnsrch                                                                   */
/* -------------------------------------------------------------------------- */
long gnsrch(char *pattern, int n, struct buff_info *bi, struct window_parms *wp)
{
    off_t saveTob, saveTols, savePbols;
    long rc;

    saveTob = bi->Tob;
    saveTols = bi->Tols;
    savePbols = bi->Pbols;
    moveTolsDown(bi, wp, 3l);
    if (!(rc = gsrch(pattern, n, bi, wp)))
      {
        restoreTols(saveTob, saveTols, savePbols, bi);
        return(0l);
      }
    else
      return(rc);
}

/* -------------------------------------------------------------------------- */
/*   natural_reg_expression                                                   */
/* -------------------------------------------------------------------------- */
int natural_reg_expression(unsigned char *line, unsigned char *pattern,
                           int maxlen )
{
   unsigned char    *lineP;
   subpat           spd;
   int len;

   spd.numsubpats = spd.activesubpats = 0;
   len = maxlen;
   for (lineP = line; *lineP && len && *lineP!=CR && *lineP!=LF; lineP++, len--)
     {
       spd.numsubpats = spd.activesubpats = 0;
       if ( pmatch(lineP, pattern, &spd) )
         return(1);
     }
   return(0);
}
