#ifndef __MAGICKMINIFY_H__
#define __MAGICKMINIFY_H__

#include <stdlib.h>

/* 
 * Initializes the ImageMagick libary. 
 */
void magickminify_init();

/* 
 * Downsamples the jpeg file stored in the memory
 * starting at the address src and continuing for 
 * src_len bytes.  New memory is allocated to store
 * the result.  It starts at the address returned
 * and continues for the number of bytes stored in the 
 * return parameter dst_len.
 */

void* magickminify(void* src, ssize_t src_len, ssize_t* dst_len);

/* 
 * Cleans up the ImageMagick libary. 
 */
void magickminify_cleanup();

#endif
