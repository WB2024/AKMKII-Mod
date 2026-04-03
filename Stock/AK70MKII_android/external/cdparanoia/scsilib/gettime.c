#ifdef _WIN32

#include <windows.h>
#include <winsock.h>
#include <time.h>
#include <sys/timeb.h>
#include "timedefs.h"

struct timezone {
	int	tz_minuteswest;
	int	tz_dsttime;
};


/* Emulate gettimeofday (Ulrich Leodolter, 1/11/95).  */
void gettimeofday (struct timeval *tv, struct timezone *tz)
{
  struct _timeb tb;
  _ftime (&tb);

  tv->tv_sec = tb.time;
  tv->tv_usec = tb.millitm * 1000L;
  if (tz)
    {
      tz->tz_minuteswest = tb.timezone;	/* minutes west of Greenwich  */
      tz->tz_dsttime = tb.dstflag;	/* type of dst correction  */
    }
}

#endif
