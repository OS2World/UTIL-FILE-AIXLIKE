static char sccsid[]="@(#)42	1.1  util/myerror/myerror.c, aixlike.src, aixlike3  9/27/95  15:53:14";
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


/*------------------------------------------------------------------------*/
/*                                                                        */
/*  myerror                                                               */
/*                                                                        */
/*  Display an error message.  The format of the message is               */
/*      name of this program (if filled in by the main program)           */
/*      (area) - Dos or program function which detected the error         */
/*      rc - numerical return code                                        */
/*      (details) - information about the object involved in the error    */
/*      text - a description of the error condition.                      */
/*                                                                        */
/*  Calling sequence:                                                     */
/*      extern char *myerror_pgm_name;                                    */
/*                                                                        */
/*      myerror_pgm_name = "myprogram";                                   */
/*      myerror(rc, area, details);                                       */
/*                                                                        */
/*  where                                                                 */
/*      myerror_pgm_name is a pointer to a string representing the        */
/*                       name of the calling program.                     */
/*                                                                        */
/*      rc               is the OS/2 return code (integer).               */
/*                                                                        */
/*      area             is a pointer to a string representing the area   */
/*                       of the program or the DOS function that detected */
/*                       the error.                                       */
/*                                                                        */
/*      details          is a pointer to a string containing additional   */
/*                       information about the error, such as a file name */
/*                       or, perhaps, the amount of space requested in a  */
/*                       malloc.                                          */
/*------------------------------------------------------------------------*/

#include <stdio.h>
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>

char *myerror_pgm_name = NULL;

void myerror(int rc, char *area, char *details)
{
   if (myerror_pgm_name != NULL)
     fprintf(stderr,
             "%s (%s) rc=%d (%s): ", myerror_pgm_name, area, rc, details);
   else
     fprintf(stderr,
             "(%s) rc=%d (%s): ", area, rc, details);
              /*--------------------------------------------------------------*/
              /* Figure out the appropriate text based on return code.        */
              /*--------------------------------------------------------------*/
   switch(rc)
   {
//   case NO_ERROR:                printf("No error - good return code\n");
//      break;
   case ERROR_FILE_NOT_FOUND:    printf("File not found\n");
      break;
   case ERROR_NO_MORE_FILES:     printf("File not found\n");
      break;
   case ERROR_PATH_NOT_FOUND:    printf("Path not found \n");
      break;
   case ERROR_INVALID_HANDLE:    printf("Bad search handle\n");
      break;
   case ERROR_INVALID_DRIVE:     printf("Invalid drive specified\n");
      break;
   case ERROR_ACCESS_DENIED:     printf("Access denied\n");
      break;
   case ERROR_NOT_DOS_DISK:      printf("Invalid disk format\n");
      break;
   case ERROR_INVALID_PARAMETER: printf("Pgm error - parm\n");
      break;
   case ERROR_DRIVE_LOCKED:      printf("Cannot access disk\n");
      break;
   case ERROR_BUFFER_OVERFLOW:   printf("Pgm error - buffer size\n");
      break;
   case ERROR_NO_MORE_SEARCH_HANDLES: printf("Out of search handles\n");
      break;
   case ERROR_FILENAME_EXCED_RANGE: printf("File name too big\n");
      break;
   case ERROR_SHARING_VIOLATION: printf("Sharing violation\n");
      break;
   case ERROR_CURRENT_DIRECTORY: printf("Can't delete current directory\n");
      break;
   case ERROR_NOT_ENOUGH_MEMORY: printf("Not enough memory for malloc\n");
      break;
   default:                      printf("Unexpected error\n");
   }
}
