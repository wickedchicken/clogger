#ifndef CLOGGER_CLOGGER_H
#define CLOGGER_CLOGGER_H

#include <stdio.h>

int clog_register_fd(int fd, int defaults);
int clog_register_fp(FILE *, int defaults);
int clog_register_fs(const char *filename, int append, int defaults);
int clog_deregister(int logid);

int clog_set_option(int logid, int option, int value);
int clog_get_option(int logid, int option);

int clog_set_global_option(int option, int value);
int clog_get_global_option(int option);

int clog_log(int logid, const char *module, int severity, const char *message);

int clog_initialize(int defaults);
int clog_teardown();

const char* clog_get_error();

/******* Logging Levels *********/
#define CLOGGER_LEV_NONE      0
#define CLOGGER_LEV_FATAL     1
#define CLOGGER_LEV_ERROR     2
#define CLOGGER_LEV_WARN      3
#define CLOGGER_LEV_INFO      4
#define CLOGGER_LEV_DEBUG     5
#define CLOGGER_LEV_TRACE     6
#define CLOGGER_LEV_ALL       7

/******* Logging Options ********/
#define CLOGGER_OP_COLOR      1
#define CLOGGER_OP_FSYNC      2
#define CLOGGER_OP_ERROR_H    3
#define CLOGGER_OP_SHOWTIME   4
#define CLOGGER_OP_SHOWMOD    5
#define CLOGGER_OP_SHOWSEV    6
#define CLOGGER_OP_SHOWPID    7
#define CLOGGER_OP_MAXSEV     8
#define CLOGGER_OP_AUTOCLOSE  9

// so as not to be confused with normal options
// we keep the number incrementing
#define CLOGGER_GLOP_FAILFAST 9

/******* Defaults for Initialize *******/
#define CLOGGER_INIT_NONE    -1
// default = full logging inc. debug
#define CLOGGER_INIT_DEFAULT  0
// force no color
#define CLOGGER_INIT_NOCOLOR  1
// normal = info, warn and errors
#define CLOGGER_INIT_NORMAL   2
// NC = no color
#define CLOGGER_INIT_NORMALNC 3
// quiet = only errors
#define CLOGGER_INIT_QUIET    4
// no color
#define CLOGGER_INIT_QUIETNC  5

/******* Default Log IDs *******/
#define CLOGGER_LOG_ALL      -1
#define CLOGGER_LOG_STDERR    0

/******* Time Output Options ********/
#define CLOGGER_TIME_NONE     0
#define CLOGGER_TIME_TS       1
#define CLOGGER_TIME_EPOCH    2

/******* Error Conditions *******/
#define CLOGGER_ERR_OK        0
#define CLOGGER_ERR_GENERAL  -1
#define CLOGGER_ERR_NOTFOUND -2
#define CLOGGER_ERR_WRITE    -3
#define CLOGGER_ERR_SYSTEM   -4
#define CLOGGER_ERR_MEM      -5
#define CLOGGER_ERR_INIT     -6
#define CLOGGER_ERR_INV_ID   -7

#endif
