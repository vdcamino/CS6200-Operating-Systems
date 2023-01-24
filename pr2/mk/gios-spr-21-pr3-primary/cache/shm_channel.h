//In case you want to implement the shared memory IPC as a library...
#include <semaphore.h>

#define SHM_MESSAGE_TYPE 2
#define CTX_MESSAGE_TYPE 3

// Message buffers for Shared Memory segments
struct message_text
{
    int shmid;
    int segsize;
};
struct msgbuf
{
    long mtype;
    struct message_text message_text;
};

// Message buffers for CTX request to be handled by cache
struct message_text_ctx
{
    int shmid;
    int segsize;
    char path[150];
};
struct msgbuf_ctx
{
    long mtype;
    struct message_text_ctx message_text_ctx;
};

// Struct for the shared memory data
struct shm_struct
{
    int chunk_size;
    int file_size;
    int total_bytes_sent;
    sem_t header_semaphore;
    sem_t reader_semaphore;
    sem_t writer_semaphore;
    void *data_chunk;
};