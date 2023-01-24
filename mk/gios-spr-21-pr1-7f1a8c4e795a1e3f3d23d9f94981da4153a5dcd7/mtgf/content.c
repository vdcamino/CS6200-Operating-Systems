#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_KEYLEN 256

typedef struct{
	int fildes;
	char key[MAX_KEYLEN];
} item_t;

static int nitems;
static item_t *items;

static int _itemcmp(const void *a, const void *b){
	return strcmp(((item_t*) a)->key,((item_t*) b)->key);
}

int content_init(const char *filename){
	FILE *filelist;
	int capacity = 16;
	char *path, *ptr;

	if( NULL == (filelist = fopen(filename, "r"))){
		fprintf(stderr, "Unable to open file in content_init.\n");
		exit(EXIT_FAILURE);
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
			exit(EXIT_FAILURE);
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

unsigned long int content_delay = 0;

int content_get(const char *key){
	int lo = 0;
	int hi = nitems - 1;
	int mid, cmp;

	if (content_delay > 0) {
		usleep(content_delay);
	}

	while (lo <= hi) {
		// Key is in items[lo..hi] or not present.
		mid = lo + (hi - lo) / 2;
		cmp = strcmp(key,items[mid].key);
		if ( cmp < 0) hi = mid - 1;
		else if (cmp > 0) lo = mid + 1;
		else{
			return items[mid].fildes;
		} 
	}
	return -1;
}

void content_destroy(){
	int i;
	for(i = 0; i < nitems; i++)
		close(items[i].fildes);
	
	free(items);
}
