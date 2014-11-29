static char sccsid[]="@(#)38	1.1  src/df/df.c, aixlike.src, aixlike3  9/27/95  15:44:18";
/* Written by Peter J. Schwaller, PJS at RALVMC */
/* Modified by Bob Blair, 27Nov91:  The credit is Peter's, the bugs are mine. */

/*
df Command

Reports information about space on file systems

Usage
     df [-l|-M|-i|-s|-v] [FileSystem FileSystem... | File File...]
        \              / \                                      /
         \            /   \                                    /
          \..one of../     \................one of............/

The df command displays information about total space and available space on
a file system.  The FileSystem parameter specified the name of the device on
which the file system resides (the drive letter, under OS/2); in UNIX it
can also be the relative path name of a file system.  If you do not specify
the FileSystem parameter, the df command displays information for all mounted
file systems.  If a file or directory is specified, then the df command
displays information for the file system on which the file resides.

If the file system is being actively modified when the df command is run,
the free count may not be accurate.

Flags

    -i   Specified that the number of free and used i-nodes for the file
         system is displayed; this is the default when the specified file
         system is mounted. This flag is accepted but ignored for OS/2.

    -l   Specified that information on total K bytes, used space, free space,
         percentage of used space, and mount point for the file system
         is displayed.

    -M   Specifies that the mount point information for the file system is
         displayed in the second column.

    -s   Specifies a full search of the free block list.  This flag is
         accepted but ignored for OS/2.

    -v   Specifies that all information for the specified file system
         is displayed.  Same as -l.
*/
#include <idtag.h>    /* package identification and copyright statement */
#define     INCL_BASE
#define     INCL_DOSMISC
#include    <os2.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <ctype.h>
// #include    <getopt.h>
#include    "df.h"
// #include    "fmf.h"

#define DISABLE_ERRORPOPUPS FERR_DISABLEEXCEPTION | FERR_DISABLEHARDERR
#define ENABLE_ERRORPOPUPS  FERR_ENABLEEXCEPTION  | FERR_ENABLEHARDERR

struct FLAGS  flags;

/* Function prototypes */;
int init(struct FLAGS *fp, int argc, char **argv);
void show_header(struct FLAGS *fp);
int do_all_disks(struct FLAGS *fp, USHORT dn, ULONG logical_file_map);
int query_drive(char *spec, struct FLAGS *fp, USHORT dn, ULONG logical_drive_map);
void show_usage(void);
extern int getopt(int argc, char **argv, char *argstring);
extern int optind;           /* exposed by getopt */

main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
{
   int i;
   USHORT rc;
#ifdef I16
   USHORT drive_number = 0;
#else
   ULONG drive_number = 0;
#endif
   ULONG  logical_drive_map;

   DosError(DISABLE_ERRORPOPUPS);
   i = init(&flags, argc, argv);     /* get command line flags */
   if (i != BAILOUT)                 /* if no errors,          */
     {                               /*   get the current drive and drive */
#ifdef I16
       DosQCurDisk(&drive_number, &logical_drive_map);         /* map     */
#else
       DosQueryCurrentDisk(&drive_number, &logical_drive_map);         /* map     */
#endif
       if (i == 0 || i >= argc)        /* Default action is 'show all' */
         rc = do_all_disks(&flags, drive_number, logical_drive_map);
       else                            /* But we can also show just those */
         {                             /* drives specified on the command line*/
           for (; i < argc; i++)
             {
              rc = query_drive(argv[i], &flags, drive_number, logical_drive_map);
//              if(rc)
//                 {
//                   printf("Drive %c: cannot be accessed.\n",toupper(*argv[i]));
//                 }
             }
         }
     }
   else
     rc = EXIT_FAILURE;
   DosError(ENABLE_ERRORPOPUPS);
   return(rc);
}

/*---------------------------------------------------------------------------*/
/*  init()                                                                   */
/*---------------------------------------------------------------------------*/
int init(struct FLAGS *fp, int argc, char **argv)
{
#ifdef I16
   char    c;                             /* flag specifed, used w/getopt*/
#else
   int     c;
#endif
   fp->M_flag = 0;
   fp->I_flag = 0;
   fp->v_flag = 0;
#ifdef I16
   while ((c = (char)getopt(argc, argv, "IiMvs")) != EOF)
#else
   while ((c = getopt(argc, argv, "IiMvs")) != EOF)
#endif
      switch (c)
      {
        case 'M':              fp->M_flag = 1;
                                 break;
        case 'I':              fp->I_flag = 1;
                                 break;
        case 'i':                break;
        case 'v':              fp->v_flag = 1;
                                 break;
        default:               show_usage();
                               return(BAILOUT);
                                 break;
      }
   return(optind);
}


/*---------------------------------------------------------------------------*/
/*  show_header()                                                            */
/*---------------------------------------------------------------------------*/
void show_header(struct FLAGS *fp)
{
   static int first_time = 1;

   if (first_time) {
      first_time = 0;
   } else {
      return;
   } /* endif */

   printf("%-22s", "Filesystem");

   if (fp->M_flag) {
      printf("%-12s", "Mounted on");
   } else {
   } /* endif */

   printf("%10s","Total KB");

   if (fp->I_flag || fp->v_flag) {
      printf("%12s", "used");
   } else {
   } /* endif */

   printf("%12s","free");

   printf("  %-8s", "%used");

   if (!fp->M_flag) {
      printf("%-12s", "Mounted on");
   } else {
   } /* endif */

   printf("\n");

}


/*---------------------------------------------------------------------------*/
/*  do_all_disks()                                                           */
/*---------------------------------------------------------------------------*/
int do_all_disks(struct FLAGS *fp, USHORT dn, ULONG logical_file_map)
{
   USHORT rc; 
#ifdef I16
   USHORT drive_number = 0;
#else
   ULONG drive_number = 0;
#endif
   ULONG  logical_drive_map;
   char drive_spec[3];

   rc = EXIT_FAILURE;
#ifdef I16
   DosQCurDisk(&drive_number, &logical_drive_map);
#else
   DosQueryCurrentDisk(&drive_number, &logical_drive_map);
#endif
   for ( drive_number=2 ; drive_number<32 ; drive_number++ ) {
      if ((logical_drive_map>>drive_number) % 2)    /* is the bit set? */
        {
          strcpy(drive_spec," :");
          *drive_spec = (char)(drive_number + 'A');
          rc = query_drive(drive_spec, fp, dn, logical_file_map);
        }
   } /* endfor */
   return(rc);

}

/*---------------------------------------------------------------------------*/
/*  query_drive()                                                            */
/*---------------------------------------------------------------------------*/
int query_drive(char *spec, struct FLAGS *fp, USHORT dn, ULONG logical_drive_map)
{
   USHORT rc, drive_number;
   FSALLOCATE fsallocate;
   ULONG  allocation_unit_size, total, avail, used;
   UCHAR  device_name[3];
#ifdef I16
   UCHAR buffer[100];
   USHORT buffer_size = 100;
#else
   UCHAR dqfsa_buffer[1024]; /* 1K should be enough for long path & hostname */
   ULONG buffer_size = 1024;
   PFSQBUFFER2 buffer = (PFSQBUFFER2) dqfsa_buffer;
#endif /* I16 */
   char   *p;
   UCHAR  * filesystem = NULL;

   p = spec;
   if (isalpha(*p) && *(p+1) == ':')
     drive_number = toupper(*spec) - 'A' + 1;
   else
     drive_number = dn;
#ifdef NOTDEF
     {
        if (*p == '\\')
          p++;
        for (drive_number=3, mask=4l; drive_number<32; mask<<1, drive_number++)
           if (mask & logical_drive_map)
             {
                strcpy(filename," :\\");
                *filename = (char)(drive_number + 'A' - 1);
                strcat(filename, p);
                if (fmf_init(filename, FMF_ALL_DIRS_AND_FILES, FMF_SUBDIR)
                                                                    == NO_ERROR)
                  if (fmf_return_next(filename, &temp) == NO_ERROR)
                    break;
             }
     }
#endif /* NOTDEF */
   if (drive_number < 32)
     {
       rc = DosQFSInfo(drive_number,(USHORT)1,(PBYTE)&fsallocate,
                       sizeof(fsallocate));
       if (rc == 0) {
         show_header(fp);
         device_name[0] = (UCHAR)('A' + drive_number - 1);
         device_name[1] = ':';
         device_name[2] = '\0';
#ifdef I16
         rc = DosQFSAttach( device_name, 0, 1, buffer, &buffer_size, 0L);
#else
         rc = DosQueryFSAttach( device_name, 0, 1, buffer, &buffer_size);
#endif /* I16 */
#ifdef I16
         switch (buffer[0]) {
           case 3:
              filesystem = &buffer[ 7 + (USHORT)buffer[2]];
              break;
           case 4:
              filesystem = &buffer[ 7 + (USHORT)buffer[2]];
              filesystem += strlen(filesystem);
              filesystem += 1;
              filesystem += 2;
              break;
           default:
              printf("unknown FS type\n");
         }
#endif
         allocation_unit_size = fsallocate.cSectorUnit * fsallocate.cbSector;
         total =  (allocation_unit_size * fsallocate.cUnit / 1024);
         avail =  (allocation_unit_size * fsallocate.cUnitAvail / 1024);
         used  = total - avail;
#ifdef I16
         printf("%-22s", filesystem); 
#else
	 if (buffer->iType != FSAT_REMOTEDRV)
           printf("%-22s", buffer->szFSDName+buffer->cbName);          /* fsd name */
         else 
           printf("%-22s", (buffer->rgFSAData)+(buffer->cbFSDName)+2); /* filesystem name */
#ifdef NOTDEF
	 {
	   int my_i;
	   char *buffer_p;
	   printf("\n");
	   buffer_p = (char *)buffer;
	   for(my_i = 0; my_i < buffer_size; my_i++,buffer_p++) {
	     if(isalpha(*buffer_p))
	       printf("%c", *buffer_p);
	         else
	       printf("^");
	   }
	   printf("\n");
	 }
#endif /* NOTDEF */
#endif
         if (fp->M_flag) {
            printf("%c:%10s",'A' + drive_number - 1, " ");
         } else {
         } /* endif */

         /* Total KB */
         printf("%10lu", total);

         if (fp->I_flag || fp->v_flag) {
            printf("%12lu", used);
         } else {
         } /* endif */

         printf("%12lu", avail);

         if (total) {
            printf("%7lu  ", (used * 100) / total );
         } else {
            printf("%7s  ", "NaN");
         } /* endif */

         if (!fp->M_flag) {
            printf(" %c:",'A' + drive_number - 1);
         } else {
         } /* endif */
         printf("\n");
         return(EXIT_SUCCESS);

//       } else {
//          printf("Drive %c: cannot be accessed.\n",'A' + drive_number - 1);
       } /* endif */

     }
   return(EXIT_FAILURE);
}

/*---------------------------------------------------------------------------*/
/*  show_usage()                                                             */
/*---------------------------------------------------------------------------*/
void show_usage()
{
printf("     Copyright IBM, 1991, 1995\n");
printf("df - Reports information about space on file systems\n");
printf("Usage\n");
printf("     df [-l|-M|-i|-s|-v] [FileSystem FileSystem... | File File...]\n");
printf("Specify FileSystem as d: where d is the drive letter.\n");
}
