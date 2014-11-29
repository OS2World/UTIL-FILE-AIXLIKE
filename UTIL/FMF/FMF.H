/* @(#)33	1.1  util/fmf/fmf.h, aixlike.src, aixlike3  9/27/95  15:52:57 */
/************************************************/
/* Author: G.R. Blair                           */
/*         BOBBLAIR @ AUSVM1                    */
/*         bobblair@bobblair.austin.ibm.com     */
/*                                              */
/* The following code is property of            */
/* International Business Machines Corporation. */
/*                                              */
/* Copyright International Business Machines    */
/* Corporation,1990,1991.  All rights reserved. */
/************************************************/

/*-----------------------------------------------------------------------*/
/* Modification History:                                                 */
/*      Release 1       5/1/91                                           */
/*  @1 5.8.91 The logic for selecting files by attribute needed lots of  */
/*            work                                                       */
/*  @2 9.22.92 Add constant for "Network Access Denied"                  */
/*-----------------------------------------------------------------------*/

/* Header file containing the prototypes for the four external "find matching
file" procedures. */

#ifdef FSFTEST
#define EXTERN
#else
#define EXTERN extern
#endif

                   /* initialize a search.  Must be called first            */
EXTERN int fmf_init(char *filespec, unsigned mask, int searchmode);

                   /* return next file matching search pattern              */
EXTERN int fmf_return_next(char *CopyNameTo, int *CopyAttrTo);

                   /* Query how many threads can use this code simultaneously*/
EXTERN int fmf_query_max_threads(void);

                   /* Close a search & recover resources used for it        */
EXTERN void fmf_close(void);

/* Constants defining the kinds of files you may request  */
#define FMF_FILES              0x0001
#define FMF_DIRS               0x0002
#define FMF_DIRS_AND_FILES     (FMF_FILES | FMF_DIRS)
#define FMF_ALLMARK            0x8000
#define FMF_ALL_FILES          (FMF_FILES | FMF_ALLMARK)
#define FMF_ALL_DIRS           (FMF_DIRS | FMF_ALLMARK)
#define FMF_ALL_DIRS_AND_FILES (FMF_ALL_FILES | FMF_ALL_DIRS)
#define FMF_READONLY           0x0004
#define FMF_HIDDEN             0x0008
#define FMF_SYSTEM             0x0010

/* Constants defining the two search modes: walk-the-tree, or not */
#define FMF_SUBDIR    1
#define FMF_NO_SUBDIR 0

/* Return Codes.  */

#define FMF_NO_ERROR 0   /* If FMF_NO_ERROR is returned, then a file name has
                            been returned.                                    */

#define FMF_NO_MORE_FILES 18  /* If this is returned, there are no more files
                                 that match the search argument.  A file name
                                 has NOT been returned.                       */

#define FMF_PATH_NOT_FOUND 3  /* If this is returned by fmf_init, the search
                                 argument contains an invalid path name.      */

#define FMF_OUT_OF_THREAD_ROOM -1 /* If this is returned, too many threads are
                                   already trying to use fmf.                 */

#define FMF_OUT_OF_MEMORY  -2 /* Error trying to allocate memory.  Treat as
                                 a fatal error.                               */

#define FMF_NETWORK_ACCESS_DENIED   65                                 /* @2a */
/* Other values returned are system return codes */
