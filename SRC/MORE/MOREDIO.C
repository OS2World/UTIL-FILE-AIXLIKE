static char sccsid[]="@(#)60	1.1  src/more/moredio.c, aixlike.src, aixlike3  9/27/95  15:45:06";
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
/* @4 03/16/92 Still had trouble with the ends of piped-in files         */
/*-----------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#define INCL_BASE
#include <os2.h>
#include "more.h"


/*   The MORE command

     Modules that do disk i/o                                               */

/*--------------------------------------------------------------------------*
 *  openit                                                                  *
 *                                                                          *
 *--------------------------------------------------------------------------*/
int  openit(char *fn, struct buff_info **bipp, char *buf)          // @3c
{
   struct stat statb;
   struct buff_info *bi;                                           // @3a

   bi = *bipp;                                                     // @3a20
   if (*bipp == (struct buff_info *)NULL)
     {
       bi = (struct buff_info *)malloc(sizeof(struct buff_info));
       if (!bi)
         {
           myerror(ERROR_NOT_ENOUGH_MEMORY, "Allocating buff_info block",
                    fn);
           return(NO);
         }
       else
         {
           *bipp = bi;
           bi->stream = (FILE *)NULL;
           bi->status = 0;
           bi->bufaddr = buf;
           bi->bufsize = BUFSIZE;
           bi->lineno = 0;
         }
     }

   if (bi->stream == (FILE *)NULL)                                // @3a2
     {
       if (stat(fn, &statb))
          return(NO);

       bi->file_size = statb.st_size;
       bi->filename = fn;
       bi->stream = fopen(fn, "rb");
       if (bi->stream)   /* it worked */
         {
           bi->Tob = 0;        /* Offset in file of first byte in buffer */
           bi->Tols = 0;       /* Offset in file of top of current window */
           bi->Pbols = 0;      /* Offset in file of start of the line that */
                           /* immediately follows the current window */
                        /* Fill up a buffer and get the count of Bytes in buff*/
           bi->Bib = fread(bi->bufaddr, sizeof(char), (size_t)BUFSIZE, bi->stream);
                            /* (It's OK for the file to be empty) */
           if (bi->Bib < BUFSIZE)                                 // @4a2
             bi->file_size = bi->Bib;
         }
       else
         {
           printf("%s: Could not open file %s\n", myerror_pgm_name, fn);
           return(NO);
         }
     }
   else         /* file is already open */                       // @3a
     {
       fseek(bi->stream, bi->Tob, SEEK_SET);
       bi->Bib = fread(bi->bufaddr, sizeof(char), (size_t)bi->Bib, bi->stream);
     }
   return(YES);
}


/*-----------------------------------------------------------------------*/
/*  refresh_buffer()                                                     */
/*  refreshes the buffer under these circumstances:                      */
/*      . buffer window needs to move DOWN in file                       */
/*      . buffer window needs to move UP in file                         */
/*                                                                       */
/*  Normally we'll try to move the buffer 1/2 of its capacity.           */
/*  Exception are:                                                       */
/*         - We will keep top            alignment with the file.        */
/*-----------------------------------------------------------------------*/

int  refresh_buffer(struct buff_info *bi, int cmd)
{
   off_t  amt, left, saveleft;
   size_t bytesread;
   char *p, *q;
                       /* amt is guaranteed to be convertible to size_t */

   if (bi->file_size == bi->Bib)
     return(NO);                 /* everything is already in the buffer */
   if ( (cmd == DOWN) && ((bi->Tob + bi->Bib) == bi->file_size) )
     return(NO);                 /* can't scroll down any further */
   if ( (cmd == UP) && (bi->Tob == 0) )
     return(NO);                 /* can't scroll up any further */

   amt = BUFSIZE/2;         /* we're going to flip forward or back half a buf */
   if (bi->Bib < amt)          /* amt = MIN(BUFSIZE/2, bi->Bib) */
     amt = bi->Bib;
   if(cmd == DOWN)              /* if direction is down the buffer */
     {
       left = bi->Bib - amt;       /* number of useful bytes in buffer */
       saveleft = left;
       bi->Bib -= left;            /* now there's less data in buffer */
       bi->Tob += left;            /* and the top is further along in the file */
       q = bi->bufaddr;            /* top of buffer */
       p = q + amt;                /* top of useful data */
       for (; left; left--)         /* move useful data to top of buffer */
         *q++ = *p++;
       bytesread = fread(bi->bufaddr + amt,     /* read into 2d half of buf */
                         sizeof(char),
                         (size_t)amt,            /* read a half-buffer full */
                         bi->stream);
       bi->Bib += bytesread;                           // @4a
       if (bytesread == 0)         /* keep track of the fact we didn't receive */
         bi->status = -1;          /* any data that time */
       if (bytesread < (size_t)amt)                            // @4a2
         bi->file_size = bi->Tob + saveleft + bytesread;
     }
   else
     if (cmd == UP)
       {
         if (amt > bi->Tob)      /* avoid scrolling beyond TOF */
           amt = bi->Tob;
         left = bi->Bib - amt;
         q = bi->bufaddr;
         p = q + amt;
         for (; left; left--)
           *p++ = *q++;                /* move the other direction */
         bi->Bib += amt;
         bi->Tob -= amt;
         fseek(bi->stream, bi->Tob, SEEK_SET);
         fread(bi->bufaddr,           /* read into top of buf */
               sizeof(char),
               (size_t)amt,
               bi->stream);
       }

}

/*--------------------------------------------------------------------------*
 *  closeit                                                                 *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void closeit(struct buff_info *bi)
{
   if (bi->file_size)
     fclose(bi->stream);
   bi->stream = (FILE *)NULL;                               //@3a
}
