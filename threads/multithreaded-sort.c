/* (C) IT Sky Consulting GmbH 2014
 * http://www.it-sky-consulting.com/
 * Author: Karl Brodowsky
 * Date: 2014-02-27
 * License: GPL v2 (See https://de.wikipedia.org/wiki/GNU_General_Public_License )
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include <itskylib.h>
#include <hsort.h>

#define SIZE 1024
#define THREAD_COUNT 10

typedef char *char_ptr;

struct thread_arg {
  struct string_array arr;
  int thread_idx;
};

enum sort_type { HEAP_SORT, QUICK_SORT };

pthread_barrier_t start_barrier;
pthread_barrier_t barrier;

pthread_mutex_t output_mutex;

int thread_count;

pthread_t *thread;
struct thread_arg *segments;

enum sort_type selected_sort_type;

int compare_full(const void *left, const void *right, void *ignored) {
  return strcmp(*(const char_ptr *) left, *(const char_ptr *) right);
}

void *thread_run(void *ptr) {
  int retcode;
  struct thread_arg *arg = (struct thread_arg *) ptr;
  char **strings = arg->arr.strings;
  int len = arg->arr.len;
  int idx = arg->thread_idx;
  /* pthread_t id = pthread_self(); */
  /* int tid = (int) (id % 0x7fffffff); */

  /* pthread_mutex_lock(&output_mutex); */
  /* printf("before:------------------------------------------------------------\n"); */
  /* for (int i = 0; i < len; i++) { */
  /*   printf("[%9d/%4d] %4d: \"%s\"\n", tid, idx, i, strings[i]); */
  /* } */
  /* printf("/before:------------------------------------------------------------\n"); */
  /* pthread_mutex_unlock(&output_mutex); */

  switch (selected_sort_type) {
  case HEAP_SORT:
    hsort_r(strings, len, sizeof(char_ptr), compare_full, (void *) NULL);
    break;
  case QUICK_SORT:
    qsort_r(strings, len, sizeof(char_ptr), compare_full, (void *) NULL);
    break;
  default:
    /* should *never* happen: */
    handle_error_myerrno(-1, -1, "wrong sort_type", PROCESS_EXIT);
  }

  /* pthread_mutex_lock(&output_mutex); */
  /* printf("after:------------------------------------------------------------\n"); */
  /* for (int i = 0; i < len; i++) { */
  /*   printf("[%9d/%4d] %4d: \"%s\"\n", tid, idx, i, strings[i]); */
  /* } */
  /* printf("/after:------------------------------------------------------------\n"); */
  /* pthread_mutex_unlock(&output_mutex); */

  for (int step = 1; step < thread_count; step *= 2) {
    retcode = pthread_barrier_wait(& barrier);
    if (retcode == PTHREAD_BARRIER_SERIAL_THREAD) {
      pthread_mutex_lock(&output_mutex);
      /* printf("thread %ld (%d) is PTHREAD_BARRIER_SERIAL_THREAD=%d\n", (long) id, tid, retcode); */
      pthread_mutex_unlock(&output_mutex);
    } else {
      handle_thread_error(retcode, "pthread_barrier_wait", THREAD_EXIT);
    }

    if (idx % (2*step) == 0) {
      int other_idx = idx + step;
      if (other_idx < thread_count) {
        int i = 0;
        int j = 0;
        int k = 0;
        int m = segments[idx].arr.len;
        int n = segments[other_idx].arr.len;
        int total_len = m + n;
        char_ptr *strings = malloc(total_len * sizeof(char_ptr));
        char_ptr *left = segments[idx].arr.strings;
        char_ptr *right = segments[other_idx].arr.strings;
        while (i+j < total_len) {
          if (i >= m) {
            while (j < n) {
              strings[k++] = right[j++];
            }
          } else if (j >= n) {
            while (i < m) {
              strings[k++] = left[i++];
            }
          } else {
            if (strcmp(left[i], right[j]) <= 0) {
              strings[k++] = left[i++];
            } else {
              strings[k++] = right[j++];
            }
          }
        }
        segments[idx].arr.len = total_len;
        segments[idx].arr.strings = strings;
        segments[other_idx].arr.len = 0;
        segments[other_idx].arr.strings = NULL;
      }
    }
  }
  return (void *) NULL;
}

void usage(char *argv0, char *msg) {
  printf("%s\n\n", msg);
  printf("Usage:\n\n");
  printf("%s -h number\n\tsorts stdin using heapsort in n threads.\n\n", argv0);
  printf("%s -q number\n\tsorts stdin using quicksort in n threads.\n\n", argv0);
  exit(1);
}

int main(int argc, char *argv[]) {
  int retcode;

  char *argv0 = argv[0];
  if (argc != 3) {
    usage(argv0, "wrong number of arguments");
  }

  /* TODO consider using getopt instead!! */
  char *argv_opt = argv[1];
  if (strlen(argv_opt) != 2 || argv_opt[0] != '-') {
      usage(argv0, "wrong option");
  }
  char opt_char = argv_opt[1];
  switch (opt_char) {
  case 'h' :
    selected_sort_type = HEAP_SORT;
    break;
  case 'q' :
    selected_sort_type = QUICK_SORT;
    break;
  default:
    usage(argv0, "wrong option: only -q and -h supported");
    break;
  }
  
  sscanf(argv[2], "%d", &thread_count);
  if (thread_count < 1 || thread_count > 1024) {
    printf("running with %d threads\n", thread_count);
    usage(argv[0], "wrong number of threads");
  }


  struct string_array content = read_to_array(STDIN_FILENO);
  int len_per_thread = content.len / thread_count;

  thread = (pthread_t *) malloc(thread_count * sizeof(pthread_t));
  segments = (struct thread_arg *) malloc(thread_count * sizeof(struct thread_arg));

  retcode = pthread_barrier_init(&barrier, NULL, thread_count);
  handle_thread_error(retcode, "pthread_barrier_init", PROCESS_EXIT);
  retcode = pthread_mutex_init(&output_mutex, NULL);
  handle_thread_error(retcode, "pthread_mutex_init", PROCESS_EXIT);

  int rest_len = content.len;
  char **ptr = content.strings;

  for (int i = 0; i < thread_count; i++) {
    segments[i].thread_idx = i;
    segments[i].arr.strings = ptr;
    if (i == thread_count-1) {
      segments[i].arr.len     = rest_len;
    } else {
      segments[i].arr.len = len_per_thread;
    }
    ptr += len_per_thread;
    rest_len -= len_per_thread;

    /* pthread_mutex_lock(&output_mutex); */
    /* printf("main: starting thread %d\n", i); */
    /* pthread_mutex_unlock(&output_mutex); */

    retcode = pthread_create(&(thread[i]), NULL, thread_run, &(segments[i]));
    handle_thread_error(retcode, "pthread_create", PROCESS_EXIT);

    /* pthread_mutex_lock(&output_mutex); */
    /* printf("main: started %d\n", i); */
    /* pthread_mutex_unlock(&output_mutex); */
  }

  for (int i = 0; i < thread_count; i++) {
    /* printf("main: joining thread %d\n", i); */
    retcode = pthread_join(thread[i], NULL);
    handle_thread_error(retcode, "pthread_join", PROCESS_EXIT);
    /* printf("main: joined thread %d\n", i); */
  }
  char_ptr *strings = segments[0].arr.strings;
  int len = segments[0].arr.len;
  for (int i = 0; i < len; i++) {
    /* printf("%5d \"%s\"\n", i, strings[i]); */
    printf("%s\n", strings[i]);
  }
  pthread_barrier_destroy(&barrier);
  // printf("DONE\n");
  exit(0);
}