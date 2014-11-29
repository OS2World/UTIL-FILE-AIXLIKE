/* @(#)13	1.1  src/xx/itstypes.h, aixlike.src, aixlike3  9/27/95  15:47:00 */
#ifndef INCLUDED_ITSTYPES /* Include semaphore */
#define INCLUDED_ITSTYPES
/*-------------------------------------------------------*/
/* itstypes.h: This header file contains any "base data types" for the ITS
   system, types which are used throughout our code. NOTE THAT I have to
   actively discard any prior definitions of these types to make this work
   reliably.
*/
#include <limits.h>                    /* Obtain compiler's data sizes */

/* Integer Types: We have a certain tension between the desire for portable
   code, and the need to minimize the size of our memory data pools (which
   can become substantial). The following types are defined in an attempt to
   address this; they adapt automatically to the best available match. Note
   the use of the ANSI C limits.h values to avoid having to code a case for
   every compiler we might use.

   WARNING: Byte order differs between Intel family, Risc6000, and probably
     other platforms we're interested in -- direct binary portability of data
     is NOT guaranteed even with these specific-length types.
*/
#undef INT16
#undef UINT16
#if UINT_MAX==0xFFFF                   /* If ints are only 16 bits */
  typedef                int  INT16;   /* use them for the INT16 types */
  typedef unsigned       int UINT16;
#define    INT16_MAX  INT_MAX
#define   UINT16_MAX UINT_MAX
#elif USHRT_MAX==0xFFFF                /* Else, shorts are probably 16 bits */
  typedef          short int  INT16;
  typedef unsigned short int UINT16;
#define   INT16_MAX  SHRT_MAX
#define  UINT16_MAX USHRT_MAX
#else
  /* If there is none... well, shorts are at least 16 bits and may be
     the best available alternative, but I'm going to deliberately
     leave these undefined to provoke an error and force us to consider
     this more carefully when the situation arises.
  */
#endif

#undef INT32
#undef UINT32
#if UINT_MAX==0xFFFFFFFF               /* If ints are 32 bits */
  typedef                int  INT32;   /* use them for the INT32 types */
  typedef unsigned       int UINT32;
#define   INT32_MAX  INT_MAX
#define  UINT32_MAX UINT_MAX
#elif ULONG_MAX==0xFFFFFFFF            /* Else, longs are probably 32 bits */
  typedef          long  int  INT32;
  typedef unsigned long  int UINT32;
#define   INT32_MAX  LONG_MAX
#define  UINT32_MAX ULONG_MAX
#else
  /* See above discussion. Longs will probably, but not certainly, be the
     best choice in this case.
  */
#endif
/*-------------------------------------------------------*/
#endif /*ndef INCLUDED_ITSTYPES  Include semaphore */
