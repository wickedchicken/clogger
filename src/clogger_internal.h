#ifndef CLOGGER_CLOGGER_INTERNAL_H
#define CLOGGER_CLOGGER_INTERNAL_H

#include <stddef.h>
#include <stdio.h>

enum clog_types{
  CLOGGER_TYPE_NONE=0,
  CLOGGER_TYPE_FP,
  CLOGGER_TYPE_FD,
  CLOGGER_TYPE_FN
};

struct clog_log_id{
  enum clog_types   type;
  FILE*             fp;
  int               fd;
  const char*       fn;
  int               color;
  int               fsync;
  int               error_h;
  int               time;
  int               mod;
  int               sev;
  int               pid;
  int               max_sev;
  int               autoclose;
};

extern int clog_num_registered;
extern int clog_allocated;
extern struct clog_log_id *clog_registered_ids;
extern char* clog_error;

extern int clog_failfast;

#endif
