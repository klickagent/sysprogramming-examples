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

void handle_thread_error(int retcode) {
  if (retcode < 0) {
    printf("thread error %d\n", retcode);
    exit(1);
  }
}

void print_info(char *txt) {
  /** warning gettid is VERY Linux-specific: */
  pid_t pid = getpid(); // gettid();
  printf("%-30s: threadId=%ld pid=%ld\n", txt, (long) pthread_self(), (long) pid);
}


void *thread_run(void *ptr) {
  print_info(ptr);
  return (void *) NULL;
}

int main(int argc, char *argv[]) {
  pthread_t thread1;
  pthread_t thread2;
  int retcode;

  retcode = pthread_create(&thread1, NULL, thread_run, "in child thread1");
  handle_thread_error(retcode);
  retcode = pthread_create(&thread2, NULL, thread_run, "in child thread2");
  handle_thread_error(retcode);

  print_info("in parent");

  retcode = pthread_join( thread1, NULL);
  handle_thread_error(retcode);

  printf("done\n");
  exit(0);
}
