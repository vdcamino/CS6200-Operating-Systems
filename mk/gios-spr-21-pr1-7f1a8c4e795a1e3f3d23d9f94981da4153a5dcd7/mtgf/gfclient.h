#ifndef __GFCLIENT_H__
#define __GFCLIENT_H__

/*
 * gfclient is a client library for transferring files using the GETFILE
 * protocol.  The interface is inspired by libcurl's "easy" interface. 
 */

#include <stdlib.h>

typedef enum{
  GF_OK,
  GF_FILE_NOT_FOUND,
  GF_ERROR,
  GF_INVALID
} gfstatus_t;

/*struct for a getfile request*/
typedef struct gfcrequest_t gfcrequest_t;

/*
 * Returns the string associated with the input status
 */
const char *gfc_strstatus(gfstatus_t status);

/* 
 * This function must be the first one called as part of 
 * request.  It returns a gfcrequest_t handle which should be
 * passed into all subsequent library calls pertaining to
 * this requeest.
 */
gfcrequest_t *gfc_create();

/*
 * Sets the server to which the request will be sent.
 */
void gfc_set_server(gfcrequest_t **gfr, const char* server);

/*
 * Sets the path of the file that will be requested.
 */
void gfc_set_path(gfcrequest_t **gfr, const char* path);

/*
 * Sets the port over which the request will be made.
 */
void gfc_set_port(gfcrequest_t **gfr, unsigned short port);

/*
 * Sets the callback for received header.  The registered callback
 * will receive a pointer the header of the response, the length 
 * of the header response as it's second argument (don't assume that
 * this is null-terminated), and the pointer registered with 
 * gfc_set_headerarg (or NULL if not specified) as the third argument.
 *
 * You may assume that the callback will only be called once and will
 * contain the full header.
 */
void gfc_set_headerfunc(gfcrequest_t **gfr, void (*headerfunc)(void*, size_t, void *));

/*
 * Sets the third argument for all calls to the registered header callback.
 */
void gfc_set_headerarg(gfcrequest_t **gfr, void *headerarg);


/*
 * Sets the callback for received chunks of the body.  The registered 
 * callback will receive a pointer the chunk, the length of the chunk
 * as it's second argument (don't assume that this is null-terminated),
 * and the pointer registered with gfc_set_writearg (or NULL if not 
 * specified) as the third argument.
 *
 * The callback may be called multiple times in a single request.  The 
 * gfclient library does not store the entire contents of the requested file
 * but rather calls this callback each time that it receives a chunk of data
 * from the server.
 */
void gfc_set_writefunc(gfcrequest_t **gfr, void (*writefunc)(void*, size_t, void *));


/*
 * Sets the third argument for all calls to the registered write callback.
 */
void gfc_set_writearg(gfcrequest_t **gfr, void *writearg);

/*
 * Performs the transfer as described in the options.  Returns a value of 0
 * if the communication is successful, including the case where the server
 * returns a response with a FILE_NOT_FOUND or ERROR response.  If the 
 * communication is not successful (e.g. the connection is closed before
 * transfer is complete or an invalid header is returned), then a negative 
 * integer will be returned.
 */
int gfc_perform(gfcrequest_t **gfr);

/*
 * Returns the status of the response.
 */
gfstatus_t gfc_get_status(gfcrequest_t **gfr);

/*
 * Returns the length of the file as indicated by the response header.
 * Value is not specified if the response status is not OK.
 */
size_t gfc_get_filelen(gfcrequest_t **gfr);

/*
 * Returns actual number of bytes received before the connection is closed.
 * This may be distinct from the result of gfc_get_filelen when the response 
 * status is OK but the connection is reset before the transfer is completed.
 */
size_t gfc_get_bytesreceived(gfcrequest_t **gfr);

/*
 * Frees memory associated with the request.  
 */
void gfc_cleanup(gfcrequest_t **gfr);


/*
 * Sets up any global data structures needed for the library.
 * Warning: this function may not be thread-safe.
 */
void gfc_global_init();


/*
 * Cleans up any global data structures needed for the library.
 * Warning: this function may not be thread-safe.
 */
void gfc_global_cleanup();


#endif
