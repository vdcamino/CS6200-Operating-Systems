#include "gfserver.h"
#include "cache-student.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "shm_channel.h"

#define BUFSIZE (630)

ssize_t handle_with_cache(gfcontext_t *ctx, const char *path, void *arg)
{
	///////////////// Read SHM segment ID and segment size off the message queue
	int msqid;
	int return_value;
	struct msgbuf *mybuf = malloc(sizeof(struct msgbuf));
	key_t mqkey = ftok("/etc", 'Q');
	if ((msqid = msgget(mqkey, 0660)) == -1)
	{
		// failed to create message queue
		exit(1);
	};
	if ((msgrcv(msqid, mybuf, sizeof(struct msgbuf), SHM_MESSAGE_TYPE, 0)) == -1)
	{
		// failed to receive message from MQ
		exit(1);
	};

	int shmid = mybuf->message_text.shmid;
	int segsize = mybuf->message_text.segsize;

	///////////////// Attach process to the shared memory and init semaphores
	struct shm_struct *shm_struct;
	shm_struct = (struct shm_struct *)shmat(shmid, (void *)0, 0);
	sem_init(&shm_struct->writer_semaphore, 1, 1);
	sem_init(&shm_struct->reader_semaphore, 1, 0);
	sem_init(&shm_struct->header_semaphore, 1, 0);

	///////////////// Add SHM_ID, Segsize and Path to the CTX queue for cache server
	int msqid_ctx;
	struct msgbuf_ctx *mybuf_ctx = malloc(sizeof(struct msgbuf_ctx));
	mybuf_ctx->mtype = CTX_MESSAGE_TYPE;
	memset(mybuf_ctx->message_text_ctx.path, 0, 150);
	strncpy(mybuf_ctx->message_text_ctx.path, path, 150);
	mybuf_ctx->message_text_ctx.segsize = segsize;
	mybuf_ctx->message_text_ctx.shmid = shmid;
	key_t mqkey_ctx = ftok("/etc", 'C');
	if ((msqid_ctx = msgget(mqkey_ctx, 0660)) == -1)
	{
		// failed to create message queue
		exit(1);
	};
	if ((msgsnd(msqid_ctx, mybuf_ctx, sizeof(struct msgbuf_ctx), CTX_MESSAGE_TYPE)) == -1)
	{
		// failed to send message to MQ
		exit(1);
	};

	///////////////// Wait on the header semaphore until cache server does its work
	sem_wait(&shm_struct->header_semaphore);
	if (shm_struct->file_size <= 0)
	{
		// FILE NOT FOUND
		return_value = gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
	}
	else if (shm_struct->file_size > 0)
	{
		//OK
		return_value = gfs_sendheader(ctx, GF_OK, shm_struct->file_size);

		///////////////// Receive data from shared memory with Semaphore synch
		int bytes_sent = 0;
		int file_size;
		int buffer_sent;

		file_size = shm_struct->file_size;

		while (bytes_sent < file_size)
		{
			// Wait on reader semaphore until cache finishes writing
			sem_wait(&shm_struct->reader_semaphore);

			shm_struct->data_chunk = (void *)shm_struct + sizeof(struct shm_struct);
			buffer_sent = gfs_send(ctx, shm_struct->data_chunk, shm_struct->chunk_size);
			bytes_sent += buffer_sent;

			// Post on writer semaphore for cache to write
			sem_post(&shm_struct->writer_semaphore);
		}
	}
	else
	{
		// ERROR
		return_value = SERVER_FAILURE;
	}

	msgsnd(msqid, mybuf, sizeof(struct msgbuf), SHM_MESSAGE_TYPE);
	free(mybuf);
	free(mybuf_ctx);
	sem_destroy(&shm_struct->writer_semaphore);
	sem_destroy(&shm_struct->reader_semaphore);
	sem_destroy(&shm_struct->header_semaphore);
	shmdt((void *)shm_struct);
	return (size_t)return_value;
}