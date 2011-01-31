#include <clogger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(char* argv[]){
  fprintf(stderr, "usage: %s [-v D] [filename] where D is a debug level from 1-6\n",argv[0]);
  exit(0);
}

int main(int argc, char* argv[]){
  clog_set_global_option(CLOGGER_GLOP_FAILFAST,1);
  clog_initialize(CLOGGER_INIT_NORMAL);

  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_INFO,"starting program...");

  for(int i=1;i < argc;i++){
    if(!strcmp(argv[i],"-v")){
      if(i < (argc-1)){
        i++;
        char *end;
        long level = strtol(argv[i],&end, 10);
        if(*end != '\0'){
          char temp[1024];
          snprintf(temp,1024, "not a valid integer: %s, defaulting to 4...", argv[i]);
          clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_WARN,temp);
        }
        else{
          if((level < CLOGGER_LEV_FATAL) || (level > CLOGGER_LEV_TRACE)){
            clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_WARN,"-v not in valid range (1-6), defaulting to 4...");
          }
          else{
            // we could use CLOGGER_LOG_STDERR if we wanted to be more specific
            clog_set_option(CLOGGER_LOG_ALL,CLOGGER_OP_MAXSEV,level);
          }
        }
      }
      else{
        clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_WARN,"-v needs an argument! defaulting to 4...");
      }
    }
    else{
      if('-' == argv[i][0]){
        char temp[1024];
        snprintf(temp,1024, "invalid option: %s", argv[i]);
        clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_ERROR,temp);
        print_usage(argv);
      }
      else{
        clog_register_fn(argv[i],1,CLOGGER_INIT_DEFAULT);
        char temp[1024];
        snprintf(temp,1024, "registered logfile %s", argv[i]);
        clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_INFO,temp);
      }
    }

  }

  printf("\n now displaying logging depending on your error level:\n\n");

  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_FATAL,"fatal error");
  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_ERROR,"standard error");
  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_WARN,"warning");
  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_INFO,"information");
  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_DEBUG,"debugging information");
  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_TRACE,"tracing information");


  clog_log(CLOGGER_LOG_ALL,"main",CLOGGER_LEV_INFO,"exiting program...");

  return 0;
}
