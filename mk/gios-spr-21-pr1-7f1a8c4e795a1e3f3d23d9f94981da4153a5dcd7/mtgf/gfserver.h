#ifndef __GF_SERVER_H__
#define __GF_SERVER_H__

/*
 * gfserver is a server library for transferring files using the GETFILE
 * protocol.
 */

#define MAX_REQUEST_LEN 128

typedef int gfstatus_t;

#define  GF_OK 200
#define  GF_FILE_NOT_FOUND 400
#define  GF_ERROR 500
#define  GF_INVALID 600

typedef enum {
    gfh_success = 5,
    gfh_failure = 10,
} gfh_error_t;

typedef struct gfserver_t gfserver_t;
typedef struct gfcontext_t gfcontext_t;

/* 
 * This function must be the first one called as part of 
 * setting up a server.  It returns a gfserver_t handle which should be
 * passed into all subsequent library calls of the form gfserver_*.  It
 * is not needed for the gfs_* call which are intended to be called from
 * the handler callback.
 */
gfserver_t *gfserver_create();

/*
 * Sets the port at which the server will listen for connections.
 */
void gfserver_set_port(gfserver_t **gfs, unsigned short port);

/*
 * Sets the maximum number of pending connections which the server
 * will tolerate before rejecting connection requests.
 */
void gfserver_set_maxpending(gfserver_t **gfs, int max_npending);


/*
 * Sets the handler callback, a function that will be called for each each
 * request.  As arguments, this function receives:
 * - a gfcontext_t handle which it must pass into the gfs_* functions that 
 * 	 it calls as it handles the response.
 * - the requested path
 * - the pointer specified in the gfserver_set_handlerarg option.
 * The handler should only return a negative value to signal an error.
 */
void gfserver_set_handler(gfserver_t **gfs, gfh_error_t (*handler)(gfcontext_t **, const char *, void*));

/*
 * Sets the third argument for calls to the handler callback.
 */
void gfserver_set_handlerarg(gfserver_t **gfs, void* arg);

/*
 * Starts the server.  Does not return.
 */
void gfserver_serve(gfserver_t **gfs);

/*
 * Sends to the client the Getfile header containing the appropriate 
 * status and file length for the given inputs.  This function should
 * only be called from within a callback registered gfserver_set_handler.
 */
ssize_t gfs_sendheader(gfcontext_t **ctx, gfstatus_t status, size_t file_len);

/*
 * Sends size bytes starting at the pointer data to the client 
 * This function should only be called from within a callback registered 
 * with gfserver_set_handler.  It returns once the data has been
 * sent.
 */
ssize_t gfs_send(gfcontext_t **ctx, const void *data, size_t size);

/*
 * Aborts the connection to the client associated with the input
 * gfcontext_t.
 */
void gfs_abort(gfcontext_t **ctx);

/*
 * this routine is used to handle the getfile request
 */
gfh_error_t gfs_handler(gfcontext_t **ctx, const char *path, void* arg);

#endif
