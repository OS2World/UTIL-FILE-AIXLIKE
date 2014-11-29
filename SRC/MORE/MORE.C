static char sccsid[]="@(#)56	1.1  src/more/more.c, aixlike.src, aixlike3  9/27/95  15:44:58";
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
/* @1 05.08.91 changed fmf_init to look for all files including hidden   */
/* @2 11.26.91 'move forward' should end processing for a file at eof    */
/* @3 11.26.91 ring of files should stay open until an explicit quit.    */
/*             This will require keeping a seperate bi for each file.    */
/* @4 03/16.92 Still had trouble printing ends of piped in files         */
/* @5 09.16.92 Make return code explicit                                 */
/* @5 11.17.92 Probably no reason to ever return non-0.                  */
/* @6 05.03.93 Modify for IBM C-Set/2 compiler                           */
/* @7 10.27.94 Make code compile under C-Set++ (GCW)                     */
/*-----------------------------------------------------------------------*/

/*
   more or page

   Displays continuous text one screen at a time on a display screen.

   Syntax

         more -cdflsu -n +Number    File [File ...
          or                or
         page            +/Pattern

   The more command displays continuous text one screen at a time. It pauses
   after each screen adn prints the word More at the bottom of the screen.
   If (you then press a carriage return, the more command displays an
   additional line.  If you press the space bar, the more command displays
   another screen of text.

   NOTE: On the IBM5151 and similar terminal models the more command clears
   the screen instead of scrolling, before displaying the next screenful
   of data.

   Flags:
       -c   Keeps the screen from scrolling and makes it easier to
            read text while the more command is writing to the
            terminal.  The -c flag is ignored if the terminal does
            not have the ability to clear to the end of a line.

       -d   Prompts the user to continue, quit, or obtain help.

       -f   Causes the more command to count logical lines, rather
            than screen lines.  The -f flag is recommended if the nroff
            command is being piped through the ul command.

       -l   Does not treat control-L (form feed) in a special manner.
            If the -l flag is not supplied, the more command pauses
            after any line that contains a control-L, as though a full
            screen of text has been reached.  If a file begins with a form
            feed, the screen is cleared before the file is printed.

       -n   (Where n is an integer) specified the number of lines in
            the window.  If the -n flag is specified, the more command
            does not use the default number.

       -s   Squeezes multiple blank lines from the output to produce only
            one blank line.  The -s flag is particularly helpful when
            viewing output from the nroff command.  This flag maximizes
            the useful information present on the screen.

       -u   Suppresses the more command from underlining or creating
            stand-out mode for underlined information in a source file.

       +Number
            Starts at the specified line Number.

       +/Pattern
            Starts two lines before the line containint the regular
            expression Pattern.

   In UNIX, the more command uses the file /etc/terminfo to determine
   terminal characteristics and to determine the default window size; in
   OS/2, I haven't figured out how to do it yet.  On a terminal capable of
   displaying 24 lines, the default window size is 22 lines.

   If the more command is reading from a file rather than a pipe, it displays
   a percent sign (%) with the more command prompt.  This provides the fraction
   of the file (in characters, not lines) that the more command has read.

   The page command is equivalent to the more command.  If the program is
   invoked using the page command, the screen is cleared before each
   screen is printed (only if a full screen is being printed).  In this
   instance, the page command prints K-1 (where K is the number of lines
   the terminal can display) rather than K-2 lines.

   Command Sequences:
       Other command sequences that can be entered when the more command
       pauses are as follows (/ is an optional integer option, defaulting
       to 1):

       i|<space>    Displays / more lines (or another screen full of text
                    if no option is given)

       ctrl-D       Displays 11 more lines (a "scroll").  If i is given,
                    the scroll size is set to i/.   Same as d.

       d            Displays 11 more lines (a "Scroll").  Same as ctrl-D.

       .i|z         Same as typing a space except that if I/ is present, it
                    becomes the new window size.

       i|s          Skips I/ lines and prints a full screen of lines.

       i|f          Skips I screens and prints a full screen of lines

       i|b          Skips back I screens and prints a full screen of lines

       i|ctrl-B     Skips back / screens and prints a full screen of lines.

       q or Q       Exits from more command.

       =            Displays the current line number

       v            Invokes vi editor <Not implemented in OS/2 version >

       h            This is the help command.  Provides a description for
                    all the more commands.

       i|/expr      Searches for the i/-th occurance of the regular expression
                    Expression.  If there are less than I occurances of
                    Expression/, and the input is a file (rather than a pipe),
                    the position in the file remains unchanged.  Otherwise,
                    a full screen is displayed, starting two lines prior to the
                    location of the expression.

       i|n          Searches for the I/th occurance of the last regular
                    expression entered.  Goes to the point from which the
                    last search started.  If a search has not been performed
                    in the current file, this sequence causes the more
                    command to go to the beginning of the file.

       !command     Invokes a shell.  The characters '%' and '!' in the
                    command sequence replaced with the current file name
                    and the previous shell command respectively.  If there
                    is no current file name, '%' is not expanded.  The
                    sequences "\%" and "\!" are replaced by"%" and "!"
                    respectively.

       i:n          Skips to the I/th next file given in the command line.
                    If (n is not valid, skips to the last file.

       .i:p         Skips to the I/th previous file given in the command line.
                    If this sequence is entered while the more command is in
                    the middle of printing a file, the more command goes back
                    to the beginning of the file.

       :f           Displays the current file name and line number.

       :q or :Q     Exits (same as q or Q).

       .            (dot)Repeats the previous command.

       When any of the command sequences are entered, they begin to process
       immediately.  It is not necessary to type a carriage return.

   The more command sets terminals to Noecho mode.  This enables the output to
   be continuous and only displays the / and ! commanda on your terminal
   as your type.  If the standard output is not a terminal, the more command
   processes like the cat command.  However, it prints a header before each
   file (if there is more than one file).

   ------------------------------------------------------------------------
   *  Implementation Notes
   *
   *     - I've decided to set a maximum number of files at 64.  I won't
   *       even try to process more than that.
   *
   *     - I'm going to use VIO for video output.
*/

#include <idtag.h>    /* package identification and copyright statement */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <conio.h>
#include <io.h>
#include "fmf.h"
#define INCL_VIO
#define  INCL_BASE
#define  INCL_NOPM
#include <os2.h>
#include "more.h"

/* temporary definition for input_is_pipe() */
#define input_is_pipe() (bi->file_size == 0)

/* temporary definition for input_is_stdin() */
#define input_is_stdin() (bi->stream == stdin)

char buf[BUFSIZE];
struct window_parms wp;
struct buff_info *bi[MORE_MAX_FILES];
char *file_list[MORE_MAX_FILES];   /* list of names of files to be browsed */

int stdout_is_tty;             /* If YES, output is to a character device */
int suppress_scrolling;        /* If YES, -c was a flag on the command line */
int show_prompt;               /* if YES, -d was a flag on the command line */
int count_logical_lines;       /* if YES, -f was a flag on the command line */
int ignore_form_feed;          /* if YES, -l was a flag on the command line */
int lines_in_window;           /* non-zero if user specified a screen size  */
int squeeze_blank_lines;       /* if YES, -s was a flag on the command line */
int suppress_underlines;       /* if YES, -u was a flag on the command line */
unsigned long starting_line;   /* first line of file to be printed (rel to 1) */
char *search_pattern;          /* regular expression to be searched for */
off_t file_size;               /* Bytes in file currently being browsed */
char *linebuff;                /* a global area big enough for an output line */
int num_files;                 /* number of files specified on cmd line */
int should_be_some_files;      /* YES if a filespec appeared on command line */
char bline[BLSIZE + 1];
char *More = "--More";
char *Line = "Line ";
char *AnyKey = "Press any key to continue...    ";
char *cSkipping = "Skipping ";
char *cBack = "Back ";
char *cLines = " Lines... ";
char *bprompt = "[SpaceBar to continue, q to quit, h for help]";
char *pnotfnd = "Pattern not found";
#define SKIPVAL (strlen(cSkipping))
#define BACKVAL (strlen(cBack))

char *HelpLines[] = {
"[i]<space> | PgDn | s      Display i more lines (default is full screen)",
"[i]PgUp                    Skip back i lines (default is full screen)",
"[i]<enter> | <DownArrow>   Move down i lines (default is 1)",
"[i]<UpArrow>               Move up i lines (default is 1)",
"[i]z                       Like <space>, but sets screen size to i",
"[i]d | ctl-D               Scroll i more lines (default is 11 or last i)",
"[i]f                       Skip forward i screens (default is 1)",
"[i]b | ctl-B               Skip back i screens (default is 1)",
"[i]:n                      Skip to the i-th next file (default i = 1)",
"[i]:p                      Skip to the i-th previous file (default i = 1)",
"[i]/expression             Locate the ith occurance of a regular expression",
"[i]n                       Locate ith occurance of last regular expression",
"=                          Display the current line number",
":f                         Display line number and file name",
"h                          Display this help screen",
"v                          Invoke vi editor (doesn't work in OS/2 version)",
"q | Q | :q | :Q            Exit the more command (Quit)",
#ifndef I16 /* We don't need an escape before the percent  @7 */
"!command                   Invoke a shell and issue the command.  % is expanded",
#else
"!command                   Invoke a shell and issue the command.  \% is expanded",
#endif /* I16 */
"                           into the current file name; ! into the previous cmd",
""
};

/* Function Prototypes */

int  init(int, char **);        /* parse cmd line and do initializations */
int  do_a_file(int *, struct buff_info **);  /* execute more on an open file */
int  dumpfile(int *);
void do_stdin(void);            /* execute more on stdin */
int  init_a_filespec(char *);   /* add a filespec's files to the file list */
void set_window_parms(int, int);/* set windowing parameters */
void add_file_to_list(char *);  /* business end of init_a_filespec */
                                /* redraw an invalid area of the screen */
void invalidate(struct buff_info *, int, int);
                                    /* Handle logical portion of scroll */
void LogicalScrollDown(struct buff_info *, unsigned);
void LogicalScrollUp(struct buff_info *, unsigned);     /* Ditto */
int  isateof(struct buff_info *);   /* boolean: window is at bottom of file */
int  isattof(struct buff_info *);   /* boolean: window is at top of file */
                                    /* put line number at bottom of screen */
void display_line_no(struct buff_info *);
                                /* put file name and line no at bottom of scrn*/
void display_file_name(struct buff_info *);
void find_expression(struct buff_info *, int);   /* find a regular expression */
void find_next_occurance(struct buff_info *, int);  /* find nth occurance of last reg exp */
void execute_command(struct buff_info *);   /* execute a command in a shell */
void bld_bline(struct buff_info *);
int find_starting_line(unsigned long, char *, struct buff_info *);
off_t grepsrch(off_t, char *);
char *offsetToAddress(struct buff_info *, off_t);
void formatline(struct buff_info *, off_t);
void putbanner(char *);

/*------------------------------------------------------------------------- *
 *  main()                                                                  *
 *                                                                          *
 *--------------------------------------------------------------------------*/
int main(int argc, char **argv)                                     /* @5c */
{
   int status, current_file;

   if (init(argc, argv) != BAILOUT)
     {
        if (num_files > 0)       /* if read files were specified */
          {
            current_file = 0;       /* This variable gets reset all over */
            do {
                 if (openit(file_list[current_file], &bi[current_file], buf)) //@3c
                   status = do_a_file(&current_file, &bi[current_file]);
               }
                  while((status == KEEP_GOING) && (++current_file < num_files));
          }
        else                     /* if we are working from stdin */
          do_stdin();
     }
//   else                                                           /* @5c @6d*/
//     return(BAILOUT);                                             /* @5c @6d*/
   return(0);                                                       /* @5c */
}

/*------------------------------------------------------------------------- *
 *  init()                                                                  *
 *                                                                          *
 *                          initialize bottom (status) line                 *
 *                          set the external variable myerror_pgm_name      *
 *                          initialize some buffer information              *
 *                          determine whether we are writing to screen      *
 *                          initialize option control varibles              *
 *                          parse the command line                          *
 *                          exit if all file names were bad                 *
 *                          use default window size if none set             *
 *--------------------------------------------------------------------------*/

init(int argc, char **argv)
{
   int i, cols_in_window;
   char *p;
                               /* initialize bottom (status) line */
   for (i = 0; i < BLSIZE; bline[i++] = ' ');
   bline[i] = 0;
                               /* set the external variable myerror_pgm_name */
   if (p = strrchr(argv[0], BACKSLASH))     /* get invocation name */
     myerror_pgm_name = p+1;
   else
     myerror_pgm_name = argv[0];
   wp.lastcmd = NULL;          /* initialize some window information */
   wp.lastexpr = NULL;
                               /* initialize some buffer information */
//   bi.bufaddr = buf;             /* set file i/o buffer address */ // @3d2
//   bi.bufsize = BUFSIZE;
   for (i = 0; i < MORE_MAX_FILES; bi[i++] = (struct buff_info *)NULL); //@3a
                               /* determine whether we are writing to screen */
   stdout_is_tty = NO;
   if (isatty(fileno(stdout)))
     stdout_is_tty = YES;
                               /* initialize option control varibles */
   suppress_scrolling = NO;
   show_prompt = NO;
   count_logical_lines = NO;
   ignore_form_feed = NO;
   lines_in_window = 0;
   squeeze_blank_lines = NO;
   suppress_underlines = NO;
   starting_line = 0;
   wp.scroll_size = 11;
   wp.move_size = 1;
   wp.linebuff = NULL;
   search_pattern == NULL;
   num_files = 0;
   should_be_some_files = NO;
                               /* parse the command line */
   for (i = 1; i < argc; i++)     /* loop thru each cmd line argument */
     {
        p = argv[i];
        if ( (*p == DASH) || (*p == SLASH) )
          while (*p)
            switch (*++p)                       /* flags introduced by - or / */
            {
            case 0  :           /* end of the command line argument */
                    break;
            case 'c':           suppress_scrolling = YES;
                    break;
            case 'd':           show_prompt = YES;
                    break;
            case 'f':           count_logical_lines = YES;
                    break;
            case 'l':           ignore_form_feed = YES;
                    break;
            case 's':           squeeze_blank_lines = YES;
                    break;
            case 'u':           suppress_underlines = YES;
                    break;
            case '?':           tell_usage();
                                return(BAILOUT);
                    break;
            default :           if (lines_in_window != 0)
                                  {
                                     tell_usage();
                                     return(BAILOUT);
                                  }
                                else
                                  if (isdigit(*p))
                                    lines_in_window = atoi(p);
                                  else
                                    {
                                      tell_usage();
                                      return(BAILOUT);
                                    }
                                *p = 0;   /* this gets us off this arg */
                    break;
            }
        else          /* flag wasn't introduced by - or / */
          if (*p == PLUS)
            if (*++p == SLASH)
              search_pattern = ++p;  /* reg expression to search for */
            else
              {
                 starting_line = atol(p);  /* starting line number */
                 if (starting_line == 0l)
                   {
                      tell_usage();
                      return(BAILOUT);
                   }
              }
          else
            {
                                  /*  it's not a flag -- must be a file name */
              if (init_a_filespec(p) == BAILOUT)
                return(BAILOUT);
              else
                should_be_some_files = YES;
            }
     }
                                      /* exit if all file names were bad */
   if (should_be_some_files && (num_files == 0) )
     return(BAILOUT);
                                      /* use default window size if none set */
   if (lines_in_window == 0)
     getvmode(&lines_in_window, &cols_in_window);
   if (lines_in_window == 0)     /* if all else fails, default to 24 lines */
     lines_in_window = 24;
   if (cols_in_window == 0)
     cols_in_window = 80;
   set_window_parms(lines_in_window, cols_in_window);

   return(YES);
}

/*------------------------------------------------------------------------- *
 *  init_a_filespec()                                                       *
 *                                                                          *
 *  If the file spec names a file, add it to the files to be browsed; if    *
 *  the file spec expands to a list of files, add all the files in the list.*
 *                                                                          *
 *  If the file spec names a directory, try to add all the files in that    *
 *  directory.                                                              *
 *--------------------------------------------------------------------------*/

init_a_filespec(char *filespec)
{
  struct stat buf;
  char filen[_MAX_PATH];
  int  attr, prev_num_files;

  if (!strchr(filespec, ASTERISK) && !strchr(filespec, QMARK))
    if (stat(filespec, &buf))         /* see if it exists */
      {
         printf("%s: no files matching %s\n", myerror_pgm_name, filespec);
         return(NO);                        /* if not, we're out of here */
      }
    else                              /* but if it does, make sure it's not * */
#ifndef I16 /* S_IFMT went away; just test the dir bit  @7 */
      if (buf.st_mode & S_IFDIR)     /* a directory */
#else
      if ( (buf.st_mode & S_IFMT) != S_IFDIR)     /* a directory */
#endif /* I16 */
        {
          add_file_to_list(filespec);      /* if it's not, we can browse it */
          return(YES);
        }

  prev_num_files = num_files;
//  if (fmf_init(filespec, 0, 0) == FMF_NO_ERROR)                          @1
  if (fmf_init(filespec, FMF_ALL_FILES, 0) == FMF_NO_ERROR)             // @1
    {
      while(fmf_return_next(filen, &attr) == FMF_NO_ERROR)
         add_file_to_list(filen);
      fmf_close();
    }
  if (prev_num_files == num_files)
    printf("%s: no files matching %s\n", myerror_pgm_name, filespec);
      return(NO);

  return(YES);
}

/*------------------------------------------------------------------------- *
 *  add_file_to_list                                                        *
 *                                                                          *
 *  Allocate enough memory to store the file name, then put a pointer to    *
 *  the name in the file list.  If there isn't enough memory, complain.     *
 *                                                                          *
 *  If the maximum number of files has been exceeded, complain.             *
 *--------------------------------------------------------------------------*/

void add_file_to_list(char *filename)
{
   if (num_files < MORE_MAX_FILES)
     {
        if (file_list[num_files] = (char *)malloc(strlen(filename)+1))
          {
            strcpy(file_list[num_files], filename);
            num_files++;
          }
        else
          myerror(ERROR_NOT_ENOUGH_MEMORY, "Adding file to browse list",
                    filename);
     }
   else
     printf("File %s not added; only %d files can be browsed at one time\n",
             filename, MORE_MAX_FILES);
}

/*
    Returns a status, which is either GET_OUT_OF_HERE, or KEEP_GOING;
    If the former, mainline will quit; if the latter, it will call this
    routine again with the next file number
*/

/*------------------------------------------------------------------------- *
 *  do_a_file                                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/
do_a_file(int *fileno, struct buff_info **bipp)                 // @3c
{
  unsigned int qty, lastqty;
  int c, lastcmd;
  struct buff_info *bi;                                          // @3a

  bi = *bipp;                                                    // @3a
  if (!stdout_is_tty)
    return(dumpfile(fileno));

                /* find_starting_line does all the init work on the file */
  if (find_starting_line(starting_line, search_pattern, bi))
    {
       clear_screen(&wp);
       invalidate(bi, wp.display_rows, 0);
       lastcmd = INVALID;
       if (stdout_is_tty)
         while (1)
         {
           setcursorpos(wp.window_rows -1, 0);
           switch (c = getcmd(&qty, bi, &wp))
           {                                /* dot */
              case REPEAT:       c = lastcmd;
                                 qty = lastqty;
                                       /* d or ctl-D */
              case SCROLL_DOWN:  if ( (qty > 0) &&
                                      (qty < (unsigned)wp.display_rows))
                                   wp.scroll_size = qty;
                                 if (isateof(bi))
                                   putchar(BEL);
                                 else
                                   LogicalScrollDown(bi,wp.scroll_size);
              break;
                                            /* PgUp */
              case MOVE_UP_SCREEN: if (qty == 0)
                                      qty = 1;
                                   if ( isattof(bi) || input_is_pipe() )
                                     putchar(BEL);
                                   else
                                     LogicalScrollUp
                                                   (bi, qty * wp.display_rows);
              break;
                                            /* PgDn */
              case MOVE_DOWN_SCREEN: if (qty == 0)
                                       qty = 1;
                                     if (isateof(bi))
//                                       putchar(BEL);              @2d
                                       return(KEEP_GOING);       // @2a
                                     else
                                       LogicalScrollDown
                                                   (bi, qty * wp.display_rows);
              break;
                                       /*  s or space bar or enter */
               case LSKIP_DOWN:
               case MOVE_DOWN:   if (isateof(bi))
//                                   putchar(BEL);                  @2d
                                   return(KEEP_GOING);           // @2a
                                 else
                                   {
                                     if (qty == 0)
                                       qty = wp.display_rows;
                                     LogicalScrollDown(bi, qty);
                                   }
               break;
                                             /* up Arrow */
               case MOVE_UP:     if ( isattof(bi) || input_is_pipe() )
                                   putchar(BEL);
                                 else
                                   {
                                     if (qty == 0)
                                       qty = wp.move_size;
                                     LogicalScrollUp(bi, qty);
                                   }
               break;
                                                 /* z */
               case CHG_SIZE:    if (qty > 2)
                                   {
                                      wp.window_rows = qty;
                                      wp.display_rows = qty - 2;
                                   }
                                 if (isateof(bi))
//                                   putchar(BEL);                  @2d
                                   return(KEEP_GOING);            //@2a
                                 else
                                   LogicalScrollDown(bi, wp.display_rows);
               break;
                                                 /* f */
               case SSKIP_DOWN:   if (isateof(bi))
//                                    putchar(BEL);                 @2d
                                    return(KEEP_GOING);           //@2a
                                  else
                                    {
                                      if (qty == 0)
                                        qty = wp.display_rows;
                                      LogicalScrollDown(
                                                     bi, qty + wp.display_rows);
                                    }
                break;
                                             /* b or ctl-B */

                case SSKIP_UP:    if (isattof(bi) || input_is_pipe() )
                                    putchar(BEL);
                                  else
                                    {
                                      if (qty == 0)
                                        qty = 1;
                                      LogicalScrollUp(bi,qty * wp.display_rows);
                                    }
                break;
                                       /* DEL or q or Q or :q or :Q */
                case QUIT:        closeit(bi);
//                                clear_screen(&wp);             @2d2
//                                setcursorpos(0, 0);
                                  return(BAILOUT);
                break;
                                              /* :l */
                case DLINENO:     display_line_no(bi);
                break;
                                              /* :f */
                case DFILENAME:   display_file_name(bi);
                break;

                case HELP:        clear_window(&wp);
                                  display_help(HelpLines, AnyKey, &wp);
                                  clear_window(&wp);
                                  invalidate(bi, wp.display_rows, 0);
                break;

                case FINDEXPR:    if (input_is_stdin())
                                    putchar(BEL);
                                  else
                                    {
                                      if (qty == 0)
                                        qty = 1;
                                      find_expression(bi,qty);
                                    }
                break;

                case FINDNTH:     if (input_is_stdin())
                                    putchar(BEL);
                                  else
                                    {
                                      if (qty == 0)
                                        qty = 1;
                                      find_next_occurance(bi, qty);
                                    }
                break;

                case EXECCMD:     if (input_is_stdin())
                                    putchar(BEL);
                                  else
                                    {
                                      execute_command(bi);
                                      clear_screen(&wp);
                                      invalidate(bi, wp.display_rows, 0);
                                    }
                break;

                case FSKIP_DOWN:  if (input_is_stdin())
                                    putchar(BEL);
                                  else
                                    {
//                                      closeit(bi);                @2d
                                      if (qty == 0)
                                        qty = 1;
                                      if ((*fileno + qty) < num_files)
                                        *fileno += (qty - 1);
//                                      else                        @2d2
//                                        *fileno = num_files - 2;
                                      return(KEEP_GOING);
                                    }
                break;

                case FSKIP_UP:    if (input_is_stdin())
                                    putchar(BEL);
                                  else
                                    {
//                                      closeit(bi);                @2d
                                      if (qty == 0)
                                        qty = 1;
                                      if ( (unsigned)*fileno < qty)
                                        *fileno = -1;
                                      else
                                        *fileno -= (qty + 1);
                                      return(KEEP_GOING);
                                    }
                break;

                default:          c = INVALID;
                break;

           }   /* end switch */

           lastcmd = c;
           lastqty = qty;

         }   /* end while */
    }
  else      /* find_starting_line failed */
    return(BAILOUT);     /* should never happen */
}

/*------------------------------------------------------------------------- *
 *  dumpfile                                                                *
 *                                                                          *
 *--------------------------------------------------------------------------*/
int dumpfile(int *fileno)
{
      FILE *f;                                                        // @3a

      putbanner(file_list[*fileno]);
      if (f = fopen(file_list[*fileno], "r"))                        // @3c@6c
        {
          while (fgets(buf, FGETSIZE, f))                             // @3c
            fputs(buf, stdout);
          printf("%c\n", CTLL);
          fclose(f);                                                  // @3c
        }
      else
        printf("--Could not open the file--\n%c", CTLL);
      return(KEEP_GOING);
}


/*------------------------------------------------------------------------- *
 *  putbanner                                                               *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void putbanner(char *fn)
{
      int fnlen, bannerlen, i;
      time_t btime;

      time(&btime);
      printf("%s", ctime(&btime));
      fnlen = strlen(fn);
      bannerlen = fnlen + 6;
      for (i = 0; i < bannerlen; buf[i++] = ASTERISK);
      buf[i] = 0;
      puts(buf);
      strnset(buf + 1, SPACE, bannerlen - 2);
      puts(buf);
      strncpy(buf + 2, fn, fnlen);
      puts(buf);
      strnset(buf + 1, SPACE, bannerlen - 2);
      puts(buf);
      strnset(buf, ASTERISK, bannerlen);
      puts(buf);
}

/*------------------------------------------------------------------------- *
 *  do_stdin                                                                *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void do_stdin()
{
   struct stat sbuf;
   int fileno = 0;
   struct buff_info *bi;

   bi = (struct buff_info *)malloc(sizeof(struct buff_info));
   if (!bi)
     myerror(ERROR_NOT_ENOUGH_MEMORY, "Buff_info block", "stdin");
   else
     {
      if (fstat(0, &sbuf))
        bi->file_size = 0;
      else
       bi->file_size = sbuf.st_size;

       bi->filename = "stdin";
       bi->stream = stdin;
       bi->Tob = 0;
       bi->Tols = 0;
       bi->Pbols = 0;
       bi->bufaddr = buf;
       bi->Bib = fread(bi->bufaddr, sizeof(char), (size_t)BUFSIZE, bi->stream);
       if (bi->Bib < BUFSIZE)                          //@4a2
         bi->file_size = bi->Bib;
       do_a_file(&fileno, &bi);
     }
}

/*--------------------------------------------------------------------------*
 *  set_window_parms                                                        *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void set_window_parms(int window_r, int window_c)
{
   wp.window_rows = window_r;
   wp.display_rows = window_r - 2;
   wp.top_row = 0;
   wp.left_col = 0;
   wp.display_cols = window_c;
   wp.clearcell[0] = 0x20;       /* use spaces to clear screen */
   wp.clearcell[1] = get_cell_attributes_at_cursor();
   if (wp.linebuff)
     free(linebuff);
   if ( (wp.linebuff = (char *)malloc(window_c + 1)) == NULL)
     myerror(ERROR_NOT_ENOUGH_MEMORY, "set_window_parms", "linebuff");
//   wp.lastcmd = NULL;

}

/*--------------------------------------------------------------------------*
 *  invalidate                                                              *
 *                                                                          *
 *  assume the contents of the area described to need refreshing.           *
 *--------------------------------------------------------------------------*/
void invalidate(struct buff_info *bi, int number_of_lines, int starting_line)
{
    int i, linedata;
    off_t lined;

    linedata = YES;
    for (lined = bi->Tols, i = 0; i < starting_line; i++)
      if ( (lined = oneLineDown(lined, bi, &wp)) == LEOF )
        linedata = NO;
    for (i = 0; i < number_of_lines; i++)
      {
         if (linedata)
           {
             formatline(bi, lined);
             if (suppress_scrolling)
               putline(i + starting_line, &wp);
             else
               {
                  PhysScrollUp(1, &wp);
                  putline(wp.display_rows - 1, &wp);
               }
             if ( (lined = oneLineDown(lined, bi, &wp)) == LEOF)
               linedata = NO;
           }
         else
           if (suppress_scrolling)
             clear_eol(i + starting_line, 0, &wp);
      }
    bld_bline(bi);
    put_bline(bline, BLSIZE, &wp);
}

/*-----------------------------------------------------------------------*/
/*  bld_bline                                                            */
/*-----------------------------------------------------------------------*/
void bld_bline(struct buff_info *bi)
{
    int pct, n, size;
    char apct[4];

    for (n = 0; n < BLSIZE; bline[n++] = SPACE);
    strncpy(bline, More, BLLPAREN);
    size = BLLPAREN;
    if (bi->file_size > 0)
      {
        pct = (int)((100*bi->Pbols)/bi->file_size);
        bline[BLLPAREN] = LPAREN;
        itoa(pct, apct, 10);
        n = strlen(apct);
        strncpy(bline+BLPCT, apct, n);
        strncpy(bline + n + BLPCT, "%)", 2);
        size += (n + 4);
      }

    if ( (num_files > 1) && (bi->Tols == 0) )
      {
        n = strlen(bi->filename);
        if ((BLSIZE - size) < n)
          n = BLSIZE - size;
        strncpy(bline + size, bi->filename + (strlen(bi->filename) - n), n);
      }
    else
      if (show_prompt)
        {
          n = strlen(bprompt);
          if ((BLSIZE - size) < n)
            n = BLSIZE - size;
          strncpy(bline + size, bprompt, n);
        }

}

/*-----------------------------------------------------------------------*/
/*  display_line_no                                                      */
/*-----------------------------------------------------------------------*/
void display_line_no(struct buff_info *bi)
{
    char cline[BLSIZE + 1];
    int i;
    int width;

    width = wp.display_cols;
    if (width > BLSIZE)
      width = BLSIZE;
    for (i = 0; i < width; cline[i++] = SPACE);
    strncpy(cline, Line, strlen(Line));
    ltoa(bi->lineno, cline + strlen(Line), 10);
    put_bline(cline, width, &wp);
}

/*-----------------------------------------------------------------------*/
/*  display_file_name                                                    */
/*-----------------------------------------------------------------------*/
void display_file_name(struct buff_info *bi)
{
    char *fp;
    char cline[13];
    char fline[BLSIZE + 1];
    int left, width, i, fnl;

    width = wp.display_cols;
    if (width > BLSIZE)
      width = BLSIZE;
    for (i = 0; i < width; fline[i++] = SPACE);
    fline[i] = 0;
    strncpy(cline, Line, strlen(Line));
    ltoa(bi->lineno, cline + strlen(Line), 10);
    left = width - strlen(cline);
    fp = bi->filename;

    fnl = strlen(cline);
    strncpy(fline, cline, fnl);
    fnl++;
    if (left < (int)strlen(bi->filename))
      {
        fp = bi->filename + 3 + ((int)strlen(bi->filename) - width);
        strncpy(fline + fnl, ELLIPSIS, strlen(ELLIPSIS));
        fnl += 3;
      }
    strncpy(fline + fnl, fp, strlen(fp));

    put_bline(fline, width, &wp);
}

/*-----------------------------------------------------------------------*/
/*  find_expression                                                      */
/*-----------------------------------------------------------------------*/
void find_expression(struct buff_info *bi, int n)
{
   char fline[BLSIZE + 1];
   int i;

   if (gsrch(wp.lastexpr, n, bi, &wp))   /* find first occurance of express */
     {
        clear_screen(&wp);
        invalidate(bi,wp.display_rows, 0);          /* and display it if found */
     }
   else
     {                                    /* If it was not found */
       for (i = 0; i < BLSIZE; fline[i++] = SPACE);
       strncpy(fline, pnotfnd, strlen(pnotfnd));
       put_bline(fline, BLSIZE, &wp);
     }
}

/*-----------------------------------------------------------------------*/
/*  find_next_occurance                                                  */
/*-----------------------------------------------------------------------*/
void find_next_occurance(struct buff_info *bi, int n)
{
   char fline[BLSIZE + 1];
   int i;

   if (gnsrch(wp.lastexpr, n, bi, &wp))  /* find first occurance of express */
     {
       clear_screen(&wp);
       invalidate(bi, wp.display_rows, 0);
     }
   else
     {                                    /* If it was not found */
       for (i = 0; i < BLSIZE; fline[i++] = SPACE);
       strncpy(fline, pnotfnd, strlen(pnotfnd));
       put_bline(fline, BLSIZE, &wp);
     }
}

/*-----------------------------------------------------------------------*/
/*  execute_command                                                      */
/*-----------------------------------------------------------------------*/
void execute_command(struct buff_info *bi)
{
   char cmdbuf[MAXCMD + 1];
   char *p, *q;
   int i;


   i = 0;
   p = wp.lastcmd;
   while ( i<MAXCMD )
     {
       if (*p)
         if (*p == PCT)
           for (q = bi->filename, p++; *q && i<MAXCMD; cmdbuf[i++] = *q++);
         else
           cmdbuf[i++] = *p++;
       else
         break;
     }
   cmdbuf[i] = 0;

   clear_screen(&wp);
   setcursorpos(0,0);
   printf("%s\n", cmdbuf);

   system(cmdbuf);

   printf("\n%s", AnyKey);
   getch();
}

/* ------------------------------------------------------------------------ */
/* formatline                                                               */
/*                                                                          */
/* Expand out the tabs to create a line ready for display.  Put its length  */
/* in wp.linlen.                                                            */
/* ------------------------------------------------------------------------ */
void formatline(struct buff_info *bi, off_t lined)
{
   char *p, *q, *eob;
   off_t left;
   int done, len, i;

   wp.linelen = 0;
   left = bi->Bib - (lined - bi->Tob);
   if ( left <= (wp.display_cols + 3) )
     refresh_buffer(bi, DOWN);

   eob = bi->bufaddr + bi->Bib;
   p = offsetToAddress(bi, lined);
   q = wp.linebuff;

   wp.linelen = 0;
   for (done = NO; !done && p != eob; p++)
     {
       switch (*p)
       {
          case CR:      /* We ignore all carriage returns          @4a */
          break;

          case LF:
          case CTLZ:
          case CTLL:    done = YES;
          break;

          case TAB:     len = TABEXPAN - (wp.linelen % TABEXPAN);
                        for (i = 0; i < len; i++)
                          *q++ = SPACE;
                        wp.linelen += len;
          break;

          default:      *q++ = *p;
                        wp.linelen++;
          break;
       }   /* end of switch */
       if (wp.linelen >= wp.display_cols)
         {
            wp.linelen = wp.display_cols;
            done = YES;
         }
     }
}

/*--------------------------------------------------------------------------*
 *  LogicalScrollDown                                                       *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void LogicalScrollDown(struct buff_info *bi, unsigned int lines)
{
    long lines_moved;
    char *savelb;
    char templn[30];

    lines_moved = moveTolsDown(bi, &wp, (long)lines);
    if (suppress_scrolling)    /* in the preferred mode of operation */
      if (lines_moved >= wp.display_rows)/* if scroll is a full window */
        {
          clear_window(&wp);                 /* clear the window */
          invalidate(bi, wp.display_rows, 0);    /* and repaint the whole thing */
        }
      else                                    /* if just a partial window */
        {
          PhysScrollUp((unsigned)lines_moved, &wp);   /* scroll to the correct*/
                                         /* place and repaint what's necessary*/
          invalidate(bi,(int)lines_moved, (int)(wp.display_rows - lines_moved));
        }
    else                       /* by default (for dumb terminals, I guess) */
      {
        if (lines_moved > wp.display_rows)
          {
             PhysScrollUp(1, &wp);
             savelb = wp.linebuff;
             strcpy(templn, cSkipping);
             ltoa(lines_moved - wp.display_rows, templn + SKIPVAL, 10);
             strcat(templn + strlen(templn), cLines);
             wp.linebuff = templn;
             wp.linelen = strlen(templn);
             putline(wp.display_rows - 1, &wp);
             wp.linebuff = savelb;
             lines_moved = wp.display_rows;
          }
        invalidate(bi, (int)lines_moved, (int)(wp.display_rows - lines_moved));
      }
}

/*--------------------------------------------------------------------------*
 *  LogicalScrollUp                                                         *
 *                                                                          *
 *--------------------------------------------------------------------------*/
void LogicalScrollUp(struct buff_info *bi, unsigned int lines)
{
   long int lines_moved;
    char *savelb;
    char templn[30];

   lines_moved = moveTolsUp(bi, &wp, (long)lines);
   if (suppress_scrolling)
     if (lines_moved >= wp.display_rows)
       {
          clear_window(&wp);
          invalidate(bi, wp.display_rows, 0);
       }
     else
       {
          PhysScrollDn((unsigned)lines_moved, &wp);
          invalidate(bi, (int)lines_moved, 0);
       }
   else
     {
       PhysScrollUp(1, &wp);
       savelb = wp.linebuff;
       strcpy(templn, cBack);
       ltoa(lines_moved, templn + BACKVAL, 10);
       strcat(templn + strlen(templn), cLines);
       wp.linebuff = templn;
       wp.linelen = strlen(templn);
       putline(wp.display_rows - 1, &wp);
       wp.linebuff = savelb;
       invalidate(bi, (int)wp.display_rows, 0);
     }
}

/*--------------------------------------------------------------------------*
 *  find_starting_line                                                      *
 *                                                                          *
-----------------------------------------------------------------------------*/

int find_starting_line(unsigned long line, char *pattern, struct buff_info *bi)
{

    if (line > 0)
      moveTolsDown(bi, &wp, line);                               // @3c

    if (pattern != NULL)
      {
        wp.lastexpr = pattern;
        gsrch(pattern, 1, bi, &wp);                              // @3c
      }
    return(YES);
}

/* ------------------------------------------------------------------------ */
/*    grepsrch                                                              */
/* ------------------------------------------------------------------------ */
off_t grepsrch(off_t start, char *pattern)
{
    return((off_t)LEOF);
}

/* ------------------------------------------------------------------------ */
/*    isattof                                                               */
/* ------------------------------------------------------------------------ */
int isattof(struct buff_info *bi)
{
   if (bi->Tols == (off_t)0)
     return(YES);
   else
     return(NO);
}

/* ------------------------------------------------------------------------ */
/*    isateof                                                               */
/* ------------------------------------------------------------------------ */
int isateof(struct buff_info *bi)
{
   if (bi->file_size == 0)
     if ( ((bi->Pbols - bi->Tob) >= bi->Bib) && (bi->status == -1) )
       return(YES);
     else
       return(NO);
   if (bi->Pbols >= bi->file_size)
     return(YES);
   else
     return(NO);
}

/*--------------------------------------------------------------------------*
 *  offsetToAddress                                                         *
 *                                                                          *
 *--------------------------------------------------------------------------*/
char *offsetToAddress(struct buff_info *bi, off_t offset)
{
    return(buf + (offset - bi->Tob));
}
