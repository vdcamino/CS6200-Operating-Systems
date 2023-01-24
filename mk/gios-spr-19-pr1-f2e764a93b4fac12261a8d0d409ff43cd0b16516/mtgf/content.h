#ifndef __CONTENT_H__
#define __CONTENT_H__

/* 
 * Initializes the content library given the information from
 * the provided file.  Each row of the file is assumed
 * to contain a key and a file path separated by a space.
 * See content.txt for an example.	
 *
 * Subsequent calls to content_get with a key value
 * as an argument will return the file descriptor for the 
 * given file path.
 */
int content_init(const char *filename);

/* 
 * Returns the file descriptor associated with the input key.
 * Returns -1 if the the key is not found
 */
int content_get(const char *key);

/* 
 * Frees all memory and closes all file descriptors
 * associated with the cache.
 */
void content_destroy();

#endif