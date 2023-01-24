/*
 * Define your service specification in this file and run rpcgen -MN minifyjpeg.x
 */
 /*
 * Data structure for input image
 */
struct img_in {
 	opaque input<>;
 };

/*
 * The result of a MINIFY_PROG operation:
 *
 */
struct img_out {
	opaque output<>;
};

/*
 * The minifyjpeg program definition
 */
 program MINIFYJPEG_PROG {
     version MINIFYJPEG_VERS {
         img_out MINIFYJPEG_PROC(img_in) = 1;    /* procedure number = 1 */
     } = 1;                          /* version number = 1 */
 } = 0x31234567;                     /* program number = 0x31234567 */
