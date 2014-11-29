/* @(#)26	1.1  src/at/at.h, aixlike.src, aixlike3  9/27/95  15:43:52 */
/* h file for at */

 char *lmnames = "JANUARY,FEBRUARY,MARCH,APRIL,MAY,JUNE,JULY,AUGUST,SEPTEMBER,\
             OCTOBER,NOVEMBER,DECEMBER";
 char *smnames = "JAN,FEB,MAR,APR,MAY,JUN,JUL,AUG,SEP,OCT,NOV,DEC";
 char *sdnames = "SUN,MON,TUE,WED,THU,FRI,SAT";
 char *ldnames = "SUNDAY,MONDAY,TUESDAY,WEDNESDAY,THURSDAY,FRIDAY,SATURDAY";
 char *ttnames = "TODAY,TOMORROW,NOON,MIDNIGHT,NOW";
 char *utnames = "NEXT,MINUTE,MINUTES,HOUR,HOURS,DAY,DAYS,MONTH,MONTHS,YEAR,YEARS,WEEK,WEEKS";
 char *mtnames = "AM,PM,ZULU";
 char *AtJobDir    = "C:\\at\\jobs";
 char *AtJobEnvStr  = "ATJOBS=C:\\at\\jobs";
 char *AtSharedSeg     = "\\SHAREMEM\\ATD.DAT";
 char *AtIconFile   = "\\atd.ico";
 char *AtIconFileName    = "atd.ico";
 char *AtDaemonFile     = "\\atd.exe";
 char *AtDaemonFileName    = "atd.exe";
 char *AtDaemonProgramTitle     = "at Daemon";
 char *atjobs  = "ATJOBS";
 char *outjobdir = "OUTPUT";
 char *qname = "\\QUEUES\\xxxxx.q";



#define BAILOUT        -1
#define OK              0


#define AM             10
#define DAY            20
#define DAYS           30
#define HOUR           40
#define HOURS          50
#define MIDNIGHT       60
#define MINUTE         70
#define MINUTES        80
#define MONTH          90
#define MONTHS        100
#define NEXT          110
#define NOON          120
#define NOW           130
#define PM            140
#define TODAY         150
#define TOMORROW      160
#define WEEK          170
#define WEEKS         180
#define YEAR          190
#define YEARS         200
#define ZULU          210

#define HYPHEN   '-'
#define COMMA    ','
#define PLUS     '+'
#define COLON    ':'
#define PERIOD   '.'
#define SLASH    '/'
#define BACKSLASH '\\'
#define YES       1
#define NO        0

#define NOTSET     0
#define USECSHELL  1
#define USEKORNSHELL 2
#define USEBOURNESHELL 3

#define ATSET      1
#define ATDELETE   2
#define ATLIST     3

#define NOONTIME (time_t)(43200)
#define MIDNIGHTTIME (time_t)0
#define BADTIME (time_t)-1
#define DAYSTIME (time_t)(86400)

#define BADDATE (time_t)-1

#define MIN_INCREMENT (time_t)60
#define HR_INCREMENT  (time_t)(3600)
#define DAY_INCREMENT (time_t)(86400)

#define BADWDAY -1
#define BADMONTH -1
#define BADPATH -1
#define PATHOK 0
#define DAYFIRST 0
#define DAYLAST  1
