#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/signal.h>
#include <printf.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <math.h>

#include "gfserver.h"
#include "cache-student.h"
#include "shm_channel.h"

/* note that the -n and -z parameters are NOT used for Part 1 */
/* they are only used for Part 2 */
#define USAGE                                                                    \
  "usage:\n"                                                                     \
  "  webproxy [options]\n"                                                       \
  "options:\n"                                                                   \
  "  -n [segment_count]  Number of segments to use (Default: 6)\n"               \
  "  -p [listen_port]    Listen port (Default: 30605)\n"                         \
  "  -t [thread_count]   Num worker threads (Default: 12, Range: 1-520)\n"       \
  "  -s [server]         The server to connect to (Default: GitHub test data)\n" \
  "  -z [segment_size]   The segment size (in bytes, Default: 6200).\n"          \
  "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"segment-count", required_argument, NULL, 'n'},
    {"port", required_argument, NULL, 'p'},
    {"thread-count", required_argument, NULL, 't'},
    {"server", required_argument, NULL, 's'},
    {"segment-size", required_argument, NULL, 'z'},
    {"help", no_argument, NULL, 'h'},
    {"hidden", no_argument, NULL, 'i'}, /* server side */
    {NULL, 0, NULL, 0}};

extern ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void *arg);

static gfserver_t gfs;
static int numsegments;
static size_t segmentsize;

static void _sig_handler(int signo)
{
  if (signo == SIGTERM || signo == SIGINT)
  {
    gfserver_stop(&gfs);
    // Deleting Shared Memory Message queue resources
    key_t mqkey;
    int msqid;
    mqkey = ftok("/etc", 'Q');
    msqid = msgget(mqkey, IPC_CREAT | 0660);
    msgctl(msqid, IPC_RMID, 0);
    // Deleting CTX Request Message queue resources
    mqkey = ftok("/etc", 'C');
    msqid = msgget(mqkey, IPC_CREAT | 0660);
    msgctl(msqid, IPC_RMID, 0);
    // Deleting Shared Memory resources
    for (int i = 0; i < numsegments; i++)
    {
      key_t shmkey;
      int shmid;
      shmkey = ftok("/etc", i);
      shmid = shmget(shmkey, segmentsize, IPC_CREAT | 0660);
      shmctl(shmid, IPC_RMID, 0);
    }
    exit(signo);
  }
}

/* Main ========================================================= */
int main(int argc, char **argv)
{
  int i;
  int option_char = 0;
  unsigned short port = 30605;
  unsigned short nworkerthreads = 12;
  unsigned int nsegments = 6;
  size_t segsize = 6200;
  char *server = "https://raw.githubusercontent.com/gt-cs6200/image_data";

  /* disable buffering on stdout so it prints immediately */
  setbuf(stdout, NULL);

  if (signal(SIGINT, _sig_handler) == SIG_ERR)
  {
    fprintf(stderr, "Can't catch SIGINT...exiting.\n");
    exit(SERVER_FAILURE);
  }

  if (signal(SIGTERM, _sig_handler) == SIG_ERR)
  {
    fprintf(stderr, "Can't catch SIGTERM...exiting.\n");
    exit(SERVER_FAILURE);
  }

  /* Parse and set command line arguments */
  while ((option_char = getopt_long(argc, argv, "s:qt:hn:xp:z:l", gLongOptions, NULL)) != -1)
  {
    switch (option_char)
    {
    default:
      fprintf(stderr, "%s", USAGE);
      exit(__LINE__);
    case 'p': // listen-port
      port = atoi(optarg);
      break;
    case 'n': // segment count
      nsegments = atoi(optarg);
      break;
    case 's': // file-path
      server = optarg;
      break;
    case 'z': // segment size
      segsize = atoi(optarg);
      break;
    case 't': // thread-count
      nworkerthreads = atoi(optarg);
      break;
    case 'i':
    case 'x':
    case 'l':
      break;
    case 'h': // help
      fprintf(stdout, "%s", USAGE);
      exit(0);
      break;
    }
  }

  if (segsize < 128)
  {
    fprintf(stderr, "Invalid segment size\n");
    exit(__LINE__);
  }

  if (!server)
  {
    fprintf(stderr, "Invalid (null) server name\n");
    exit(__LINE__);
  }

  if (port < 1024)
  {
    fprintf(stderr, "Invalid port number\n");
    exit(__LINE__);
  }

  if (nsegments < 1)
  {
    fprintf(stderr, "Must have a positive number of segments\n");
    exit(__LINE__);
  }

  if ((nworkerthreads < 1) || (nworkerthreads > 520))
  {
    fprintf(stderr, "Invalid number of worker threads\n");
    exit(__LINE__);
  }

  //////////////// Setting static variables for cleaning up on Signal Handler
  numsegments = nsegments;
  segmentsize = segsize;

  //////////////// Initializing shared memory Message Queue ////////////////
  key_t mqkey;
  int msqid;
  mqkey = ftok("/etc", 'Q');
  if ((msqid = msgget(mqkey, IPC_CREAT | 0660)) == -1)
  {
    // failed to create message queue
    exit(1);
  };

  //////////////// Initializing CTX request Message Queue ////////////////
  key_t mqkey_ctx;
  int msqid_ctx;
  mqkey_ctx = ftok("/etc", 'C');
  if ((msqid_ctx = msgget(mqkey_ctx, IPC_CREAT | 0660)) == -1)
  {
    // failed to create message queue
    exit(1);
  };

  ///////////////// Initialize shared memory set-up here ////////////////
  for (int i = 0; i < nsegments; i++)
  {
    key_t shmkey;
    int shmid;
    struct msgbuf mybuf;

    shmkey = ftok("/etc", i);
    if ((shmid = shmget(shmkey, segsize, IPC_CREAT | 0660)) == -1)
    {
      // failed to create shared memory segment
      exit(1);
    };

    mybuf.mtype = SHM_MESSAGE_TYPE;
    mybuf.message_text.segsize = (int)segsize;
    mybuf.message_text.shmid = shmid;

    if ((msgsnd(msqid, &mybuf, sizeof(struct message_text), SHM_MESSAGE_TYPE)) == -1)
    {
      // failed to send message to MQ
      exit(1);
    };
  }

  // Initialize server structure here
  gfserver_init(&gfs, nworkerthreads);

  // Set server options here
  gfserver_setopt(&gfs, GFS_MAXNPENDING, 121);
  gfserver_setopt(&gfs, GFS_WORKER_FUNC, handle_with_cache);
  gfserver_setopt(&gfs, GFS_PORT, port);

  // Set up arguments for worker here
  for (i = 0; i < nworkerthreads; i++)
  {
    gfserver_setopt(&gfs, GFS_WORKER_ARG, i, NULL);
  }

  // Invoke the framework - this is an infinite loop and shouldn't return
  gfserver_serve(&gfs);

  // not reached
  return -1;
}
