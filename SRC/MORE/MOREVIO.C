static char sccsid[]="@(#)62	1.1  src/more/morevio.c, aixlike.src, aixlike3  9/27/95  15:45:10";
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <conio.h>
#define INCL_VIO
#define  INCL_NOPM
#include <os2.h>
#include "more.h"

/*  The MORE command

    Routines that have knowledge of Vio interfaces                      */

/*-----------------------------------------------------------------------*/
/*  display_help                                                         */
/*-----------------------------------------------------------------------*/
void display_help(char *HelpLines[], char *AnyKey, struct window_parms *wp)
{
   int i, scrnline;
   char mybl[BLSIZE+1];

   for (i = scrnline = 0; strcmp(HelpLines[i], ""); i++)
     {
       VioWrtCharStr(HelpLines[i], strlen(HelpLines[i]), scrnline++, 0, (HVIO)0);
       if (scrnline == wp->display_rows)
         {
           VioWrtCharStr(AnyKey, strlen(AnyKey), wp->window_rows - 1, 0, (HVIO)0);
           getch();
           clear_window(wp);
           scrnline = 0;
         }
      }
   if (scrnline > 0)
     {
       for (i = 0; i < BLSIZE; mybl[i++] = SPACE);
       strncpy(mybl, AnyKey, strlen(AnyKey));
       VioWrtCharStr(mybl, BLSIZE, wp->window_rows - 1, 0, (HVIO)0);
       getch();
     }
}

/*-----------------------------------------------------------------------*/
/*  put_bline - output the status line on the bottom line of the window  */
/*-----------------------------------------------------------------------*/
void put_bline(char *bline, int blsize, struct window_parms *wp )
{
   VioWrtCharStr(bline, blsize, wp->window_rows - 1, 0, (HVIO)0);
}

/*-----------------------------------------------------------------------*/
/*  putline                                                              */
/*-----------------------------------------------------------------------*/
void putline(int scrnline, struct window_parms *wp)
{
   VioWrtCharStr(wp->linebuff, wp->linelen, scrnline, 0, (HVIO)0);
}

/*--------------------------------------------------------------------------*
 *  getvmode                                                                *
 *                                                                          *
 *--------------------------------------------------------------------------*/
getvmode(int *rows, int *cols)
{
    int rc;
    VIOMODEINFO vmi;
    void myerror(int, char *, char *);

    if (rows)
      *rows = 0;
    if (cols)
      *cols = 0;
    vmi.cb = sizeof(VIOMODEINFO);
    if (rc = VioGetMode(&vmi, 0))
         {
            myerror(rc, "init", "VioGetMode");
            return(BAILOUT);
         }
    else
         {
           if (rows)
             *rows = vmi.row;
           if (cols)
             *cols = vmi.col;
         }
    return(1);
}

/*--------------------------------------------------------------------------*
 *  get_cell_sttributes_at_cursor                                           *
 *                                                                          *
 *--------------------------------------------------------------------------*/
char get_cell_attributes_at_cursor()
{
   int rc, row, col;
   char attr[4];
   int len;

   len = 4;
   getcursorpos(&row, &col);
   if (rc = VioReadCellStr(attr, (unsigned short *) &len, row, col, (HVIO)0))
     {
       printf("Couldn't read attributes at cursor (%d, %d) rc %d\n",row,col,rc);
       return(DEFAULTATTR);
     }
   else
     return(attr[1]);
}

/*--------------------------------------------------------------------------*
 *  getcursorpos                                                            *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void getcursorpos(int *row, int *col)
{
   int rc;

   if (rc = VioGetCurPos((unsigned short *)row, (unsigned short *)col, (HVIO)0))
     printf("Failed getting cursor position. rc = %d", rc);
}


/*--------------------------------------------------------------------------*
 *  setcursorpos                                                            *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void setcursorpos(int row, int col)
{
   int rc;

   if (rc = VioSetCurPos(row, col, (HVIO)0))
     printf("Failed setting cursor position. rc = %d", rc);
}

/*--------------------------------------------------------------------------*
 *  clear_window                                                            *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void clear_window(struct window_parms *wp)
{
   int rc;

   if (rc = VioScrollDn(wp->top_row,          wp->left_col,
                        wp->display_rows - 1, wp->display_cols - 1,
                        wp->display_rows,     wp->clearcell,       (HVIO)0))
     printf("Failed trying to clear display area . rc = %d\n",  rc);
}

/*--------------------------------------------------------------------------*
 *  clear_screen                                                            *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void clear_screen(struct window_parms *wp)
{
   int  rc;

   if (rc = VioScrollDn(0                  ,  0,
                        wp->window_rows - 1,  wp->display_cols - 1,
                        wp->window_rows,      wp->clearcell,       (HVIO)0))
     printf("Failed trying to clear window,  rc = %d\n",  rc);
}

/*--------------------------------------------------------------------------*
 *  clear_eol                                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void clear_eol(int row, int col, struct window_parms *wp)
{
   int rc, acol, arow;

   acol = wp->left_col + col;
   arow = wp->top_row + row;
   if (rc = VioWrtNCell(wp->clearcell, wp->display_cols - col, arow, acol, (HVIO)0))
     printf("Failed clearing EOL (%d, %d) rc = %d\n", arow, acol, rc);
}

/*--------------------------------------------------------------------------*
 *  PhysScrollUp                                                            *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void PhysScrollUp(unsigned int lines, struct window_parms *wp)
{
   int rc;

   if (rc = VioScrollUp(wp->top_row,          wp->left_col,
                        wp->display_rows - 1, wp->display_cols - 1,
                        (int)lines,          (PBYTE)&wp->clearcell,    (HVIO)0))
     printf("Failed scrolling up %d. rc = %d\n", lines, rc);
}

/*--------------------------------------------------------------------------*
 *  PhysScrollDn                                                            *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void PhysScrollDn(unsigned int lines, struct window_parms *wp)
{
   int rc;

   if (rc = VioScrollDn(wp->top_row,          wp->left_col,
                        wp->display_rows - 1, wp->display_cols - 1,
                        (int)lines,          (PBYTE)&wp->clearcell,   (HVIO)0))
     printf("Failed scrolling down %d. rc = %d\n", lines, rc);
}

/*-----------------------------------------------------------------------*/
/*  file_has_disappeared                                                 */
/*-----------------------------------------------------------------------*/
void file_has_disappeared(char *bline, char *AnyKey, struct window_parms *wp)
{
   VioWrtCharStr("File no longer exists       ", 28, wp->window_rows - 1,
                 0, (HVIO)0);
   VioWrtCharStr(AnyKey, strlen(AnyKey), wp->window_rows - 1, 28, (HVIO)0);
   getch();
   put_bline(bline, BLSIZE, wp);
}
