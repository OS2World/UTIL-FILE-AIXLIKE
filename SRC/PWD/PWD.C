static char sccsid[]="@(#)72	1.1  src/pwd/pwd.c, aixlike.src, aixlike3  9/27/95  15:45:31";
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

/*
   pwd

   Displays the path name of the working directory.

   Syntax:
            pwd

   The pwd command writes to standard output the full path name of your
   current directory (from the root directory <of the current drive>.
   All directories are seperated by a < \ (backslash>.  <The current
   drive is represented first, followed by a colon, then> the root directory.
   The last directory names is your current directory.

*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <direct.h>
#include <stdio.h>

char pathbuf[_MAX_PATH + 1];

int main(int argc, char **argv)                                     /* @A1c */
{
    if (getcwd(pathbuf, _MAX_PATH) == NULL)
      {                                                             /* @A1a */
        printf("pwd() failed.\n");
        return(-1);                                                 /* @A1a */
      }                                                             /* @A1a */
    else
      printf("%s\n", pathbuf);
    return(0);                                                      /* @A1a */
}
