static char sccsid[]="@(#)64	1.1  src/more/movetols.c, aixlike.src, aixlike3  9/27/95  15:45:14";
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

/* These routines handle moving our display window within the buffer, which */
/* is itself a window into the file.  For the most part, we can ignore the  */
/* additional complication of the buffer; when we need to, we can refresh   */
/* it.                                                                      */
/*                                                                          */
/* The two routines are:                                                    */
/*          moveTolsDown, which adjusts the top of our view window down     */
/*             in the file by a requested number of lines.  We may not      */
/*             actually move it that far.  In fact, the bottom of our       */
/*             view window is already at the bottom of the file, we won't   */
/*             move it at all (returning and EOF indication instead).       */
/*             We will never move the top of the window past the point      */
/*             where the bottom of the window is not in the file.           */
/*                                                                          */
/*          moveTolsUp, which is pretty much the same thing, except we are  */
/*             moving the top of the window in the other direction.  Here,  */
/*             we will not move the top beyond the top of the file.         */
/*                                                                          */
/* These routines need to know how to count lines for two reasons:  first,  */
/* in order to move the 'right' line distance; and second, to be able to    */
/* to determine the location of the bottom of the window, given the top.    */
/* I'm not sure I understand how the unix more is supposed to count lines   */
/* -- there's a command line switch that allows you to use 'logical lines   */
/* rather than screen lines'. I think that means that certain control       */
/* sequences (for underlining, perhaps) are ignored.  So this routine       */
/* calls oneLineDown,    which knows all that stuff.                        */
/*                                                                          */
/* We don't want our starting point for each line to change, so we guarantee*/
/* to oneLineDown and oneLineUp that                                        */
/*         . There is a full line left in the buffer, or                    */
/*         . End-of-buffer means end-of-file                                */
/*                                                                          */
/* Finally, there's a routine restoreTols which restores a saved state      */
/* by refreshing the buffer and restoring the state variables in structure  */
/* buff_info.   This routine is necessary because the regular expression    */
/* search routine may change the contents of the buffer, but still need     */
/* to return to the original state if no matching expression is found.      */
/*                                                                          */
/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/* @1 03/16/92 decided to ignore CRs, do eol on LF only.                 */


#include <stdlib.h>     /* for malloc */
#include <stdio.h>      /* for FILE */
#include <sys/types.h>  /* for off_t */
#define  INCL_BASE      /* for error constants */
#define  INCL_NOPM
#include <os2.h>
#include "more.h"       /* for structures */

/* ------------------------------------------------------------------------- *
 *   moveTolsDown(n)                                                         *
 *                                                                           *
 *   If Pbols is at eof, move 0 lines.                                       *
 *                                                                           *
 *   Locate new Tols (Top of logical screen) n lines down from old Tols      *
 *   If eof encountered after m < n lines                                    *
 *     Set new Tols at old Tols or display_rows back, whichever is greater.  *
 *                                                                           *
 *   Locate new Pbols (Past bottom of logical screen) display_rows below new *
 *   Tols                                                                    *
 *   If eof encountered after m < display_rows lines                         *
 *     If newTols is at tof                                                  *
 *       set Pbols at eof                                                    *
 *     else                                                                  *
 *       Locate new Tols (display_rows - m) back, set Pbols at eof           *
 *   else                                                                    *
 *     set new Pbols at located point.                                       *
 *                                                                           *
 * ------------------------------------------------------------------------- */
long moveTolsDown(struct buff_info *bi, struct window_parms *wp, long lines)
{
    long i, rec, original_line;
    off_t *sb, new_Tols_offset, new_Pbols_offset;


/*   If Pbols is at eof, move 0 lines.                                       */

    if (bi->file_size == 0)
      {
        if ( ((bi->Pbols - bi->Tob) >= bi->Bib) && (bi->status == -1) )
          return(0l);
      }
    else
      if (bi->Pbols == bi->file_size)
        return(0l);

    original_line = bi->lineno;
    sb = (off_t *)malloc(wp->display_rows * sizeof(off_t) + 1);
    if (sb)
      for (i = 0; i < wp->display_rows; sb[i++] = 0);
    else
      {
         myerror(ERROR_NOT_ENOUGH_MEMORY, "moveTolsDown", "allocating sb");
         return(0l);
      }

    new_Tols_offset = bi->Tols;   /* initialize new Tols */

/*   Locate new Tols (Top of logical screen) n lines down from old Tols      */

    sb[0] = new_Tols_offset;
    for (i = 0l; i < lines; i++)     /* One line at a time,     */
      {
        new_Tols_offset = oneLineDown(new_Tols_offset, bi, wp);

/*   If eof encountered after m < n lines                                    */
/*     Set new Tols at old Tols or display_rows back, whichever is greater.  */

        if (new_Tols_offset == LEOF)
          break;
        sb[(i+1) % wp->display_rows] = new_Tols_offset;
      }
    if (new_Tols_offset == LEOF)
      {
         if (i > wp->display_rows)
           {
              bi->Tols = sb[(i + 1) % wp->display_rows];
              bi->lineno += (i + 1 - wp->display_rows);
              i+=2;      /* Why, I'm not sure */
           }
         /* otherwise, Tols is unchanged */
      }
    else
      {
        bi->Tols = new_Tols_offset;
        bi->lineno += i;
      }

    rec = i;
    new_Pbols_offset = bi->Tols;

/*   Locate new Pbols (Past bottom of logical screen) display_rows below new */

    for (i = 0; i < wp->display_rows; i++)
      {
        new_Pbols_offset = oneLineDown(new_Pbols_offset, bi, wp);

/*   If eof encountered after m < display_rows lines                         */

        if (new_Pbols_offset == LEOF)
          break;
        rec++;
      }

    if (new_Pbols_offset == LEOF)

/*     If newTols is at tof                                                  */
/*       set Pbols at eof                                                    */

          if (bi->Tols == (off_t)0)
            bi->Pbols = bi->file_size;

/* else Locate new Tols (display_rows - m) back, set Pbols at eof           */

          else
            {
               bi->Tols = sb[rec % wp->display_rows];
               if (bi->lineno > (wp->display_rows - i) )
                 bi->lineno -= (wp->display_rows - i);
               else
                 bi->lineno = 0;
               if (bi->file_size == 0)
                 bi->Pbols = bi->Tob + bi->Bib;
               else
                 bi->Pbols = bi->file_size;
            }

/* else set new Pbols at located point.                                       */

    else
      bi->Pbols = new_Pbols_offset;

    free(sb);
    return(bi->lineno - original_line);
}


/* ------------------------------------------------------------------------- *
 *   moveTolsUp(n)                                                           *
 *                                                                           *
 *   If Tols is at tof, move 0 lines.                                       *
 *                                                                           *
 *   Locate new Tols (Top of logical screen) n lines up   from old Tols      *
 *   If tof encountered after m < n lines                                    *
 *     Set new Tols at tof                                                   *
 *                                                                           *
 *   Locate new Pbols (Past bottom of logical screen) display_rows below new *
 *   Tols                                                                    *
 *   If eof encountered after m < display_rows lines                         *
 *     If newTols is at tof                                                  *
 *       set Pbols at eof                                                    *
 *     else                                                                  *
 *       Locate new Tols (display_rows - m) back, set Pbols at eof           *
 *   else                                                                    *
 *     set new Pbols at located point.                                       *
 *                                                                           *
 * ------------------------------------------------------------------------- */
long moveTolsUp(struct buff_info *bi, struct window_parms *wp, long lines)
{

    long i, original_line;
    off_t new_Tols_offset, new_Pbols_offset;

/*   If Tols is at tof, move 0 lines.                                       */

    if (bi->Tols == (off_t)0)
      return(0l);

    original_line = bi->lineno;
    new_Tols_offset = bi->Tols;
    for (i = 0l; i < lines; i++)
      if ( (new_Tols_offset = oneLineUp(new_Tols_offset, bi, wp)) == LTOF)
        {
           bi->Tols = (off_t)0;
           bi->lineno = 0l;
           break;
        }
      else
        {
           bi->Tols = new_Tols_offset;
           bi->lineno--;
        }

    new_Pbols_offset = bi->Tols;
    for (i = 0; i < wp->display_rows; i++)
      {
        new_Pbols_offset = oneLineDown(new_Pbols_offset, bi, wp);
        if (new_Pbols_offset == LEOF)
          break;
      }

    if (new_Pbols_offset == LEOF)
      {
//        bi->Tols = 0;
//        bi->lineno = 0;
        if (bi->file_size == 0)
          bi->Pbols = bi->Tob + bi->Bib;
        else
          bi->Pbols = bi->file_size;
      }
    else
      bi->Pbols = new_Pbols_offset;

    return(original_line - bi->lineno);
}

/* ------------------------------------------------------------------------- *
 *    oneLineDown(fromwhere)                                                 *
 *                                                                           *
 *  If there may not be a full line left in the buffer, refresh the buffer.  *                         *
 *  (Don't worry about stuff like moving our current position outside the    *
 *  buffer - the routine that does the refreshing will take care of that.)   *
 *                                                                           *
 *  Walk through the line looking for CR, LF, Ctl-L or ctl-Z.  Expand tabs   *
 *  as you go.  When a line is complete, look for and return the start of    *
 *  the next line.  If eof is encountered at any time, return LEOF.          *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 * ------------------------------------------------------------------------- */
off_t oneLineDown(off_t from, struct buff_info *bi, struct window_parms *wp)
{
   char *p, *eob;
   off_t left;
   int incr, outsize, c, cc;

   left = bi->Bib - (from - bi->Tob);
   if ( left <= (wp->display_cols + 3) )
     refresh_buffer(bi, DOWN);

   eob = bi->bufaddr + bi->Bib;
   p = bi->bufaddr + from - bi->Tob;
   for (outsize = 0; p != eob ; p++)
     {
        switch (*p)
        {
            case CR:    /* Ignore all carriage returns: they're garbage @1a */
            break;

//            case LF:    c = (*p == CR) ? LF : CR;                    @1d5
//                        if (++p != eob)
//                          if (*p == (char)c)
//                            p++;
//                        return(bi->Tob + (p - bi->bufaddr));

            case LF:      return(bi->Tob + (p - bi->bufaddr) + 1);
            break;

            case CTLL:
            case CTLZ:  if (++p == eob)
                          if (outsize == 0)                 /* @1a */
                            return(LEOF);
                          else
                            {
//                             *--p = CR;    /* convert ctl-z to cr when a file */
//                             p++;          /* ends with ctl-z but no CR or LF */
                             *--p = LF;    /* convert ctl-z to cr when a file */
                            }
//                        return(bi->Tob + (p - bi->bufaddr));
                        return(bi->Tob + (p - bi->bufaddr) + 1);
            break;

            case TAB:   incr = TABEXPAN - (outsize % TABEXPAN);
                        if ((outsize + incr) >= wp->display_cols)
                          return(bi->Tob + (p - bi->bufaddr) - 1);
                        else
                          outsize += incr;
            break;

            default:    outsize++;
            break;
        }     /* end switch */

        if (outsize >= wp->display_cols)     /* If we've overrun a line */
          {
             p++;                            /* skip over any CR or LF  */
             cc = *p;                        /* immediately following   */
             if (cc==CR || cc==LF)           /* otherwise we get an     */
               if (++p != eob)               /* unwanted blank line     */
                 c = (cc == CR) ? LF : CR;
                 if (*p == (char)c)
                   p++;
             return(bi->Tob + (p - bi->bufaddr));
          }
     }    /* end for */
   return(LEOF);
}

/* ------------------------------------------------------------------------- *
 *    oneLineUp(fromwhere)                                                   *
 *                                                                           *
 *   This is a little different problem.  We want to break the line in the   *
 *   same place that it would be broken if scanned from the top down.  This  *
 *   is no problem if the line is less than wp->display_cols long, but longer*
 *   lines must be scanned to their beginning then broken down into screen-  *
 *   line-sized chunks.  Tabs are also a bit of a problem when we are        *
 *   moving up in the file.  A tab may represent as few as 1 and as many     *
 *   as TABEXPAN bytes, and we don't know how many until we scan back to     *
 *   the beginning of the line; therefore we keep a minimum/maximum length   *
 *   in order to minimize the work that needs to be done when a line has     *
 *   tabs.                                                                   *
 *                                                                           *
 *   We should always scan all the way back to the start of the previous     *
 *   line.  But we have to put some limit on it -- otherwise, we have to     *
 *   do a lot of buffer management work, and long lines could really slow    *
 *   performance.  So there's a constant, MAXUPLIN, which limits the         *
 *   number of bytes we'll go backwards looking for the start of the line.   *
 *                                                                           *
 * ------------------------------------------------------------------------- */
off_t oneLineUp(off_t from, struct buff_info *bi, struct window_parms *wp)
{
   char *p, *tob, *ls;
   off_t left;
   int size, minsize, maxsize, c, i, done;

   if (from == (off_t)0)
     return(LTOF);
   left =  (from - bi->Tob);
   if ( left <= MAXUPLIN)
     refresh_buffer(bi, UP);

   tob = bi->bufaddr;
   p = bi->bufaddr + from - bi->Tob;
   if (--p == tob)
     return(0l);
   else
     if (*p == CR || *p == LF)
       {
         c = (*p == CR) ? LF : CR;
         if (--p == tob)
           return(0l);
         else
           if (*p == (char)c)
             --p;
       }
     else
       if (*p == CTLL || *p == CTLZ)
         --p;

   done = NO;
   for (minsize = maxsize = 0; !done && minsize < MAXUPLIN && p != tob; p--)
     {
       switch(*p)
       {
          case CR:
          case LF:
          case CTLL:         p+=2;
                             done = YES;
          break;

          case TAB:          minsize += 1;
                             maxsize += TABEXPAN;
          break;

          default:           minsize++;
                             maxsize++;
          break;
       }
     }
   if (p == tob)
     return(0l);
   else                      /* most of the time, the line will plainly fit */
     if (maxsize <= wp->display_cols)
       return(bi->Tob + (p - bi->bufaddr));
     else                    /* but sometimes it won't be clear */
       if (minsize <= wp->display_cols)
         {
            for (i = size = 0; i < minsize; i++)
              if (p[i] == TAB)
                size += (TABEXPAN - (size % TABEXPAN));
              else
                size++;
            if (size <= wp->display_cols)   /* Now we know: it fits */
              return(bi->Tob + (p - bi->bufaddr));
         }
   /* if we got this far, we know the line is too long to print.  find */
   /* where the last line starts */

   i = 0;
   while (i < minsize)
     {
       ls = &p[i];
       for (size = 0; ((size + 1) % wp->display_cols) && (i <= minsize); i++)
         if (p[i] == TAB)
           size += (TABEXPAN - (size % TABEXPAN));
         else
           size++;
       i++;            /* ? */
     }
   return(bi->Tob + (ls - bi->bufaddr));
}

/* ------------------------------------------------------------------------- *
 *  restoreTols                                                              *
 *                                                                           *
 *  Unless we are reading from a pipe, seek to the saved top-of-Buffer,      *
 *  read from there, and reset the top, bottom and start state variables     *
 *  in bi.                                                                   *
 *                                                                           *
 * ------------------------------------------------------------------------- */
void restoreTols(off_t saved_Tob,   off_t saved_Tols,
                 off_t saved_Pbols, struct buff_info *bi)
{
    if (bi->file_size != 0)          /* if we're not reading from a pipe */
      {
        if (bi->Tob != saved_Tob)          /* and Tob has moved */
          {
             fseek(bi->stream, saved_Tob, SEEK_SET);
             bi->Bib = fread(bi->bufaddr, sizeof(char), (size_t)BUFSIZE, bi->stream);
          }
        bi->Tob = saved_Tob;
        bi->Tols = saved_Tols;
        bi->Pbols = saved_Pbols;
      }
}
