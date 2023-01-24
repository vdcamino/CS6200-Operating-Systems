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
#include <curl/curl.h>

#include "gfserver.h"
#include "cache-student.h"

#define MAX_KEYLEN 1024

#if !defined(CACHE_FAILURE)
#define CACHE_FAILURE (-1)
#endif // CACHE_FAILURE

typedef struct{
	int fildes;
	char key[MAX_KEYLEN];
} item_t;

static int nitems;
static item_t *items;

static int _itemcmp(const void *a, const void *b){
	return strcmp(((item_t*) a)->key,((item_t*) b)->key);
}

extern unsigned long int cache_delay;


int simplecache_init(char *filename){
	FILE *filelist;
	int capacity = 16;
	char *path, *ptr;

	if( NULL == (filelist = fopen(filename, "r"))){
		fprintf(stderr, "Unable to open file in simplecache_init.\n");
		exit(CACHE_FAILURE);
	}

	items = (item_t*) malloc(capacity * sizeof(item_t));
	nitems = 0;
	while(fgets(items[nitems].key, MAX_KEYLEN, filelist)){
		/*Taking out EOL character*/
		items[nitems].key[strlen(items[nitems].key)-1] = '\0';

		/* Using space delimiter to sep key and path*/
		ptr = items[nitems].key;
		strsep(&ptr, " \t"); 		/* The key is first */
		path = strsep(&ptr, " \t"); /* The path second */

		if( 0 > (items[nitems].fildes = open(path, O_RDONLY))){
			fprintf(stderr, "Unable to open file %s.\n", path);
			exit(CACHE_FAILURE);
		}
		nitems++;

		if(nitems == capacity){
			capacity *= 2;
			items = realloc(items, capacity * sizeof(item_t));
		}

	}

	fclose(filelist);

	qsort(items, nitems, sizeof(item_t), _itemcmp);

	return EXIT_SUCCESS;
}

int simplecache_get(char *key){
	int lo = 0;
	int hi = nitems - 1;
	int mid, cmp;

	if (cache_delay > 0) {
		usleep(cache_delay);
	}

	while (lo <= hi) {
		// Key is in items[lo..hi] or not present.
		mid = lo + (hi - lo) / 2;
		cmp = strcmp(key,items[mid].key);
		if ( cmp < 0) hi = mid - 1;
		else if (cmp > 0) lo = mid + 1;
		else{
			lseek(items[mid].fildes, 0, SEEK_SET);
			return items[mid].fildes;
		} 
	}
	return -1;
}

void simplecache_destroy(){
	int i;
	for(i = 0; i < nitems; i++)
		close(items[i].fildes);
	
	free(items);
}
