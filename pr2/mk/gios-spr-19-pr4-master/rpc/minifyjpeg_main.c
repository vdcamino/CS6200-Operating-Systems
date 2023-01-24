#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <rpc/rpc.h>
#include <string.h>
#include <limits.h> // for PATH_MAX

#include "steque.h"

typedef struct{
  char inputfilename[256];
  char outputfilename[256];
} request_t;

static pthread_mutex_t mutex;
static steque_t queue;
static char workload_dirpath[PATH_MAX];

extern CLIENT* get_minify_client(char *server);
extern int minify_via_rpc(CLIENT* cl, void* src_val, size_t src_len, size_t *dst_len, void** dst_val);

static void workload_filepath(char* filename, char* direction, char* path) {

  int char_len = direction == NULL ?
    snprintf(path, PATH_MAX, "%s/%s", workload_dirpath, filename) :
    snprintf(path, PATH_MAX, "%s/%s/%s", workload_dirpath, direction, filename);

  if (char_len < 0) {
    fprintf(stderr, "Error creating %s path for %s\n", direction, filename);
  }

}

static void load_requests(char *workload_path){
  FILE *file_handle;
  char filepath[PATH_MAX];
  request_t *req;

  workload_filepath(workload_path, NULL, filepath);

  file_handle = fopen(filepath, "r");
  if (file_handle == NULL) {
    fprintf(stderr,"cannot open workload file %s\n", filepath);
    exit(EXIT_FAILURE);
  }

  steque_init(&queue);
  req = (request_t*) malloc(sizeof(request_t));
  while (fscanf(file_handle, "%s %s", req->inputfilename, req->outputfilename) != EOF){
    steque_enqueue(&queue, req);
    req = (request_t*) malloc(sizeof(request_t));
  }
  free(req);

  fclose(file_handle);
}

static void* read_from_file(char* filename, size_t *len){
  FILE *infile;
  char *buffer;
  char filepath[PATH_MAX];
  off_t file_len;

  workload_filepath(filename, "input", filepath);

  /* open an existing file for reading */
  infile = fopen(filepath, "r");

  /* quit if the file does not exist */
  if(infile == NULL){
    fprintf(stderr, "Unable to open the file %s", filepath);
    exit(EXIT_FAILURE);
  }

  /* Get the number of bytes */
  fseek(infile, 0L, SEEK_END);
  file_len = ftell(infile);

  /* reset the file position indicator to
  the beginning of the file */
  fseek(infile, 0L, SEEK_SET);

  /* grab sufficient memory for the
  buffer to hold the text */
  buffer = (char*)malloc(file_len);

  /* memory error */
  if(buffer == NULL){
    fprintf(stderr, "Failed to allocate memory for %s\n", filename);
    exit(EXIT_FAILURE);
  }

  /* copy all the text into the buffer */
  fread(buffer, sizeof(char), file_len, infile);
  fclose(infile);

  *len = file_len;
  return buffer;
}

static int write_to_file(char* filename, void *data, ssize_t len){
  FILE *outfile;
  char filepath[PATH_MAX];

  workload_filepath(filename, "output", filepath);

  /* open an existing file for reading */
  outfile = fopen(filepath, "w");

  /* quit if the file does not exist */
  if(outfile == NULL)
    return 1;

  fwrite((char*) data, 1, len, outfile);
  fclose(outfile);

  return 0;
}

static void *worker_main(void *arg){
  request_t *request;
  void *src_val, *dst_val= NULL;
  size_t src_len, dst_len;
  CLIENT *cl = (CLIENT*) arg;
  int *code = calloc(1, sizeof(int));

  pthread_mutex_lock(&mutex);
  while(!steque_isempty(&queue)){
    request = steque_pop(&queue);
    pthread_mutex_unlock(&mutex);

    src_val = read_from_file(request->inputfilename, &src_len);

    *code = minify_via_rpc(cl, src_val, src_len, &dst_len, &dst_val);

    if(*code == 0)
        write_to_file(request->outputfilename, dst_val, dst_len);

    free(src_val);
    free(dst_val);
    free(request);

    pthread_mutex_lock(&mutex);
  }
  pthread_mutex_unlock(&mutex);

  return code;
}

#define USAGE                                                                 \
"usage:\n"                                                                    \
"  minifyjpeg_main [options]\n"                                               \
"options:\n"                                                                  \
"  -s [server_addr]    Server address (Default: localhost)\n"                 \
"  -w [workload]       Workload file (Default: workload.txt)\n"               \
"  -d [workload_dir]   Workload directory (Default: ../workload)\n"           \
"  -t [nthreads]       Number of threads (Default: 1)\n"                      \
"  -h                  Show this help message\n"

static void Usage() {
  fprintf(stderr, "%s\n", USAGE);
}

static struct option gLongOptions[] = {
  {"server",        required_argument,      NULL,           's'},
  {"workload",      required_argument,      NULL,           'w'},
  {"workload_dir",  required_argument,      NULL,           'd'},
  {"nthreads",      required_argument,      NULL,           't'},
  {"help",          no_argument,            NULL,           'h'},
  {NULL,            0,                      NULL,             0}
};


int main(int argc, char* argv[]){
  char *workload = "workload.txt";
  char *workload_dir = "../workload";
  char* server = "localhost";
  int i, nthreads = 1;
  int option_char = 0;
  pthread_t *pthreads;
  CLIENT **clients;
  int code, *result;

  // Parse and set command line arguments
  while ((option_char = getopt_long(argc, argv, "s:w:d:t:h", gLongOptions, NULL)) != -1){
    switch (option_char) {
      case 's': // server
        server = optarg;
        break;
      case 'w': // workload
        workload = optarg;
        break;
      case 'd': // workload directory
        workload_dir = optarg;
        break;
      case 't': // number of threads
        nthreads = atoi(optarg);
        break;
      case 'h': // help
        Usage();
        exit(0);
        break;
      default:
        Usage();
        exit(1);
    }
  }

  strcpy(workload_dirpath, workload_dir);
  pthread_mutex_init(&mutex, NULL);

  //Load up the workfile into queue
  load_requests(workload);

  pthreads = (pthread_t*) malloc(nthreads * sizeof(pthread_t));
  clients = (CLIENT**) malloc(nthreads * sizeof(CLIENT*));
  for(i = 0; i < nthreads; i++){
    clients[i] = get_minify_client(server);
    pthread_create(&pthreads[i], NULL, worker_main, clients[i]);
  }

  for(i = 0; i < nthreads; i++){
    pthread_join(pthreads[i],(void**) &result);
    clnt_destroy(clients[i]);
    code = *result;
    free(result);
    // If any of the threads returns a value greater than 0, we ignore the
    // return values for the remaining and report this error.
    if (code != 0) {
        i += 1;
        while(i < nthreads) {
            pthread_join(pthreads[i], (void**) &result);
            clnt_destroy(clients[i]);
            free(result);
            i++;
        }
    }

  }

  free(pthreads);
  free(clients);

  return code;
}
