#include "minifyjpeg_clnt.c"
#include "minifyjpeg_xdr.c"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/rpc.h>

CLIENT* get_minify_client(char *server){
    CLIENT *cl;
    struct timeval tv;
    /* Create client handler */
    if ((cl = clnt_create(server, MINIFYJPEG_PROG, MINIFYJPEG_VERS, "tcp")) == NULL) {
    /*
     * can't establish connection with server
     */
     clnt_pcreateerror(server);
     exit(1);
    }
    // reset a new TIMEOUT
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    clnt_control(cl, CLSET_TIMEOUT, (char *)&tv);
    return cl;
}

/*
The size of the minified file is not known before the call to the library that minimizes it,
therefore this function should allocate the right amount of memory to hold the minimized file
and return it in the last parameter to it
*/
int minify_via_rpc(CLIENT *cl, void* src_val, size_t src_len, size_t *dst_len, void **dst_val){

	/*Your code goes here */
  img_in src;
  img_out dst;
  dst.output.output_val = (char *)malloc(src_len*sizeof(char));
  src.input.input_val = src_val;
  src.input.input_len = src_len;

  enum clnt_stat retval;
  // call minifyjpeg procedure
  retval = minifyjpeg_proc_1(src, &dst, cl);
  if(retval!=RPC_SUCCESS){
    clnt_perror(cl, "RPC failed");
    return retval;
  }
  *dst_len = dst.output.output_len;
  *dst_val = dst.output.output_val;
  // printf("Actual output file size is: %zu\n", *dst_len);
  return retval;
}
