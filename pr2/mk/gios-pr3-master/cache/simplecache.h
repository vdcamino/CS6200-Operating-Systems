#ifndef _SIMPLECACHE_H_
#define _SIMPLECACHE_H_

/* 
 * Initializes the input cache given the information from
 * the provided file.  Each row of the file is assumed
 * to contain a key and a file path separated by a space.
 * Subsequent calls to simplecache_get with a key value
 * as an argument will return the file descriptor for the 
 * given file path.
 */
int simplecache_init(char *filename);

/* 
 * Returns the file descriptor associated with the input key.
 */
int simplecache_get(char *key);

/* 
 * Frees all memory and closes all file descriptors
 * associated with the cache.
 */
void simplecache_destroy();

#endif