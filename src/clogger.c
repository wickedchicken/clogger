#include "clogger.h"
#include "clogger_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define CLOGGER_INITIAL_POOL 4 // initial pool of IDs to allocate
#define MAX_HEADER           1024 // max size of header

int clog_num_registered=0;
int clog_allocated=0;
struct clog_log_id *clog_registered_ids = NULL;
char* clog_error = "";
int clog_failfast = 0;

static int set_stuff(int logid, int defaults, int istty){
  int result;
  switch(defaults){
    case CLOGGER_INIT_DEFAULT:
      if(istty){
        if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_COLOR,1))){ return result; }
      }
    case CLOGGER_INIT_NOCOLOR:
      if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_MAXSEV,CLOGGER_LEV_ALL))){ return result; }
      break;

    case CLOGGER_INIT_NORMAL:
      if(istty){
        if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_COLOR,1))){ return result; }
      }
    case CLOGGER_INIT_NORMALNC:
      if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_MAXSEV,CLOGGER_LEV_INFO))){ return result; }
      break;

    case CLOGGER_INIT_QUIET:
      if(istty){
        if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_COLOR,1))){ return result; }
      }
    case CLOGGER_INIT_QUIETNC:
      if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_MAXSEV,CLOGGER_LEV_ERROR))){ return result; }
      break;
    default:
      clog_error = "logging initialization was passed invalid parameter";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_INIT;
  }

  if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_ERROR_H,0))){ return result; }
  if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_FSYNC,1))){ return result; }
  if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_SHOWTIME,CLOGGER_TIME_TS))){ return result; }
  if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_SHOWMOD,1))){ return result; }
  if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_SHOWPID,0))){ return result; }
  if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_SHOWSEV,1))){ return result; }
  if(CLOGGER_ERR_OK != (result = clog_set_option(logid,CLOGGER_OP_AUTOCLOSE,0))){ return result; }

  return CLOGGER_ERR_OK;
}

static int clog_realloc_ids(int requested){
  if(requested >= clog_allocated){
    int newsize = clog_allocated*2;
    struct clog_log_id * new_reg_ids = (struct clog_log_id *)realloc(clog_registered_ids,newsize*sizeof(struct clog_log_id));
    if(NULL == new_reg_ids){
      clog_error = "could not allocate memory for logging";
      if(clog_failfast){ perror(clog_error); exit(1); }
      return CLOGGER_ERR_MEM;
    }

    memset(clog_registered_ids + clog_allocated,0,sizeof(struct clog_log_id) * (newsize - clog_allocated));
    clog_allocated = newsize;
    clog_registered_ids = new_reg_ids;
  }

  return CLOGGER_ERR_OK;
}

static int find_or_make_slot(int *result){
  int idx = 0;
  int idx_set = 0;
  int i=0;
  for(i=0;i < clog_num_registered;i++){
    // make sure we don't use up first slot to ensure CLOGGER_LOG_STDERR stays invalid
    if((CLOGGER_TYPE_NONE == clog_registered_ids[i].type) && (i > 0)){
      idx = i;
      idx_set = 1;
      break;
    }
  }

  if(!idx_set){
    int result;
    if(CLOGGER_ERR_OK != (result = clog_realloc_ids(clog_num_registered+1))){ return result; }
    idx = clog_num_registered;
  }

  *result = idx;

  return CLOGGER_ERR_OK;
}

/* woo linear search! the fastest in the land! */
/* obviously switch this to a hash or something later */
int clog_register_fd(int fd, int defaults){
  int idx;
  int result;
  if(CLOGGER_ERR_OK != (result = find_or_make_slot(&idx))){ return result; }

  clog_registered_ids[idx].type = CLOGGER_TYPE_FD;
  clog_registered_ids[idx].fd = fd;
  clog_num_registered++;
  if(CLOGGER_ERR_OK != (result = set_stuff(idx, defaults, (1 == isatty(fd))))){ return result; }

  return idx;
}

int clog_register_fp(FILE *fp, int defaults){
  int idx;
  int result;
  if(CLOGGER_ERR_OK != (result = find_or_make_slot(&idx))){ return result; }

  clog_registered_ids[idx].type = CLOGGER_TYPE_FP;
  clog_registered_ids[idx].fp = fp;
  clog_num_registered++;
  if(CLOGGER_ERR_OK != (result = set_stuff(idx, defaults, (1 == isatty(fileno(fp)))))){ return result; }

  return idx;
}

int clog_register_fs(const char *filename, int append, int defaults){
  int idx;
  int result;
  if(CLOGGER_ERR_OK != (result = find_or_make_slot(&idx))){ return result; }

  const char *modestring;
  if(append){
    modestring = "a";
  }
  else{
    modestring = "w";
  }

  FILE* fp = fopen(filename,modestring);
  if(NULL == fp){
    clog_error = "error opening logfile";
    if(clog_failfast){ perror(clog_error); exit(1); }
    return CLOGGER_ERR_SYSTEM;
  }

  clog_registered_ids[idx].type = CLOGGER_TYPE_FP;
  clog_registered_ids[idx].fp = fp;
  clog_num_registered++;
  if(CLOGGER_ERR_OK != (result = set_stuff(idx, defaults, (1 == isatty(fileno(fp)))))){ return result; }

  if(CLOGGER_ERR_OK != (result = clog_set_option(idx,CLOGGER_OP_AUTOCLOSE,1))){ return result; }

  return idx;
}

static int clog_deregister_internal(int logid){
  switch(clog_registered_ids[logid].type){
    case CLOGGER_TYPE_NONE:
      break;
    case CLOGGER_TYPE_FD:
      if(clog_registered_ids[logid].autoclose){
        fsync(clog_registered_ids[logid].fd);
        if(0 != close(clog_registered_ids[logid].fd)){
          clog_error = "error closing out log stream";
          if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
          return CLOGGER_ERR_SYSTEM;
        }
      }
      break;
    case CLOGGER_TYPE_FP:
    case CLOGGER_TYPE_FN:
      if(clog_registered_ids[logid].autoclose){
        fsync(fileno(clog_registered_ids[logid].fp));
        if(0 != fclose(clog_registered_ids[logid].fp)){
          clog_error = "error closing out log stream";
          if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
          return CLOGGER_ERR_SYSTEM;
        }
      }
      break;
    default:
      clog_error = "invalid log stream type detected -- shouldn't happen";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_GENERAL;
  }

  clog_registered_ids[logid].type = CLOGGER_TYPE_NONE;
  return CLOGGER_ERR_OK;
}

int clog_deregister(int logid){
  if((logid < CLOGGER_LOG_ALL) || (logid >= clog_num_registered)){
    clog_error = "logging deregister was passed invalid log ID";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INV_ID;
  }

  int result;
  if(CLOGGER_LOG_ALL == logid){
    int i=0;
    for(i=0;i<clog_num_registered;i++){
      if(clog_registered_ids[i].type != CLOGGER_TYPE_NONE){
        if(CLOGGER_ERR_OK != (result = clog_deregister_internal(i))){
          return result;
        }
      }
    }
  }
  else{
    if(CLOGGER_ERR_OK != (result = clog_deregister_internal(logid))){
      return result;
    }
  }

  return CLOGGER_ERR_OK;
}

static int clog_option_setter(int logid, int option, int value){
  if(CLOGGER_TYPE_NONE == clog_registered_ids[logid].type){
    clog_error = "logging set_option was passed deregistered log ID";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INV_ID;
  }

  switch(option){
    case CLOGGER_OP_COLOR:
      clog_registered_ids[logid].color = value;
      break;
    case CLOGGER_OP_FSYNC:
      clog_registered_ids[logid].fsync = value;
      break;
    case CLOGGER_OP_ERROR_H:
      clog_registered_ids[logid].error_h = value;
      break;
    case CLOGGER_OP_SHOWTIME:
      if((value != CLOGGER_TIME_TS) &&
          (value != CLOGGER_TIME_NONE) &&
          (value != CLOGGER_TIME_EPOCH)){
        clog_error = "invalid time display option specified to logger";
        if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
        return CLOGGER_ERR_NOTFOUND;
      }
      clog_registered_ids[logid].time = value;
      break;
    case CLOGGER_OP_SHOWMOD:
      clog_registered_ids[logid].mod = value;
      break;
    case CLOGGER_OP_SHOWSEV:
      clog_registered_ids[logid].sev = value;
      break;
    case CLOGGER_OP_SHOWPID:
      clog_registered_ids[logid].pid = value;
      break;
    case CLOGGER_OP_MAXSEV:
      clog_registered_ids[logid].max_sev = value;
      break;
    case CLOGGER_OP_AUTOCLOSE:
      clog_registered_ids[logid].autoclose = value;
      break;
    default:
      clog_error = "invalid option specified to logger";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_NOTFOUND;
      break;
  }

  return CLOGGER_ERR_OK;
}

int clog_set_option(int logid, int option, int value){
  if((logid < CLOGGER_LOG_ALL) || (logid >= clog_num_registered)){
    clog_error = "logging set_option was passed invalid log ID";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INV_ID;
  }

  int result;
  if(CLOGGER_LOG_ALL == logid){
    int sawany = 0;
    int i=0;
    for(i=0;i<clog_num_registered;i++){
      if(clog_registered_ids[i].type != CLOGGER_TYPE_NONE){
        if(CLOGGER_ERR_OK != (result = clog_option_setter(i,option,value))){
          return result;
        }
        sawany = 1;
      }
    }

    // TODO: is this an error?
    if(!sawany){
      clog_error = "logging options set when no log IDs were registered";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_INV_ID;
    }
  }
  else{
    if(CLOGGER_ERR_OK != (result = clog_option_setter(logid,option,value))){
      return result;
    }
  }

  return CLOGGER_ERR_OK;
}

int clog_get_option(int logid, int option){
  if((logid <= CLOGGER_LOG_ALL) || (logid >= clog_num_registered)){
    clog_error = "logging get_option was passed invalid log ID";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INV_ID;
  }
  if(CLOGGER_TYPE_NONE == clog_registered_ids[logid].type){
    clog_error = "logging get_option was passed deregistered log ID";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INV_ID;
  }

  switch(option){
    case CLOGGER_OP_COLOR:
      return clog_registered_ids[logid].color;
      break;
    case CLOGGER_OP_FSYNC:
      return clog_registered_ids[logid].fsync;
      break;
    case CLOGGER_OP_ERROR_H:
      return clog_registered_ids[logid].error_h;
      break;
    case CLOGGER_OP_SHOWTIME:
      return clog_registered_ids[logid].time;
      break;
    case CLOGGER_OP_SHOWMOD:
      return clog_registered_ids[logid].mod;
      break;
    case CLOGGER_OP_SHOWSEV:
      return clog_registered_ids[logid].sev;
      break;
    case CLOGGER_OP_SHOWPID:
      return clog_registered_ids[logid].pid;
      break;
    case CLOGGER_OP_MAXSEV:
      return clog_registered_ids[logid].max_sev;
      break;
    case CLOGGER_OP_AUTOCLOSE:
      return clog_registered_ids[logid].autoclose;
      break;
    default:
      clog_error = "invalid option requested from logger";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_NOTFOUND;
      break;
  }
} 

int clog_set_global_option(int option, int value){
  switch(option){
    case CLOGGER_GLOP_FAILFAST:
      clog_failfast = value;
      break;
    default:
      clog_error = "invalid option specified to logger";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_NOTFOUND;
      break;
  }

  return CLOGGER_ERR_OK;
}

static int clog_log_internal(int logid, const char *module, int severity, const char *message){
  if(CLOGGER_TYPE_NONE == clog_registered_ids[logid].type){
    clog_error = "logging was passed deregistered log ID";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INV_ID;
  }

  if(severity > clog_registered_ids[logid].max_sev){
    return CLOGGER_ERR_OK;
  }

  int wrote_idx = 0;
  char header[MAX_HEADER +1];

  if(CLOGGER_TIME_TS == clog_registered_ids[logid].time){

/* credit for this portion goes to:
   http://stackoverflow.com/questions/1551597/using-strftime-in-c-how-can-i-format-time-exactly-like-a-unix-timestamp
*/ 

    char fmt[64], buf[64];
    struct timeval  tv;
    struct tm       *tm;

    gettimeofday(&tv, NULL);
    if((tm = localtime(&tv.tv_sec)) != NULL)
    {
            strftime(fmt, sizeof fmt, "%Y-%m-%d %H:%M:%S.%%06u %z", tm);
            snprintf(buf, sizeof buf, fmt, tv.tv_usec);
    }

/*******/

    snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%36s]\t", buf);
    wrote_idx += 39;
  }
  if(CLOGGER_TIME_EPOCH == clog_registered_ids[logid].time){
    struct timeval tv;
    gettimeofday(&tv, NULL);

    snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%9u.%06u]\t",(unsigned int)tv.tv_sec,(unsigned int)tv.tv_usec);
    wrote_idx += 19;
  }

  if(clog_registered_ids[logid].pid){
    snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "%06u\t",getpid());
    wrote_idx += 7;
  }
  if(clog_registered_ids[logid].mod){
    snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%-20s]\t",module);
    wrote_idx += 23;
  }
  if(clog_registered_ids[logid].sev){
    const char* red = "\033[0;40;31m";
    const char* cyan = "\033[0;40;36m";
    const char* yellow = "\033[0;40;33m";
    const char* magenta = "\033[0;40;35m";
    const char* normal = "\033[0m";
    if(!clog_registered_ids[logid].color){
      red = cyan = yellow = magenta = normal = "";
    }

    switch(severity){
      case CLOGGER_LEV_FATAL:
        snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%sFATAL",red);
        wrote_idx += 6 + strlen(red);
        break;
      case CLOGGER_LEV_ERROR:
        snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%sERROR",red);
        wrote_idx += 6 + strlen(red);
        break;
      case CLOGGER_LEV_WARN:
        snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%sWARN ",yellow);
        wrote_idx += 6 + strlen(yellow);
        break;
      case CLOGGER_LEV_INFO:
        snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%sINFO ",normal);
        wrote_idx += 6 + strlen(normal);
        break;
      case CLOGGER_LEV_DEBUG:
        snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%sDEBUG",magenta);
        wrote_idx += 6 + strlen(magenta);
        break;
      case CLOGGER_LEV_TRACE:
        snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "[%sTRACE",cyan);
        wrote_idx += 6 + strlen(cyan);
        break;
      default:
        clog_error = "invalid severity provided to logging -- shouldn't happen";
        if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
        return CLOGGER_ERR_GENERAL;
    }

    snprintf(header + wrote_idx, (MAX_HEADER - wrote_idx), "%s]\t",normal);
    wrote_idx += 2 + strlen(normal);
  }

  assert(wrote_idx < MAX_HEADER);
  assert(strlen(header) == wrote_idx);
  header[MAX_HEADER] = '\0'; // just in case

  switch(clog_registered_ids[logid].type){
    case CLOGGER_TYPE_FD:
      if(-1 == write(clog_registered_ids[logid].fd,header,wrote_idx)){
        clog_error = "error writing to log";
        if(clog_failfast){ perror(clog_error); exit(1); }
        return CLOGGER_ERR_WRITE;
      }
      if(-1 == write(clog_registered_ids[logid].fd,message,strlen(message))){
        clog_error = "error writing to log";
        if(clog_failfast){ perror(clog_error); exit(1); }
        return CLOGGER_ERR_WRITE;
      }
      if(-1 == write(clog_registered_ids[logid].fd,"\n",1)){
        clog_error = "error writing to log";
        if(clog_failfast){ perror(clog_error); exit(1); }
        return CLOGGER_ERR_WRITE;
      }

      break;
    case CLOGGER_TYPE_FP:
    case CLOGGER_TYPE_FN:
      if(wrote_idx > fwrite(header,sizeof(char),wrote_idx,clog_registered_ids[logid].fp)){
        clog_error = "error writing to log";
        if(clog_failfast){ perror(clog_error); exit(1); }
        return CLOGGER_ERR_WRITE;
      }
      if(strlen(message) >  fwrite(message,sizeof(char),strlen(message),clog_registered_ids[logid].fp)){
        clog_error = "error writing to log";
        if(clog_failfast){ perror(clog_error); exit(1); }
        return CLOGGER_ERR_WRITE;
      }
      if(1 > fwrite("\n",sizeof(char),1,clog_registered_ids[logid].fp)){
        clog_error = "error writing to log";
        if(clog_failfast){ perror(clog_error); exit(1); }
        return CLOGGER_ERR_WRITE;
      }
      break;
    default:
      clog_error = "invalid log stream type detected -- shouldn't happen";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_GENERAL;
  }

  if(clog_registered_ids[logid].fsync){
    switch(clog_registered_ids[logid].type){
      case CLOGGER_TYPE_FD:
        fsync(clog_registered_ids[logid].fd);
        break;
      case CLOGGER_TYPE_FP:
      case CLOGGER_TYPE_FN:
        fsync(fileno(clog_registered_ids[logid].fp));
        break;
      default:
        clog_error = "invalid log stream type detected -- shouldn't happen";
        if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
        return CLOGGER_ERR_GENERAL;
    }
  }

  return CLOGGER_ERR_OK;
}

int clog_log(int logid, const char *module, int severity, const char *message){
  if((logid < CLOGGER_LOG_ALL) || (logid >= clog_num_registered)){
    clog_error = "logging was passed invalid log ID";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INV_ID;
  }

  if((severity < CLOGGER_LEV_FATAL) || (severity > CLOGGER_LEV_TRACE)){
    clog_error = "logging severity outside valid range";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_GENERAL;
  }

  int result;
  if(CLOGGER_LOG_ALL == logid){
    int i=0;
    for(i=0;i<clog_num_registered;i++){
      if(clog_registered_ids[i].type != CLOGGER_TYPE_NONE){
        if(CLOGGER_ERR_OK != (result = clog_log_internal(i, module, severity, message))){
          return result;
        }
      }
    }
  }
  else{
    if(CLOGGER_ERR_OK != (result = clog_log_internal(logid, module, severity, message))){
      return result;
    }
  }

  return CLOGGER_ERR_OK;
}

int clog_get_global_option(int option){
  switch(option){
    case CLOGGER_GLOP_FAILFAST:
      return clog_failfast;
      break;
    default:
      clog_error = "invalid option specified to logger";
      if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
      return CLOGGER_ERR_NOTFOUND;
      break;
  }
}

int clog_initialize(int defaults){
  int result;

  if((clog_num_registered != 0) ||
      (clog_registered_ids != NULL) ||
      (clog_allocated != 0)){
    clog_error = "previous logger instance was not properly torn down";
    if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
    return CLOGGER_ERR_INIT;
  }

  clog_num_registered = 0; // just in case
  clog_registered_ids = (struct clog_log_id *)calloc(CLOGGER_INITIAL_POOL,sizeof(struct clog_log_id));
  if(NULL == clog_registered_ids){
    clog_error = "could not allocate memory for logging";
    if(clog_failfast){ perror(clog_error); exit(1); }
    return CLOGGER_ERR_MEM;
  }
  clog_allocated = CLOGGER_INITIAL_POOL;

  if(defaults != CLOGGER_INIT_NONE){
      int slot = clog_register_fp(stderr,defaults);
      if(CLOGGER_LOG_STDERR != slot){
        clog_error = "strange allocation for stderr";
        if(clog_failfast){ fprintf(stderr, "FATAL: %s\n", clog_error); exit(1); }
        return CLOGGER_ERR_INIT;
      }

      /* this is stderr, no need for separate fsync */
      if(CLOGGER_ERR_OK != (result = clog_set_option(slot,CLOGGER_OP_FSYNC,0))){ return result; }
      if(CLOGGER_ERR_OK != (result = clog_set_option(slot,CLOGGER_OP_SHOWTIME,CLOGGER_TIME_NONE))){ return result; }
      if(CLOGGER_ERR_OK != (result = clog_set_option(slot,CLOGGER_OP_SHOWMOD,0))){ return result; }
      if(CLOGGER_ERR_OK != (result = clog_set_option(slot,CLOGGER_OP_SHOWPID,0))){ return result; }
  }
  else{
    /* make sure CLOGGER_LOG_STDERR is now invalid */
    clog_num_registered++;
  }

  return CLOGGER_ERR_OK;
}

int clog_teardown(){
  int result;
  if(CLOGGER_ERR_OK != (result = clog_deregister(CLOGGER_LOG_ALL))){ return result; }
  clog_num_registered = 0;
  free(clog_registered_ids);
  clog_registered_ids = 0;
  clog_allocated = 0;
  return CLOGGER_ERR_OK;
}

const char* clog_get_error(){
  return clog_error;
}

