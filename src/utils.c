#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#define IOCTL_RETRY_COUNT 5


void* my_malloc(size_t size)
{
  void *ptr = malloc(size);
  if(ptr == NULL){
    log_message("Can't malloc memory! %s\n", strerror(errno));
    exit(1);
  }
  return ptr;
}

char* my_strdup(const char *s)
{
  char *ptr = strdup(s);
  if(ptr == NULL){
    log_message("Can't strdup! %s\n", strerror(errno));
    exit(1);
  }
  return ptr;
}


// varargs version for calling from another varargs function
void vlog_message(const char *format, va_list ap) 
{
  static FILE *output_stream = NULL;
  if(output_stream == NULL){
    output_stream = freopen("./lt_log.txt", "w", stderr);
    if(output_stream == NULL){
      printf("Error opening logfile!\n");
      return;
    }
  }
  time_t now = time(NULL);
  struct tm  *ts = localtime(&now);
  char       buf[80];
  strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
  
  fprintf(stderr, "[%s] ", buf);
  vfprintf(stderr, format, ap);
  fflush(stderr);
}

void log_message(const char *format, ...)
{
  va_list ap;
  va_start(ap,format);
  vlog_message(format, ap);
  va_end(ap);
}

int my_ioctl(int d, int request, void *argp)
{
  int cntr = 0;
  int res;
  
  do{
    res = ioctl(d, request, argp);
    if(0 == res){
      break;
    }else{
      if(errno != EIO){
        break;
      }
    }
    cntr++;
//    usleep(100);
  }while(cntr < IOCTL_RETRY_COUNT);
  return res;
}

void strlower(char *s)
{
  while (*s != '\0') {
    *s = tolower(*s);
    s++;
  }
}

char *my_strcat(char *str1, char *str2)
{
  size_t len1 = strlen(str1);
  size_t sum = len1 + strlen(str2) + 1; //Count trainling null too
  char *res = (char*)my_malloc(sum);
  strcpy(res, str1);
  strcpy(res + len1, str2);
  return res;
}
