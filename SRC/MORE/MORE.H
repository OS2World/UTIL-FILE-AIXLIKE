/* @(#)58	1.1  src/more/more.h, aixlike.src, aixlike3  9/27/95  15:45:02 */
/* more.h */

/* constants and structures shared by the modules that make up more.exe */

#define MORE_MAX_FILES   64

#define TABEXPAN     5             /* tabs are every 5 columns */
#define TAB       0x05
#define SPACE     0x20
#define CR        0x0d
#define LF        0x0a
#define BEL       0x07
#define DEL       0x08
#define COLON      ':'
#define DASH       '-'
#define SLASH      '/'
#define SLASHS     "/"
#define BACKSLASH  '\\'
#define PLUS       '+'
#define DOT        '.'
#define XCLM       '!'
#define XCLMS      "!"
#define ELLIPSIS   "..."
#define LPAREN     '('
#define RPAREN     ')'
#define ASTERISK   '*'
#define QMARK      '?'
#define PCT        '%'
#define CTLB 2
#define CTLD 4
#define CTLL 11
#define CTLM 12
#define CTLZ 26
#define PGUP 73
#define PGDN 81
#define UPARROW 72
#define DNARROW 80
#define KEEP_GOING 1
#define BAILOUT -1
#define YES 1
#define NO  0
#define UP  1
#define DOWN -1
#define LEOF -1l
#define LTOF -2l
#define CPLEOF (char *)-1
#define CPLTOF (char *)-2
#define BLSIZE         80
#define BLLPAREN        6
#define BLPCT           7
#define BLLINE         13
#define BLLINENO       18
#define MAXLINELEN    160
#define MAXUPLIN     1024
#define BUFSIZE     50000
#define FGETSIZE    32767
#define MAXMOVE     35000
#define TOPM         2000
#define BOTM         2000
#define MINMOVE      5000
#define DEFAULTATTR  0x07
#define MAXCMD        256

#define NOCMD           0
#define INVALID         0
#define SCROLL_DOWN     1
#define LSKIP_DOWN      2
#define MOVE_DOWN       3
#define MOVE_UP         4
#define CHG_SIZE        5
#define SSKIP_DOWN      6
#define QUIT            7
#define DLINENO         8
#define DFILENAME       9
#define HELP           10
#define FINDEXPR       11
#define FINDNTH        12
#define EXECCMD        13
#define FSKIP_DOWN     14
#define FSKIP_UP       15
#define SSKIP_UP       16
#define REPEAT         17
#define SCROLL_UP      18
#define MOVE_DOWN_SCREEN 19
#define MOVE_UP_SCREEN 20

struct window_parms {        /* parameters that affect window characteristics*/
     int  scroll_size;         /* number of lines in a 'scroll' operation */
     int  move_size;           /* default lines in a 'move' operation */
     int  window_rows;         /* depth of the entire window */
     int  display_rows;        /* depth of the part used for data */
     int  top_row;             /* screen row corresponding to top of window */
     int  left_col;            /* screen column corresponding to left margin */
     int  display_cols;        /* width of the window */
     char clearcell[2];        /* data used to clear lines */
     char *lastcmd;            /* last command entered */
     char *lastexpr;           /* last search expression entered */
     char *linebuff;           /* a buffer to use for line formatting */
     int  linelen;             /* length of data in linebuff */
                    };

struct buff_info {
     FILE   *stream;
     int     status;           /* status of stream - particularly, eof */
     char   *bufaddr;          /* address of buffer for this file */
     off_t   bufsize;          /* size of buffer in bytes */
     off_t   Bib;              /* bytes in buffer */
     off_t   file_size;        /* size of file in bytes */
     long    lineno;           /* line number at Tols */
     char   *filename;         /* pointer to file name */
     off_t   Tob;              /* offset in file of data at top of buffer */
     off_t   Tols;             /* ditto data at top of logical screen */
     off_t   Pbols;            /* offset in file of first line after log scrn*/
                 };

/* function prototypes */
                                /* in movetols.c */
long moveTolsDown(struct buff_info *, struct window_parms *, long);
long moveTolsUp(struct buff_info *, struct window_parms *, long);
long oneLineDown(off_t, struct buff_info *, struct window_parms *);
long oneLineUp  (off_t, struct buff_info *, struct window_parms *);
void restoreTols(off_t, off_t, off_t, struct buff_info *);

                                /* in moredio.c */
int openit(char *, struct buff_info **, char *);                        // @3c
int refresh_buffer(struct buff_info *, int);
void closeit(struct buff_info *);

                                /* in mogetcmd.c */
int getcmd(unsigned *, struct buff_info *, struct window_parms *);

                                /* in morevio.c */
void display_help(char **, char *, struct window_parms *);
void put_bline(char *, int, struct window_parms *);
void putline(int, struct window_parms *);
int  getvmode(int *, int *);
char get_cell_attributes_at_cursor(void);
void getcursorpos(int *, int *);
void setcursorpos(int, int);
void clear_window(struct window_parms *);
void clear_screen(struct window_parms *);
void clear_eol(int, int, struct window_parms *);
void PhysScrollUp(unsigned int, struct window_parms *);
void PhysScrollDn(unsigned int, struct window_parms *);
void file_has_disappeared(char *, char *, struct window_parms *);

                                /* in morusage.c */
void tell_usage(void);
                                /* in myerror */
extern char *myerror_pgm_name;
void myerror(int, char *, char *);  /* error logging routine */
                                /* in moregrep.c */
long gsrch(char *, int, struct buff_info *, struct window_parms *);
long gnsrch(char *, int, struct buff_info *, struct window_parms *);
int natural_reg_expression(unsigned char *, unsigned char *, int);
