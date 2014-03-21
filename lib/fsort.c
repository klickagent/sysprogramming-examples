/* (C) IT Sky Consulting GmbH 2014
 * http://www.it-sky-consulting.com/
 * Author: Karl Brodowsky
 * Date: 2014-02-27
 * License: GPL v2 (See https://de.wikipedia.org/wiki/GNU_General_Public_License )
 */

#include <stdio.h>
#include <string.h>
#include <alloca.h>
#include <stdlib.h>

#include <fsort.h>
#include <itskylib.h>

#define POINTER(base, idx, size) ((base) + (size) * (idx))


void fsort_r(void *base,
             size_t nmemb,
             size_t size,
             compare_fun3 compare,
             void *argc,
             metric_fun2 metric,
             void *argm) {
  // fprintf(stderr, "fsort_r\n");
  fsort_f(base, nmemb, size, 0.42, compare, argc, metric, argm);
}

size_t calculate_k(double factor, double value, size_t lsize) {
  /* think of rounding errors, so add a little bit to the double */
  double prod = factor * value + 1e-9;
  if (prod < 0) {
    prod = 0;
  }
  if (prod > lsize -1) {
    prod = lsize - 1;
  }
  size_t result = (size_t) prod;
  return result;
}

void fsort_f(void *base,
             size_t nmemb,
             size_t size,
             double factor,
             compare_fun3 compare,
             void *argc,
             metric_fun2 metric,
             void *argm) {
  // fprintf(stderr,"starting\n");
  if (nmemb <= 1) {
    /* nothing to sort */
    return;
  }

  // fprintf(stderr, "non-trivial\n");
  /* preparation: form classes */
  // fprintf(stderr, "nmemb=%ld factor=%lf calculating lsize\n", nmemb, factor);
  size_t lsize = calculate_k(nmemb, factor, nmemb) + 1;
  if (lsize < 2) {
    lsize = 2;
  }
  // fprintf(stderr, "lsize=%ld\n", lsize);
  size_t *l = (size_t *) malloc(lsize * sizeof(size_t));
  for (size_t k = 0; k < lsize; k++) {
    l[k] = 0;
  }
  //size_t idx_min = 0;
  void *amin = POINTER(base, 0, size);
  size_t idx_max = 0;
  void *amax = POINTER(base, idx_max, size);
  for (size_t i = 0; i < nmemb; i++) {
    if (compare(POINTER(base, i, size), amin, argc) < 0) {
      // idx_min = i;
      amin = POINTER(base, i, size);
    }
    if (compare(POINTER(base, i, size), amax, argc) > 0) {
      idx_max = i;
      amax = POINTER(base, i, size);
    }
  }
  if (compare(amin, amax, argc) == 0) {
    /* min and max are the same --> already sorted */
    free(l);
    return;
  }
  double amin_metric = metric(amin, argm);
  double amax_metric = metric(amax, argm);
  double step = (lsize - 1)/(amax_metric - amin_metric);
  /* size_t k_min = calculate_k(step, 0, lsize); */
  /* size_t k_max = calculate_k(step, amax_metric - amin_metric, lsize); */
  /* fprintf(stderr, "nmemb=%ld lsize=%ld step=%lf k_min=%ld k_max=%ld\n", nmemb, lsize, step, k_min, k_max); */
  // fprintf(stderr, "nmemb=%ld lsize=%ld step=%lf idx_min=%ld idx_max=%ld\n", nmemb, lsize, step, idx_min, idx_max);

  /* count the elements in each of the lsize classes */
  for (size_t i = 0; i < nmemb; i++) {
    double ai_metric = metric(POINTER(base, i, size), argm);
    size_t k = calculate_k(step, ai_metric - amin_metric, lsize);
    l[k]++;
  }

  /* find the start positions for each of the classes */
  size_t *ll = (size_t *) malloc((lsize + 1) * sizeof(size_t));
  ll[0] = 0;
  for (size_t k = 1; k < lsize; k++) {
    // size_t left  = l[k];
    // size_t right = l[k-1];
    l[k] += l[k-1];
    ll[k+1] = l[k];
    // fprintf(stderr, "k=%ld l[k]=%ld=%ld+%ld\n", k, l[k], left, right);
  }
  
  swap_elements(POINTER(base, 0, size), POINTER(base, idx_max, size), size);
  // fprintf(stderr,"prepared\n");

  /* do the permutation */
  size_t nmove = 0;
  size_t j = 0;
  size_t k = lsize - 1;
  while (nmove < nmemb - 1) {
    // fprintf(stderr, "nmove=%ld nmemb=%ld j=%ld k=%ld lsize=%ld step=%lf\n", nmove, nmemb, j, k, lsize, step);
    while (j >= l[k]) {
      // fprintf(stderr, "j>=l[k]: j=%ld k=%ld l[k]=%ld\n", j, k, l[k]);
      j++;
      k = calculate_k(step, metric(POINTER(base, j, size), argm) - amin_metric, lsize);
    }
    /* now: j < l[k] */
    // fprintf(stderr, "looped j>l[k]-1: j=%ld k=%ld l[k]=%ld\n", j, k, l[k]);
    void *flash_ptr = alloca(size);
    /* flash_ptr takes element a[j] such that j > l[k] */
    // fprintf(stderr, "copy a[j=%ld] to flash_ptr\n", j);
    memcpy(flash_ptr, POINTER(base, j, size), size);
    // fprintf(stderr, "before loop j!=l[k]: j=%ld k=%ld l[k]=%ld\n", j, k, l[k]);
    while (j != l[k]) {
      // fprintf(stderr, "loop 1: j=%ld k=%ld l[k]=%ld\n", j, k, l[k]);
      k = calculate_k(step, metric(flash_ptr, argm) - amin_metric, lsize);
      // fprintf(stderr, "loop 2: j=%ld k=%ld l[k]=%ld\n", j, k, l[k]);
      void *alkm_ptr = POINTER(base, l[k]-1, size);
      // fprintf(stderr, "swap flash_ptr and a[l[k=%ld]=%ld]\n", k, l[k]);
      swap_elements(flash_ptr, alkm_ptr, size);
      l[k]--;
      nmove++;
      // fprintf(stderr, "loop 3: j=%ld k=%ld l[k]=%ld (nmove=%ld)\n", j, k, l[k], nmove);
    }
    // fprintf(stderr, "looped j!=l[k]: j=%ld k=%ld l[k]=%ld (nmove=%ld)\n", j, k, l[k], nmove);
  }
  // fprintf(stderr,"permuted\n");
  //   /* straight insertion */
  //   for (size_t ii = nmemb - 2; ii > 0; ii--) {
  //     size_t i = ii-1;
  //     fprintf(stderr, "outer: i=%ld ii=%ld j=%ld nmemb=%ld\n", i, ii, j, nmemb);
  //     void *ai_succ_ptr = POINTER(base, ii, size);
  //     void *ai_ptr = POINTER(base, i, size);
  //     if (compare(ai_succ_ptr, ai_ptr, argc) < 0) {
  //         void *hold_ptr = alloca(size);
  //         memcpy(hold_ptr, ai_ptr, size);
  //         j = i;
  //         void *aj_ptr = POINTER(base, j, size);
  //         while (TRUE) {
  //           fprintf(stderr, "inner: i=%ld ii=%ld j=%ld nmemb=%ld\n", i, ii, j, nmemb);
  //           void *aj_succ_ptr = POINTER(base, j+1, size);
  //           if (compare(aj_succ_ptr, hold_ptr, argc) >= 0) {
  //             break;
  //           }
  //           memcpy(aj_ptr, aj_succ_ptr, size);
  //           j++;
  //           aj_ptr = aj_succ_ptr;
  //         }
  //         fprintf(stderr, "outer done: i=%ld ii=%ld j=%ld nmemb=%ld\n", i, ii, j, nmemb);
  //         memcpy(aj_ptr, hold_ptr, size);
  //     }
  //   }
  //   fprintf(stderr,"inserted\n");
  /* use qsort or hsort for each class */
  for (k = 0; k < lsize; k++) {
    size_t n = ll[k+1] - ll[k];
    if (n > 1) {
      // fprintf(stderr, "k=%ld n=%ld ll[k]=%ld ll[k+1]=%ld\n", k, n, ll[k], ll[k+1]);
      void *basek = POINTER(base, ll[k], size);
      qsort_r(basek, n, size, compare, argc);
    }
  }
  free(l);
  free(ll);
  return;
}
