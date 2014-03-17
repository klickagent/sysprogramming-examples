/* (C) IT Sky Consulting GmbH 2014
 * http://www.it-sky-consulting.com/
 * Author: Karl Brodowsky
 * Date: 2014-02-27
 * License: GPL v2 (See https://de.wikipedia.org/wiki/GNU_General_Public_License )
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;

#define TRUE 1
#define FALSE 0

#define SIZE 1024
#define THREAD_COUNT 10

char buff[SIZE];

int done = FALSE;

void handle_thread_error(int retcode) {
  if (retcode < 0) {
    printf("thread error %d\n", retcode);
    exit(1);
  }
}

void *thread_run(void *ptr) {
  // warning: name gets changed by main-thread all the time!!!
  char *name = ptr;
  char *orig_name = strdup(name);
  int retcode;
  printf("%s (%ld) started (orig_name=%s)\n", name, (long) pthread_self(), orig_name);
  sleep(5);
  printf("%s (%ld) done (orig_name=%s)\n", name, (long) pthread_self(), orig_name);
  return (void *) NULL;
}

int main(int argc, char *argv[]) {
  pthread_t thread[THREAD_COUNT];
  int retcode;
  retcode = pthread_mutex_init(&mutex1, NULL);
  handle_thread_error(retcode);

  int i;
  for (i = 0; i < THREAD_COUNT; i++) {
    char raw_name[20];
    sprintf(raw_name, "thread %3d", i);
    printf("main: starting %s\n", raw_name);
    retcode = pthread_create(&(thread[i]), NULL, thread_run, raw_name);
    handle_thread_error(retcode);
    printf("main: started %s\n", raw_name);
    sleep(1);
  }
  for (i = 0; i < THREAD_COUNT; i++) {
    printf("main: joining thread %d\n", i);
    retcode = pthread_join(thread[i], NULL);
    handle_thread_error(retcode);
    printf("main: joined thread %d\n", i);
  }
  printf("DONE\n");
  exit(0);
}
