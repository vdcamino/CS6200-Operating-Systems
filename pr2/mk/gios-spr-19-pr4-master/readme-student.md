# GIOS-SPR-19-PR4 Project README file

This is **Yiran Xu (email: yxu629@gatech.edu)**'s CS-6200-O01 Graduate Introduction to Operating Systems Project 4 Readme file.

## Project Description
In this project, I worked with Remote Procedure Calls, specifically with Sun RPC. The RPC server will accept a jpeg image as input, downsample it to a lower resolution, and return the resulting image.

###Part 1 - Building a Single-Threaded RPC Server

The part1 project is build a single-threaded RPC server. The **`rpcgen`** tool can generate much of the C code. I implemented the functionality by modifying the following three relevant files.

- **minifyjpeg.x** I defined the RPC/XDR interface and minifyjpeg program here. The program number is set to **0x31234567**. In order to support images of arbitrary size and not worry about implementing packet ordering, I chose **TCP** as the transport protocol. Also, defined the input image structure and the output image structure by using **opaque** type.
```
struct img_in {
 	opaque input<>; // input image
 };

struct img_out {
	opaque output<>; // output image
};
```
After modified **minifyjpeg.x**, I run **`rpcgen`** tool to generate much of the C code.
```
rpcgen -MN minifyjpeg.x
```
- **minify_via_rpc.c** I create the client handle and call minifyjpeg procedure. After I created the client handle, I called **`clnt_control`** to change the timeout value to **5** seconds.
```
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
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    clnt_control(cl, CLSET_TIMEOUT, (char *)&tv);
    return cl
```
In **`minify_via_rpc.c`**, I called minifyjpeg procedure by passing client handle `cl`, input image data structure `img_in src` and output image data structure `img_out dst` into **`minifyjpeg_proc_1`**. Herein, `img_in src` includes the `src_len` and `src_val`. The minified image will be stored in `dst` with `dst_len` and `dst_val`.
```
  retval = minifyjpeg_proc_1(src, &dst, cl);
  if(retval!=RPC_SUCCESS){
    clnt_perror(cl, "RPC failed");
    return retval;
  }
```
- **minifyjpeg.c** I implemented the server side function. Specifically, I initialzied and called `magickminify` method to reduce the size of the input image, and return the small size output image. I also cleaned up the ImageMagick library and freed the XDR routine here.
```
  magickminify_init();
  // get minified image and image size
  jpeg_out->output.output_val = magickminify(src, src_len, &dst_len);
  jpeg_out->output.output_len = dst_len;
```
```
  magickminify_cleanup(); // cleaned up the ImageMagick libary
  (void) xdr_free(proc, caddr); // free the XDR routine
  return 1;
```
###Part 2 - Building a Multi-Threaded RPC Server

The objective of this second part of the project is to modify the server-side file **`rpc-mt/minifyjpeg_svc.c`** to make the server multithreaded. 

I used the part 1 codes **minifyjpeg.x**, **`minify_via_rpc.c`**, and **minifyjpeg.c**. Then I run **`rpcgen`** tool to generate much of the C code.
```
rpcgen -MN minifyjpeg.x
```
My approaches to work on the Part 2:
1. Initialize pthread, mutex and queue in **`main`**.
2. Parse the command line arguments for number of threads and create worker threads in **`main`**..
3. Call **`svctcp_create`**, **`svc_register`** and **`svc_run`** in **`main`**..
4. Call **`svc_getargs`** in the  function registered with **`svc_register`** and enqueue all the information needed for worker thread. (Boss thread)
5. Pop all the information needed and proceed the image by calling **`minifyjpeg_proc_1`** and **`svc_sendreply`**. (Worker thread)
6. Cleanup and free memory.

I implemented the functionality by modifying the following relevant file.
- **minifyjpeg_svc.c** In the **`main`** method, I modified to accept a command-line parameter (-t) to indicate the number of threads to be created.
```
	int nthreads = 1; //1
 	int option_char = 0;
	// Parse and set command line arguments
  	while ((option_char = getopt_long(argc, argv, "t:hr", gLongOptions, NULL)) != -1) {
    	switch (option_char) {
      		case 't': // nthreads
        		nthreads = atoi(optarg);
        		break;
      		case 'h': // help
        		fprintf(stdout, "%s", USAGE);
       	 		exit(0);
        		break;
      		default:
        		fprintf(stderr, "%s", USAGE);
        		exit(1);
    }
  }

	/* not useful, but it ensures the initial code builds without warnings */
  	if (nthreads < 1) {
   	 	nthreads = 1;
  	}
```
Then I initialized mutex, condition variable, and queue. I also called **`pthread_create`** to create worker threads **`threadFunc`**. Further, I modified **`minifyjpeg_prog_1`** to a boss thread. I defined a global struct `steque_node` to store the information I want to put int the queue `req_queue`.
```
typedef struct{
  	struct svc_req *rqstp;
  	SVCXPRT *transp;
	xdrproc_t _xdr_argument; //input image
  	xdrproc_t _xdr_result; // output image
  	img_in argument;
	img_out result;
}steque_node;
``` 

- In the boss thread **`minifyjpeg_prog_1`**, I called  **`svc_getargs`** function to  copy global data from the rpc library into memory. Then together with other information, when the boss thread acquires the mutex lock, I can put these information into the queue `req_queue`. These information can include the server handle, request from the client, input image structure, etc. After that, signal the worker thread to proceed request from the client.
```
    steque_node* node;
    node = (steque_node*)malloc(sizeof(steque_node));
    node->rqstp = rqstp;
    node->transp = transp;
    node->_xdr_argument = _xdr_argument;
		node->_xdr_result = _xdr_result;
    node->argument = argument;
		if(pthread_mutex_lock(&mutex)!=0){
			printf("\n mutex lock has failed\n");
			exit(1);
		}
		steque_enqueue(req_queue,node);

		if(pthread_mutex_unlock(&mutex)!=0){
			printf("\n mutex unlock has failed\n");
			exit(1);
		}
		/* Signal the worker thread*/
		pthread_cond_signal(&mutex_cv);
```
- In the worker thread **`threadFunc`**, when worker can first acquires the mutex lock, the worker can pop a request object `req_obj` from the queue `steque_node`. Then the worker called can proceed the request by calling the **`minifyjpeg_proc_1`** and **`svc_sendreply`**, which is to call `magickminify` libraries to reduce the size of the input image and send the minified image back to the client. After that, the worker will free the arguments and do some memory cleanup.

## Known Bugs/Issues/Limitations
- In part 1, for the timeout problem, I need to cast the timeout value to `char`.
```
clnt_control(cl, CLSET_TIMEOUT, (char *)&tv);
```
- In part 2, I deleted the original `union` type input image structure and output image structure (generated by **`rpcgen`**). 
```
	union {
		input minifyjpeg_proc_1_arg;
	} argument;
	union {
		output minifyjpeg_proc_1_res;
	} result;
```
Instead, I defined them directly like this:
```
img_in argument;
img_out result;
```

- In part2, when I declared a `img_out result` in the worker thread function, my code passed all the Bonnie tests even I have never used it in the worker thread function. While when I remove this line of code, or I replaced the `req_obj->result` with the `result`, I failed **"Tests multithreaded server with multiple clients"** with an **AssertionError: Your rpc server is not indicating the requested number of threads. 9 threads detected; 8 threads requested**. I am not sure why it happened. Actually in the worker thread function, I have never used the declared `result` in the function.
```
img_out result;

retval = (bool_t) (*local)((char *)&req_obj->argument, (void *)&req_obj->result, req_obj->rqstp);
if (retval > 0 && !svc_sendreply(req_obj->transp, (xdrproc_t) req_obj->_xdr_result, (char *)&req_obj->result)) {
    	svcerr_systemerr (req_obj->transp);
}
```

## Personal thoughts and suggestions
- I think it very usefule to use gdb to track the program working flow so that I can understand how the server and the client interact with each other. Further, I can know what kind of methods can be put into the worker thread and what kind of methods should be kept in the boss thread.

- Avoid double freeing memory, make sure free the memory only once.

- When you malloc'ed a memory, always remember to free it after you finished using it.

## References

- [Sun RPC Example](http://web.cs.wpi.edu/~rek/DCS/D04/SunRPC.html)
- [Sun RPC Documentation](https://docs.oracle.com/cd/E19683-01/816-1435/index.html) 
- [Timeout] (https://docs.oracle.com/cd/E19253-01/816-1435/rpcgenpguide-21/index.html)
- [RFC 1057 - RPC: Remote Procedure Calls](https://www.rfc-editor.org/info/rfc1057) (this is the official RPC specification)
- [From C to RPC Tutorial](http://www.cs.cf.ac.uk/Dave/C/node34.html)
- [RPC MAN Pages](http://man7.org/linux/man-pages/man3/rpc.3.html)
- [XDR MAN Pages](http://man7.org/linux/man-pages/man3/xdr.3.html)