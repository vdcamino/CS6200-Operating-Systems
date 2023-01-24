# Multithreading

## Foreword

In this project, you will design and implement a multi-threaded server that
serves static files based on the GetFile protocol, which is a simple HTTP-like
protocol. Alongside the server, you will
also create a multi-threaded client that acts as a load generator for the
server. Both the server and client will be written in C and be based on a
sound, scalable design.

## Setup

Please follow the instructions given in the [environment](https://github.gatech.edu/gios-spr-21/environment)
repository for setting up your development/test environment.

You can clone the code in this repository with the command

```console
$ git clone https://github.gatech.edu/gios-spr-21/pr1.git
```

## Submission Instructions

We use an auto-grading system based upon Gradescope.  You can find Gradescope
from Canvas, and that will make the submission engine available to you.  You will need
to manually upload your code to Gradescope, where it will be executed
in its own environment (a Docker container) and the results will be
gathered up and returned to you.


### Feedback from Gradescope

Note that there is a strict limit to the amount of feedback that we can
return to you.  Thus:

* We do not return feedback for tests that pass
* We truncate any feedback past the limit (approximately 10KB)
* We do not return detailed feedback for tests that run after the 10KB limit is reached.

For this project, you may submit your code as many times as you wish for the
Warmup exercises.  For Parts 1 & 2 (each of which consists of a client and a
server part) you will have a limit of no more than 50 submissions between now
and the final due date; you may use them all within one day, or you may use
one per day - the choice is yours.

We strongly encourage you to think about testing your own code.  Test driven
development is a standard technique for software, including systems software.
The auto-grader is a _grader_ and not a test suite.

Note: your project readme file (10% of your grade for Project 1) is submitted on
**Canvas** in PDF format.

### We do not use the "activate" feature

Gradescope includes an "activate" feature that purportedly tells you which version
of your code you want us to grade; **we do not use this feature**.  We always grade
your last submission prior to the deadline.  This is due to the nature of our
assignments, which can have race conditions; allowing you to run the same code 10
times and pick the one that worked is antithetical to our learning objectives.
Thus, we use your last submission.  Plan accordingly.

## README

Throughout the project, we encourage you to keep notes on what you have done,
how you have approached each part of the project and what resources you used in
preparing your work.  We have provided you with a prototype file,
**readme-student-template.md** that you may use as the basis for this report. You
are not required to use this format.

## Warm-up: Sockets

In this project, your client and server will communicate via sockets.  To help
you familiarize yourself with C’s standard socket API, you will complete a few
warm-up exercises

### Echo Client-Server

Most socket tutorials start with a client-server pair where the server simply
echoes (that is, repeats back) the message that  the client sends.  Sometimes
referred to as the EchoClient and the EchoServer.   In the first part of this
assignment, you will turn in a pair of programs that perform these roles.

For this first part of the assignment, we will make some simplifying
assumptions.  You may assume that neither the message to the server nor the
response will be longer than 15 bytes.  Thus, you may statically allocate
memory (e.g. `char buffer[16];`) and then pass this memory address into send or
recv.  Because these messages are so short, you may assume that the full
message is sent or received in every send or recv call.  (If you may like, you
may detect when this is not the case by looking at the return value and simply
exiting if the full message is not sent or received).  Neither your server nor
your client should assume that the string response will be null-terminated,
however.

It is important that the only output (either to stdout or stderr) be the response
from the server.  Any missing or additional output causes the test to fail.

Your echoserver should not terminate after sending its first response; rather,
it should prepare to serve another request.  You may consult the references suggested
in this document or others on the internet.

Your programs should support both IPv4 and IPv6. We have tests in subsequent parts to test support 
for IPv4 and IPv6.

Once you have completed your programs inside the echo directory, you upload it
to Gradescope (which you can find from Canvas).

Once submitted, Gradescope will test your echoclient.c code with its own reference
implementation of echoserver.c and your echoserver.c code with its own
reference implementation of echoclient.c.  It will then return some feedback to
help you refine your code if it does not meet requirements.

 **Note** For this part of the assignment, you may submit your code as many times as you would like.
 Your grade is based upon the grade of your last submission.

### Transferring a File

Because the network interface is a shared resource, the OS does not guarantee
that calls to `send` will copy the contents of the full range of memory that is
specified to the socket.  Instead, (for non-blocking sockets) the OS may copy
as much of the memory range as it can fit in the network buffer and then lets
the caller know how much it copied via the return value.


```c
int socket;
char *buffer
size_t length;
ssize_t sent;

/* Preparing message */
…

/*Sending message */
sent = send(socket, buffer, length, 0);

assert( sent == length); //WRONG!
```

You should *not* assume that sent has the same value as length.  The function
`send` may need to be called again (with different parameters) to ensure that
the full message is sent.

Similarly, for `recv`, for TCP sockets, the OS does not wait for the range of
memory specified to be completely filled with data from the network before the
call returns.  Instead, only a portion of the originally sent data may be
delivered from the network to the memory address specified in a `recv`
operation.  The OS will let the caller know how much data it wrote via the
return value of the `recv` call.

In several ways, this behavior is desirable, not only from the perspective of
the operating system which must ensure that resources are properly shared, but
also from the user’s perspective.  For instance, the user may have no need to
store in memory all of the data to be transferred.  If the goal is to simply
save data received over the network to disk, then this can be done a chunk at a
time without having to store the whole file in memory.

In fact, it is this strategy that you should employ as the second part of the
warm-up.  You are to create a client, which simply connects to a server--no
message need be sent-- and saves the received data to a file on disk.

Beware of file permissions.  Make sure to give yourself read and write access,
e.g. `S_IRUSR | S_IWUSR` as the last parameter to open.

A sender of data may also want to use this approach of sending it a chunk at a
time to conserve memory.  Employ this strategy as you create a server that
simply begins transferring data from a file over the network once it
establishes a connection and closes the socket when finished.

You may consult the references suggested in this document or others on the
internet. Starter code is provided inside of the transfer directory.  Once you
have completed your programs inside this directory, you may upload it to Gradescope.

Your server should not stop after a single transfer, but should continue accepting
and transferring files to other clients.  Your client should terminate after
receiving the full contents.  Unlike a normal web server (which leaves the socket
open until the client closes it) your server should close the socket so the client
knows when the full contents of the file have been transmitted.  Note that the
length of the file being sent is otherwise not known to the client; adding it to
your client and server means it will not pass the grader because it does not implement
the same protocol.

## Part 1: Implementing the Getfile Protocol

Getfile is a simple (HTTP-like) protocol used to transfer a file from one computer to
another that we made up for this project.  A typical successful transfer is
illustrated below.

![Getfile Transfer Figure](illustrations/gftransfer.png)

The general form of a request is

```
<scheme> <method> <path>\r\n\r\n
```

Note:

* The scheme is always GETFILE.
* The method is always GET.
* The path must always start with a ‘/’.

The general form of a response is

```
<scheme> <status> <length>\r\n\r\n<content>
```

Note:

* The scheme is always GETFILE.
* The status must be in the set {‘OK’, ‘FILE_NOT_FOUND’, ‘ERROR’, 'INVALID'}.
* INVALID is the appropriate status when the header is invalid. This includes a malformed header as well an incomplete header due to communication issues.
* FILE_NOT_FOUND is the appropriate response whenever the client has made an
  error in his request.  ERROR is reserved for when the server is responsible
  for something bad happening.
* No content may be sent if the status is FILE_NOT_FOUND or ERROR.
* When the status is OK, the length should be a number expressed in ASCII (what
  sprintf will give you). The length parameter should be omitted for statuses
  of FILE_NOT_FOUND or ERROR.
* The character '\r' is a single byte, which when used in a C literal string is translated by the compiler
  to be a carriage return.
* The character '\n' is a single byte, which when used in a C literal string is translated by the compiler
  to be a newline character.
* The sequence ‘\r\n\r\n’ marks the end of the header. All remaining bytes are
  the files contents. This is four bytes of data.
* The space between the \<scheme> and the \<method> and the space between the \<method> and the \<path> are required. There should not be a space between the \<path> and '\r\n\r\n'.
* The space between the \<scheme> and the \<status> and the space between the \<status> and the \<length> are required. There should not be a space betwen the \<length> and '\r\n\r\n'.  There should not be a space between \<status> and '\r\n\r\n'.
* The length may be up to a 64 bit decimal unsigned value (that is, the value will be less than 18446744073709551616) though we do not require that you support this amount (nor will we test against a 16EB-1 byte file).

**Note** the strings are not terminated with a null character.  The buffers that you receive are not C strings.  Just like HTTP, this is
a language neutral format.  Do not assume that anything you receive is null ('\0') terminated.  You should avoid using string functions unless you have ensured that the data has been null-terminated.

### Instructions

For Part 1 of the assignment you will implement a Getfile client library and a
Getfile server library.  The API and the descriptions of the individual
functions can be found in the gfclient.h and gfserver.h header files.  Your
task is to implement these interfaces in the gfclient.c and gfserver.c files
respectively.  To help illustrate how the API is intended to be used and to
help you test your code we have provided the other files necessary to build a
simple Getfile server and workload generating client inside of the gflib
directory.  It contains the files

* Makefile - (do not modify) used for compiling the project components
* content.[ch] - (do not modify) a library that abstracts away the task of
  fetching content from disk.
* content.txt - (modify to help test) a data file for the content library
* gfclient.c - (modify and submit) implementation of the gfclient interface.
* gfclient.h - (do not modify) header file for the gfclient library
* gfclient-student.h - (modify and submit) header file for students to modify - submitted for client only
* gfclient_download.c - (modify to help test) the main file for the client
  workload generator.  Illustrates use of gfclient library.
* gfserver.c - (modify and submit) implementation of the gfserver interface.
* gfserver.h - (do not modify) header file for the gfserver library.
* gfserver-student.h - (modify and submit) header file for students to modify - submitted for server only
* gfserver_main.c (modify to help test) the main file for the Getfile server.
  Illustrates the use of the gfserver library.
* gf-student.h (modify and submit) header file for students to modify - submitted for both client and server
* handler.o - contains an implementation of the handler callback that is
  registered with the gfserver library.
* workload.[ch] - (do not modify) a library used by workload generator
* workload.txt - (modify to help test) a data file indicating what paths should
  be requested and where the results should be stored.

### gfclient

The client-side interface documented in gfclient.h is inspired by [libcurl’s
“easy” interface](http://curl.haxx.se/libcurl/c/libcurl-easy.html).  To help
illustrate how the API is intended to be used and to help you test your code,
we have provided the file gfclient_download.c. For those not familiar with
function pointers in C and callbacks, the interface can be confusing.  The key
idea is that before the user tells the gfclient library to perform a request,
he should register one function to be called on the header
(`gfc_set_headerfunc`) and register one function to be called for every “chunk”
of the body (`gfc_set_writefunc`). That is, one call to the latter function
will be made for every `recv()` call made by the gfclient library.  (It so
happens that none of the test code actually will use the header function, but
please implement your library to support it anyway.)

The user of the gfclient library may want his callback functions to have access
to data besides the information about the request provided by the gfclient
library.  For example, the write callback may need to know the file to which it
should write the data.  Thus, the user can register an argument that should be
passed to his callback on every call.  She or he does this with the
`gfc_set_headerarg` and `gfc_set_writearg` functions.  (Note that the user may
have multiple requests being performed at once so a global variable is not
suitable here.)

Note that the `gfcrequest_t` is used as an [opaque
pointer](https://en.wikipedia.org/wiki/Opaque_pointer), meaning that it should
be defined within the gfclient.c file and no user code (e.g.
gfclient_download.c) should ever do anything with the object besides pass it to
functions in the gfclient library.

### gfserver

The server side interface documented in gfserver.h is inspired by python’s
built-in httpserver.  The existing code in the file gfserver_main.c illustrates
this usage.  Again, for those not familiar with function pointers in C and
callbacks, the interface can be confusing.  The key idea is before starting
up the server, the user registers a handler callback with gfserver library
which controls how the request will be handled.  The handler callback should
not contain any of the socket-level code.  Instead, it should use the
gfs_sendheader, gfs_send, and potentially the gfs_abort functions provided
by the gfserver library.  Naturally, the gfserver library needs to know which
request the handler is working on.  All of this information is captured in the
opaque pointer passed into the handler, which the handler must then pass back
to the gfserver library when making these calls.

### Submission

You should submit your code via Gradescope.

## Part 2: Implementing a Multithreaded Getfile Server

*Note: For both the client and the server, your code should follow a
boss-worker thread pattern. This will be graded. Also keep in mind this is a multi-threading
exercise. You are **NOT** supposed to use `fork()` to spawn child processes, but instead
you should be using the **pthreads** library to create/manage threads. You will have
points removed if you don't follow this instruction.*

In Part 1, the Getfile server can only handle a single request at a time.  To
overcome this limitation, you will make your Getfile server multi-threaded by
implementing your own version of the handler in handler.c and updating gfserver_main.c
as needed.  The main, i.e. boss, thread will continue listening to the socket and
accepting new connection requests. New connections will, however, be fully handled by
worker threads. The pool of worker threads should be initialized to the number of
threads specified as a command line argument.  You may need to add additional
initialization or worker functions in handler.c.  Use extern keyword to make
functions from handler.c available to gfserver_main.c.  For instance, the main
file will need a way to set the number of threads.

Similarly on the client side, the Getfile workload generator can only download
a single file at a time.  In general for clients, when server latencies are
large, this is a major disadvantage.  You will make your client multithreaded
by modifying gfclient_download.c.  This will also make testing your getfile
server easier.

In the client, a pool of worker threads should be initialized based on number
of client worker threads specified with a command line argument. Once the
worker threads have been started, the boss should enqueue the specified
number of requests (from the command line argument) to them.  They should
continue to run.  Once the boss confirms all requests have been completed,
it should terminate the worker threads and exit.  We will be looking for
your code to use a work queue (from `steque.[ch]`), at least one
mutex (`pthread_mutex_t`), and at least one condition variable (`pthread_cond_t`)
to coordinate these activities. Gradescope will confirm that your implementation
meets these requirements.

The folder mtgf includes both source and object files. The object
files gfclient.o and gfserver.o may be used in-place of your own
implementations.  Source code is not provided because these files implement
the protocol for Part 1.  Note: these are the binary files used by Gradescope.
They are known *not to work* on Ubuntu versions other than 18.04.  They are
64 bit binaries (only).  If you are working on other platforms, you should
be able to use your protocol implementation files from Part 1.

* Makefile - (do not modify) used for compiling the project components
* content.[ch] - (do not modify) a library that abstracts away the task of
  fetching content from disk.
* content.txt - (modify to help test) a data file for the content library
* gfclient.o - (do not modify) implementation of the gfclient interface.
* gfclient.h - (do not modify) header file for the gfclient library
* gfclient-student.h - (modify and submit) header file for students to modify - submitted for client only
* gfclient_download.c - (modify and submit) the main file for the client
  workload generator.  Illustrates use of gfclient library.
* gfserver.o - (do not modify) implementation of the gfserver interface.
* gfserver.h - (do not modify) header file for the gfserver library.
* gfserver-student.h - (modify and submit) header file for students to modify - submitted for server only
* gfserver_main.c (modify and submit) the main file for the Getfile server.
  Illustrates the use of the gfserver library.
* gf-student.h (modify and submit) header file for students to modify - submitted for both client and server
* handler.c - (modify and submit) contains an implementation of the handler callback that is
  registered with the gfserver library.
* steque.[ch] - (do not modify) a library you must use for implementing your boss/worker queue.
* workload.[ch] - (do not modify) a library used by workload generator
* workload.txt - (modify to help test) a data file indicating what paths should
  be requested and where the results should be stored.

### Submission

Your code should be submitted via Gradescope.

## Readme

Throughout your development you should be keeping notes about what you are doing
for your development.  We encourage you to submit your readme file as your project
develops, so that it can serve as a log of your work.

This file will be submitted **via Canvas** as a PDF.  You may call it whatever you want,
and you may use any tool you desire, so long as it is a compliant PDF - and for us,
compliant means "we can open it using Acrobat Reader".

To obtain all possible points, we want you to demonstrate your understanding of what you
have done.  **We are not looking for a diary, we are looking for a clear explanation of
what you did and why you did it.**  This is your grader's opportunity to award you points
for _understanding_ the project, even if you struggle with the implementation, as
these are manually graded.

You should upload your README file via Canvas (Project 1 README).

**NOTE:** This is your opportunity to clearly document your reference materials.

Finally, we encourage you to make concrete suggestions on how you would improve this project. This is not _required_ for the project, but is an opportunity for you to earn extra credit points.  This could also include sample code, suggested tests
(we really like examples!), ways to improve the documentation, etc.

## References

### Sample Source Code

* [Server and Client
  Example Code](https://github.com/zx1986/xSinppet/tree/master/unix-socket-practice) - note the link
  referenced from the readme does not work, but the sample code is valid.
* Another Tutorial with Source
  Code for [server](http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/server.c) and [client](http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/client.c)

### General References

* [POSIX Threads (PThreads)](https://computing.llnl.gov/tutorials/pthreads/)
* [Linux Sockets Tutorial](http://www.linuxhowtos.org/C_C++/socket.htm)
* [Practical TCP/IP Sockets in C](http://cs.baylor.edu/~donahoo/practical/CSockets/)
* [Guide to Network Programming](http://beej.us/guide/bgnet/)

## Helpful Hints

* [High-level code design of multi-threaded GetFile server and
  client](https://docs.google.com/drawings/d/1a2LPUBv9a3GvrrGzoDu2EY4779-tJzMJ7Sz2ArcxWFU/edit?usp=sharing)
* Return code (exit status) 11 of client or server means it received a SIGSEGV
  (segmentation fault). You can get a comprehensive list of signals with `kill
  -l`.
* In the warm-up part, the server should be running forever. It should not
  terminate after serving one request.
* Use the  class VM to test and debug your code. The autograder is not a test harness.

### Rubric

* Warm-up (20 points)
  * Echo Client-Server (10 points)
  * Transfer (10 points)

Your submission should compile and build without any errors on gradescope. 

For echo client-server: Your client program will be tested to make sure it 
correctly prints the message that was received from the server and your 
server will be tested to make sure it correctly echoes the message sent 
by the client. Note your submission may fail tests if it prints or echoes 
any additional characters (including formatting options such as newline 
characters.)

For transfer: Your client should successfully save the file sent from the 
sever on the disk and the server should accurately send the file to the client. 
File size and contents on the client should match the file size and contents on  the server side.

* Part 1: Getfile Libraries (30 points)
  * Client (15 points)
  * Server(15 points)

Full credit requires: code compiles successfully, does not crash, files fully
transmitted, basic safety checks (file present, end of file, etc.), and proper
use of sockets - including the ability to restart your server implementation
using the same port. Note that the automated tests will test _some_ of these
automatically, but graders may execute additional tests of these requirements.

Our tests include malformed requests such as invalid scheme (eg: PULLFILE), 
invalid method (eg: FETCH) and invalid path (eg: foo); requests for files 
that are not on the server as well as scenarios where the server is unable to 
process the client request. Likewise, our tests include valid requests that 
may result in a single message from the server (ie a single message with both 
the response header and data in the same message) or multiple messages from 
the server (response header or data sent in multiple messages, message size 
may or may not vary). 

In addition to the above tests , we also run some sanity tests to ensure your 
submission compiles & builds without any errors on gradescope and proper 
initialization & cleanup is performed (eg: no memory leaks and no invalid 
memory accesses). 

* Part 2: Multithreading (40 points)
  * Client (20 points)
  * Server(20 points)

Full credit requires: code compiles successfully, does not crash, does not
deadlock, number of threads can be varied, concurrent execution of threads,
files fully transmitted, correct use of locks to access shared state, no race
conditions, etc. Note that the automated tests will test _some_ of these
automatically, but graders may execute additional tests of these requirements.

In addition to the sanity tests discussed in part 1, our tests will validate 
if your submission supports multiple file downloads of varying file sizes 
concurrently. Your implementation employs boss worker model to distribute work 
and uses appropriate synchronization mechanisms to protect shared data.

* README (10 points + 5 point extra credit opportunity)
  * Clearly demonstrates your understanding of what you did and why - we
    want to see your _design_ and your explanation of the choices that you
    made _and why_ you made those choices. (4 points)
  * A description of the flow of control for your code; we strongly suggest
    that you use graphics here, but a thorough textual explanation is sufficient.
    (2 points)
  * A brief explanation of how you tested your code. (2 points)
  * References any external materials that you consulted during your
    development process (2 points)
  * Suggestions on how you would improve the documentation, sample code,
    testing, or other aspects of the project (up to 5 points *extra credit*
    available for noteworthy suggestions here, e.g., actual descriptions of
    how you would change things, sample code, code for tests, etc.) We do not
    give extra credit for simply _reporting_ an issue - we're looking for
    actionable suggestions on how to improve things.

**Partial Credit** - some tests are run multiple times and credit is awarded
for successul runs. This is the only partial credit we award.  If your code
did not work but you can explain why fully in your README file, we may give
you full marks in the README.

In cases where we identify an issue with the grader that impacts your project,
we may manually adjust the final scores.  For example, if we find that your
code fails to implement required functionality but the grader failed to
detect this, we may reduce the score.  Similarly, if we find an issue with the
grader that failed to credit you properly, we may adjust the score manually.
Normally this is done by running a modified version of the grader that is
corrected.  **This is unusual, but has happened in the past.**.

**Ideal README** - the ideal README file is one that you could have picked up
at the start of the project and used it to guide your development.  It would
explain the choices you considered and _why_ you chose specific approaches
to the problem.  It won't complain about the code, the process, how little you
know about C, or other things unrelated to the project.  It won't discuss the
_code_; it will discuss the techniques and algorithms.

Thus, a README file that explains the issues you faced, or provides an English
description of your code, will receive at most half-points.  A README file
that explains what choices you faced, why you chose to implement it how you did,
how you implemented it, and what your conclusions are about that choice can
be awarded up to full marks.

## Questions

For all questions, please use the class Piazza forum or Slack Workspace so
that TAs and other students can assist you.
