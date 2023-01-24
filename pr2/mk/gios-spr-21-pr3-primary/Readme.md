# IPC: Inter-Process Communication

## Foreword

This project builds upon Project 1.  In Project 1 we took you through a series of steps towards
building a simplified web server.  In _this_ project you will start with a working getfile
server and augment it:

1. **Part 1** You will convert this implementation of the getfile server to act as a [proxy server](https://en.wikipedia.org/wiki/Proxy_server).
  This server will accept incoming GETFILE requests and translate them into http requests for another server,
  such as one located on the internet.
2. **Part 2** You will implement a simple [cache server](https://whatis.techtarget.com/definition/cache-server) 
   that communicates with the proxy via **shared memory**.
   The design of this cache server is intended to permit its use with **multiple** proxy servers.

We have simplified this project by omitting requirements that are typically present in such systems, the most
notable of which is that **we do not** require you _download and save_ files from the Internet.  The servers
that we have provided to you already implement threading, since you did that in a previous project.

## Setup

Please follow the instructions given in the [environment](https://github.gatech.edu/gios-spr-21/environment)
repository for setting up a viable development/test environment.  We do not require that you use this
environment, so please feel free to work in the environment you find most productive.  The auto-grader
uses an environment that is compatible with what we have described.

You can clone the code in this repository with the command:

```
git clone https://github.gatech.edu/gios-spr-21/pr3.git
```

We encourage you to save your code in a separate repository.  You can make one in the **gios-spr-21**
organization. Regardless of where you place your repository, we expect you to keep it private to
meet the obligations of the honor code.

## Submission Instructions

We are using gradescope as the auto-grading system this semester. 
You can find Gradescope from Canvas, and that will make the submission 
engine available to you.  You will need to manually upload your code to 
Gradescope, where it will be executed in its own environment (a Docker 
container) and the results will be gathered up and returned to you.

Note that there is a strict limit to the amount of feedback that we can
return to you.  Thus:

* We do not return feedback for tests that pass
* We truncate any feedback past the limit (approximately 10KB)
* We do not return detailed feedback for tests that run after the 10KB limit is reached.

We may be adjusting the details of the auto-grader so the availability for all 
sections might be delayed a bit; you may begin working on the projects now and 
submitting them as we open subsequent portions of the auto-grader.

For Parts 1 & 2 you will have a limit of no more than 50 submissions between now
and the final due date; you may use them all within one day, or you may use
one per day - the choice is yours.

We strongly encourage you to think about testing your own code.  Test driven
development is a standard technique for software, including systems software.
The auto-grader is a _grader_ and not a test suite.

Note: your project report (10% of your grade for Project 3) is submitted on
**Canvas** in PDF format. There is no limit to the number of times you may
submit your **readme-student.pdf** file.

The Project 3 grading is more extensive than with Project 1.  Accordingly, you
may be used to fairly quick responses.  You **may** experience much longer
wait times with Project 3.

Once again, we strongly encourage you to think about testing your own code.  Test driven
development is a standard technique for software, including systems software.
The autograder is _not_ your test suite.

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

## Part 1

To convert the getfile server into a proxy, you only need to replace the part
of the code that retrieves the file from disc with code that retrieves it from
the web.  Using the gfserver library provided in the starter code, this is as
easy as registering a callback.  Documentation can be found in the gfserver.h
file.  To implement the callback you should use the [libcurl's “easy” C interface](http://curl.haxx.se/libcurl/c/).

![part1 architecture](docs/part1.png)

**The framework code for part 1 is in the `server` directory.**

This code will handle implementation of the boss-worker multi-threading pattern.  This allows
it to serve multiple connections at once. Note that you don’t have to write your own http 
server.  Workloads are provided for files that live on a [GitHub repository](https://github.com/gt-cs6200/image_data). 
Concatenate https://raw.githubusercontent.com/gt-cs6200/image_data with one of
the paths found in workload.txt to create a valid url. 
**Note: the grading server will not use the same URL.**
Do not encode this URL into your code or it should fail in the auto-grader.

Here is a summary of the relevant files and their roles.  **Note** that files which are
marked "do not modify" are **not** used by the auto-grader for grading.  You can modify them
for debugging purposes, but those changes are not used in the auto-grader.

- **courses** - These are files that you may use during testing

- **gfclient_download** - a binary executable that serves as a workload generator for the proxy.  It downloads the requested files, using the current directory as a prefix for all paths. Note you must specify the correct port to use.

- **gfclient_measure** - a binary executable that serves as a workload generator for the proxy.  It measures the performance of the proxy, writing one entry in the metrics file for each chunk of data received. Note you must specify the correct port to use.

- **gfserver.h** - (not submitted) header file for the library that interacts with the getfile client.

- **gfserver.o** - (not submitted) object file for the library that interacts with the getfile client.

- **gfserver_noasan.o** - (not submitted) a non-address sanitizer version of **gfserver.o**.

- **handle_with_curl.c** - (modify and submit) implement the handle_with_curl function here using the libcurl library.  On a 404 from the webserver, this function should return a header with a Getfile status of `GF_FILE_NOT_FOUND`, as normal web servers map access denied and not found to the same error code to prevent probing security attacks.

- **handle_with_file.c** - (not submitted) illustrates how to use the gfserver library with an example.

- **Makefile** - (not submitted) file used to compile the code.  Run `make` to compile your code.

- **proxy-student.h** - (submitted) you may use this header file to store any content you desire.  If you do not wish to do so,
you can ignore this file.

- **steque.[ch]** - (not submitted) you may find this steque (stack and queue) data useful in implementing the proxy server.

- **webproxy.c** - (modify and submit) this is the main file for the webproxy program.  Only small modifications are necessary.

- **workload.txt** - (not submitted) this file contains a list of files that can be requested of  https://github.com/gt-cs6200/image_data as part of your tests.

Once you have completed your program, you can submit it using Gradescope by copying the files we have indicated should
be submitted.  Any additional files you submit will not be used in grading your submission.  Any files you fail to
upload will not be used in grading your submission (and usually mean your code will not compile).

## Part 2

The objective of this second part of the project is for you to gain experience
with shared memory-based IPC. You will implement a cache process that will run
on the same machine as the proxy and communicate with it via shared memory.

![part2 architecture](docs/part2.png)

For transferring local files efficiently, you are expected to separate the data
channel from the command channel. File content is transferred through the data
channel and transfer command is transferred through the command channel. The
data channel is implemented using shared memory and the command channel is
implemented using other IPC mechanisms, such as message queue.

Because the goal is to give you experience with shared memory IPC and not cache
replacement policy or cache implementations, you will access the contents of
the cache through an API (simplecache.h) that abstracts away most details of
the cache implementation.  Your focus will be on relaying the contents of the
cache to the proxy process by way of shared memory.

You can use either System V or the POSIX API to implement the shared-memory
component and/or other IPC mechanisms (e.g., message queues or semaphores).
Please see the list of reference material.  If it works on the course’s VM,
then it will work in the gradescope test.

**The framework code for part 2 is in the `cache` directory.**

The command line interface for the proxy should include two new components: the
number of segments and the size of the segments to be used for the interprocess
communication.  Instead of automatically relaying requests to the http server,
the proxy should query the cache to see if the file is available in local
memory already.  If the file is available there, it should send the cached
version.  In the real world, you would then have the proxy query the server to
obtain the file.  For the gradescope submission, however, **you need only
check the cache**. In other words, you only need to implement client, proxy,
cache, and the communication protocols, depicted in dark colors in the figure.

In the interprocess communication, **the proxy is responsible for
creating and destroying the shared memory (or message queues)**.  This is good
practice because in a scenario where the cache is connected to more than one
client (or proxy servers in this case) it makes it easier to ensure that:

- a non-responsive client doesn’t degrade the performance of the cache
- a malicious client doesn’t corrupt another’s data.

From the cache’s perspective this falls under the “don’t mess with my memory”
adage.

The cache must set up some communication mechanism (socket, message
queue, shared memory) by which the proxy can communicate its request along with
the information about the communication mechanism (shared memory name, id,
etc.)  For the purposes of this project, this mechanism does not have to be
robust to misbehaving clients.

Neither the cache daemon nor the proxy should crash if the other process is not
started already.   For instance, if the proxy cannot connect to the IPC
mechanism over which requests are communicated to the cache, then it might
delay for a second and try again.  The test server will start your cache and proxy
in different orders.

It is not polite to terminate a process without cleaning up, so the proxy
process (and perhaps the cache as well depending on your implementation) must
use a signal handler for both SIGTERM (signal for kill) and SIGINT (signal for
Ctrl-C) to first remove all existing IPC objects -- shared memory segments,
semaphores, message queues, or anything else -- and only then terminate.  The
test server will check for cleanup of IPC resources.

Depending on your implementation, it may be necessary to hard-code something
about how the proxy communicates its requests to the cache (e.g. a port number
or a name of shared memory segment).  This is acceptable for the purpose of the
assignment.

Here is a summary of the relevant files and their roles.

- **cache-student.h** - (modify and submit) this optional header file may be used by you, and included in any of the submitted files.  Its use
is optional, but it is there in case you would like to use it.

- **courses** - (not submitted) a directory containing files that will be returned from the cache server.  Use in conjunction with locals.txt.

- **gfclient_download** - a binary executable that serves as a workload generator for the proxy.  It downloads the requested files, using the current directory as a prefix for all paths. You must specify the correct port to use.

- **gfclient_measure** - a binary executable that serves as a workload generator for the proxy.  It measures the performance of the proxy, writing one entry in the metrics file for each chunk of data received. You must specify the correct port to use.

- **gfserver.h** - (not submitted) header file for the library that interacts with the getfile client.

- **gfserver.o** - (not submitted) object file for the library that interacts with the getfile client.

- **gfserver_noasan.o** - (not submitted) a version of **gfserver.o** created without [address sanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer) support. It may be used to test with [valgrind](http://www.valgrind.org/).

- **handle_with_cache.c** - (modify and submit) implement the handle_with_cache function here.  It should use one of the IPC mechanisms discussed to communicate with the simplecached process to obtain the file contents.  You may also need to add an initialization function that can be called from webproxy.c

- **locals.txt** - (not submitted) a file telling the simplecache where to look for its contents.

- **Makefile** - (not submitted) file used to compile the code.  Run `make` to compile your code.

- **shm_channel.[ch]** - (modify and submit) you may use these files to implement whatever protocol for the IPC you decide upon (e.g., use of designated socket-based communication, message queue, or shared memory ).

- **simplecache.[ch]** - (not submitted) these files contain code for a simple, static cache.  Use this interface in your simplecached implementation.  See the simplecache.h file for further documentation.

- **simplecached.c** - (modify and submit) this is the main file for the cache daemon process, which should receive requests from the proxy and serve up the contents of the cache using the simplecache interface. The process should use a boss-worker multithreaded pattern, where individual worker threads are responsible for handling a single request from the proxy.

- **steque.[ch]** - (not submitted) you may find this steque (stack and queue) data useful in implementing the cache.  Beware this uses malloc and is not suitable for shared memory.

- **webproxy.c** - (modify and submit) this is the main file for the webproxy program.  Only small modifications are necessary.  In addition to setting up the callbacks for the gfserver library, you may need to pre-create a pool of shared memory descriptors, a queue of free shared memory descriptors, associated synchronization variables, etc. The segment count and segement size parameters should be used.

- **workload.txt** - (not submitted) this file contains a list of files that can be requested of  https://github.com/gt-cs6200/image_data as part of your tests.

Once you have completed your program, you can submit it using Gradescope by copying the files we have indicated should
be submitted.  Any additional files you submit will not be used in grading your submission.  Any files you fail to
upload will not be used in grading your submission (and usually mean your code will not compile).


## References

### Relevant Lecture Material

- P3L3 Inter-Process Communication
    - SysV Shared Memory API
    - POSIX Shared Memory API

### Sample Source Code

- [Signal Handling Code Example](http://www.thegeekstuff.com/2012/03/catch-signals-sample-c-code/)
- [Libcurl Code Example](https://www.hackthissite.org/articles/read/1078)

### Address Sanitizer

Note that address sanitizer is enabled in the test server as well as the
project Makefile.  To fully exploit this, you may need to set further options
for your environment.

- [Google Sanitizer Wiki](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- ~~[Using GDB with Address Sanitizer](https://xstack.exascale-tech.com/wiki/index.php/Debugging_with_AddressSanitizer)~~ (This link is currently broken, we are looking for another comparable article).

Address Sanitizer is not compatible with valgrind.  If you wish to use valgrind, you
should build a version of your code without Address Sanitizer. In most cases, your
code will be tested with Address Sanitizer enabled.

## Rubric

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

### Part 1: Sockets (35 points)

#### Proxy Server: Sockets

- Correctly send client requests to server via curl
- Correctly send responses back to client using gfserver library

### Part 2: Shared Memory (55 points)

#### Proxy Server: Shared Memory

- Creation and use of shared memory segments
- Segment ID communication
- Cache request generation
- Cache response processing
- Synchronization
- Proper clean up of shared memory resources

#### Cache: Shared Memory

- Segment ID communication
- Proxy request processing
- Proxy response processing
- Synchronization

#### Cache: Command Channel

 - Creation of command channel
 - Proper clean up of command channel resources

Full credit requires: code compiles successfully, does not crash, files fully transmitted, basic safety checks (file present, end of file, etc.), and proper use of IPC - including the ability to start your cache/proxy in any order. Note that the automated tests will test some of these automatically, but graders may execute additional tests of these requirements.

### Report (10 points + 5 points Extra Credit)

- README (10 points + 5 point extra credit opportunity)
  - Clearly demonstrates your understanding of what you did and why - we
    want to see your _design_ and your explanation of the choices that you
    made _and why_ you made those choices. (4 points)
  - A description of the flow of control for your code; we strongly suggest
    that you use graphics here, but a thorough textual explanation is sufficient.
    (2 points)
  - A brief explanation of how you implemented and tested your code. (2 points)
  - References any external materials that you consulted during your
    development process (2 points)
  - Suggestions on how you would improve the documentation, sample code,
    testing, or other aspects of the project (up to 5 points *extra credit*
    available for noteworthy suggestions here, e.g., actual descriptions of
    how you would change things, sample code, code for tests, etc.) We do not
    give extra credit for simply _reporting_ an issue - we're looking for
    actionable suggestions on how to improve things.

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
