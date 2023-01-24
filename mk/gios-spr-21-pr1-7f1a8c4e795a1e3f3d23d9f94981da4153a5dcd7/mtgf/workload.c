#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "workload.h"

static char *gWorkloadPathArray[100];
static unsigned short int gUniqueWorkloadPaths = 0;

static pthread_mutex_t counter_mutex;
static int counter = 0;
static int mode = WORKLOAD_SEQ;

int workload_init(char *workload_path) {
  int i = 0;
  char temp_buf[256];

  FILE *file_handle;

  file_handle = fopen(workload_path, "r");
  if (file_handle == NULL) {
    fprintf(stderr, "cannot open workload file %s", workload_path);
    return EXIT_FAILURE;
  }

  while (fscanf(file_handle, "%s", temp_buf) != EOF)
    gWorkloadPathArray[i++] = strdup(temp_buf);

  gUniqueWorkloadPaths = i;

  pthread_mutex_init(&counter_mutex, NULL);

  fclose(file_handle);

  return EXIT_SUCCESS;
}

unsigned short int workload_num_unique_paths(){
  return gUniqueWorkloadPaths;
}

char* workload_get_path(){
  int entry;

  if(mode == WORKLOAD_RND)
    return gWorkloadPathArray[(int)(1.0 * gUniqueWorkloadPaths * rand()/(RAND_MAX+1.0))];

  entry = __sync_fetch_and_add(&counter, 1);

  return gWorkloadPathArray[entry % gUniqueWorkloadPaths];

}