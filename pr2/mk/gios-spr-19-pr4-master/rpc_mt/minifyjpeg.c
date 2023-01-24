#include "magickminify.h"
#include "minifyjpeg.h"
#include <stdio.h>
#include <rpc/rpc.h>

/* Implement the server-side functions here */
bool_t minifyjpeg_proc_1_svc(img_in jpeg_in, img_out *jpeg_out, struct svc_req *sreq){

  void* src = jpeg_in.input.input_val;
  ssize_t src_len = jpeg_in.input.input_len;
  ssize_t dst_len;
  bool_t retval;

  magickminify_init();
  // get minified image and image size
  jpeg_out->output.output_val = magickminify(src, src_len, &dst_len);
  jpeg_out->output.output_len = dst_len;
  
  retval = 1;
  return retval;
}


int minifyjpeg_prog_1_freeresult (SVCXPRT *svc_xpr, xdrproc_t proc, caddr_t caddr){
  magickminify_cleanup();
  (void) xdr_free(proc, caddr);
  return 1;
}
