static char sccsid[]="@(#)67	1.1  src/printenv/printenv.c, aixlike.src, aixlike3  9/27/95  15:45:21";
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
    printenv

    Displays the values of the variables in the environment.

    Syntax
           prinvent [name [name ...

    The printenv command displays the values of the variables in the
    environment.  If a Name is specified, only its value is printed.
    If (a Name is not specified, the printenv command displays the
    current environment, one Name=Value per line.

    If a Name is specified and it is not defined in the environment,
    the printenv command returns exit status 1, else it returns status 0.

*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

main(int argc, char **argv)

{
   int i;
   char *p;

   if (argc == 1)         /* command called with no arguments */
     for (i = 0; environ[i]; printf("%s\n",environ[i++]));  /* print all */
   else                   /* some Names were specified */
     {
       for (i = 1; i < argc; i++)
         {
           strupr(argv[i]);         /* all OS/2 env names are upper case */
           if (p = getenv(argv[i]))
             printf("%s=%s\n", argv[i],p);  /* print just the names requested */
           else
             return(1);
         }
     }
   return(0);
}
