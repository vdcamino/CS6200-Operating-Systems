#ifndef __WORKLOAD_H__
#define __WORKLOAD_H__

#define WORKLOAD_SEQ 0
#define WORKLOAD_RND 1

/* 
 * Opens the file associated with the input argument
 * and reads in a list of paths to request. 
 */
int workload_init(char *workload_path);

/*
 * Sets the mode.  If WORKLOAD_SEQ, then workload getpath will
 * return the paths in sequence.  If WORKLOAD_RND, then
 * the paths will be chosen uniformly at random with replacement.
 */
int workload_set_mode(int mode);

/*
 * Returns the number of unique paths in the workload
 */
unsigned short int workload_num_unique_paths();

/*
 * Returns a path from the workload.  Whether this is
 * done sequentially, randomly or by some other method
 * is not specified.
 */
char* workload_get_path();

/*
 * Cleans up the workload package.
 */
void workload_destroy(void);

#endif
