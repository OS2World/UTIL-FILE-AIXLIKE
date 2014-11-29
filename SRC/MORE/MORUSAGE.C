static char sccsid[]="@(#)63	1.1  src/more/morusage.c, aixlike.src, aixlike3  9/27/95  15:45:12";
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

/* The MORE command

   This module displays the help panel when -? or an invalid parameter is
   entered on the command line.                                             */
/*--------------------------------------------------------------------------*
 *  tell_usage                                                              *
 *                                                                          *
 *--------------------------------------------------------------------------*/

void tell_usage()
{
printf("more - Copyright IBM, 1990, 1992\n");
printf("Displays continuous text one screen at a time on a display screen.\n");

printf("Syntax\n");

printf("         more -cdflsu -n +Number    file file ...\n");
printf("          |                 |\n");
printf("         page            +/Pattern\n");

printf("The more command displays continuous text one screen at a time. It pauses\n");
printf("after each screen and prints the word More at the bottom of the screen.\n");
printf("If (you then press a carriage return, the more command displays an\n");
printf("additional line.  If you press the space bar, the more command displays\n");
printf("another screen of text.\n\n");

printf("Flags:\n");
printf("    -c   Keeps the screen from scrolling and makes it easier to read\n");
printf("         text while the more command is writing to the terminal.\n");

printf("    -d   Prompts the user to continue, quit, or obtain help.\n");

printf("    -f   Causes the more command to count logical lines, rather than\n");
printf("         screen lines.\n");

printf("    -l   Does not treat control-L (form feed) in a special manner.  If the\n");
printf("         -l flag is not supplied, the more command pauses after any line\n");
printf("         that contains a control-L.\n");

printf("    -n   (Where n is an integer) specified the number of lines in the\n");
printf("         window.\n");

printf("    -s   Squeezes multiple blank lines from the output to produce only one\n");
printf("         blank line.\n");

printf("    -u   Suppresses the more command from underlining or creating\n");
printf("         stand-out mode for underlined information in a source file.\n");

printf("    +Number     Starts at the specified line Number.\n");

printf("    +/Pattern   Starts two lines before the line containing the regular\n");
printf("                expression Pattern.\n");
}
