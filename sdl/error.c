/*
    error.c --
    Error logging 
*/

#include "osd.h"

FILE *error_log;

struct {
  int32_t enabled;
  int32_t verbose;
  FILE *log;
} t_error;

void error_init(void)
{
#ifdef LOGERROR
  error_log = fopen("error.log","w");
#endif
}

void error_shutdown(void)
{
#ifdef LOGERROR
  if(error_log) fclose(error_log);
#endif
}

void error(int8_t *format, ...)
{
#ifdef LOGERROR
  if (!log_error) return;
  va_list ap;
  va_start(ap, format);
  if(error_log) vfprintf(error_log, format, ap);
  va_end(ap);
#endif
}

