# Project One README

## Echo Client-Server

### Echo Client
#### Project Design 
The overall goal of the Echo client is to be able to send a message to an Echo server counterpart and receive the message and print to the console. This is achieved through the concept of Socket programming. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this.  
#### Trade-offs and Design decisions
1. Even though IPv6 wasn't going to be tested for this part of the project, I started working in that direction using sockaddr_in6 instead of sockaddr_in and sin6_family, sin6_port and sin6_addr along with in6addr_any. Even though I submitted the client with family manually set to AF_INET, eventually this is changed to AF_INET6 and some additional logic to account for IPv4 and IPv6 addresses.
2. I was initially printing the received message back to the console with an additional newline and it was failing the tests on Gradescope. Removed it later to fix that issue.  
#### Control Flow
1. Create a socket of type SOCK_STREAM and assign it to a socket file descriptor
2. Set the sockaddr_in6 struct information which in turn populates the sockaddr needed for socket connections
3. Connect the sockaddr_in6 to the socket file descriptor created in Step 1
4. Send the message to the socket file descriptor along with the string length. Assumption was made that the message is short enough to have a fixed Buffer Size (+1 for null terminating character) and also that the message will be sent in one single `send()`
5. Receive the message from the server and append the null terminating character to the last element of the message. Same assumptions made as in point 4
6. Print out the message to the client's console  
#### Testing Process
Worked on the Echo server first, so had the server running and testing the client with it to make sure the behavior is as intended.  
#### References
1. The Echo client was created after the Echo server, so some of the code in the Echo client was borrowed directly from the Echo server portion (To be more specific, the lines on socket, connect, send, receive and the sockaddr_in6 information)
2. Heavily referenced Beej's guide to network programming for the socket programming parts including socket, connect, send, receive and setting up the sockaddr_in struct
3. Some online links referenced:
- https://stackoverflow.com/questions/51911564/cannot-bind-socket-to-port-in-both-ipv4-and-ipv6
- https://security.stackexchange.com/questions/92081/is-receiving-ipv4-connections-on-af-inet6-sockets-insecure
- https://stackoverflow.com/questions/18609397/whats-the-difference-between-sockaddr-sockaddr-in-and-sockaddr-in6  


### Echo Server
#### Project Design
The overall goal of the Echo server is to be able to receive a message from an Echo client counterpart and relay back the message as it is to the client. This is achieved through the concept of Socket programming. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this.  
#### Trade-offs and Design decisions
1. Similar design decisions as the Echo client, but in this one the family type of AF_INET6 as it wasn't giving any issues for working with IPv6 clients as well. Follows similar approach of using sockaddr_in6 instead of sockaddr_in and sin6_family, sin6_port and sin6_addr along with in6addr_any
2. I was getting "Address already in use" when testing the server locally and found setsockopt to use SO_REUSEADDR along with an optval in Beej's Guide to network programming  
#### Control Flow
1. Create a socket of type SOCK_STREAM and assign it to a socket file descriptor
2. Set the sockaddr_in6 struct information which in turn populates the sockaddr needed for socket connections
3. Bind the sockaddr_in6 to the socket file descriptor created in Step 1
4. Start listening to connections on the socket file descriptor
5. In a forever loop (since the server is not expected to close) accept an incoming sockaddr connection from a client to the socket file descriptor
6. Receive the message from the client and append the null terminating character to the last element of the message
7. Send the message buffer back to the client to the client socket file descriptor that is created from the accept command in Step 5
8. Close the client socket file descriptor once the request has been served  
#### Testing Process
Had the server up and running in a terminal and used an app called Packet Sender on Mac to simulate a client connecting to the server on localhost at port 30605. To simulate an IPv6 client I used the address as ::1 address. No additional tests were written or binaries used for this part.  
#### References
1. Heavily referenced Beej's guide to network programming for the socket programming parts including socket, bind, accept, receive, send, and setting up the sockaddr_in struct
2. Some online links referenced:
- https://stackoverflow.com/questions/12102332/when-should-i-use-perror-and-fprintfstderr
- https://stackoverflow.com/questions/1618240/how-to-support-both-ipv4-and-ipv6-connections  


## File Transfer

### File Transfer Client
#### Project Design
The overall goal of the Transfer client is to be able to connect to a Transfer server counterpart and receive some data and store it in a local file. This is achieved through the concept of Socket programming. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this.  
#### Trade-offs and Design decisions
1. Similar design decisions as the Echo server. Follows similar approach of using sockaddr_in6 instead of sockaddr_in and sin6_family, sin6_port and sin6_addr along with in6addr_any to work with IPv4 and IPv6 hosts
2. Receive method is expected to handle cases where the entire data isnt' sent in one request and also allows for chunking of the receive buffer. So the file data is received in a loop as long as the `recv()` returns a positive number indicating the number of bytes transferred
3. fseek is used to reset the file pointer to the start of the file and fwrite is used to write the data in a buffer to the file  
#### Control Flow
1. We follow the exact same steps for Echo client's steps 1-3
2. Open a file pointer to the default or user overwridden file name
3. While there's still being received from the server, fwrite the data to the file pointer
4. Clear out the buffer within the loop while receiving data
5. Close the File pointer and the Socket file descriptor  
#### Testing Process
Worked on the Transfer server first, so had the server running and testing the client with it to make sure the behavior is as intended and the entire file gets downloaded.  
#### References
1. Heavily referenced Beej's guide to network programming for the socket programming parts including socket, bind, accept, receive, send, and setting up the sockaddr_in struct
2. Directly used some of the boilerplate code that was written for Echo Client
3. Some online links referenced:
- https://www.tutorialspoint.com/c_standard_library/c_function_fwrite.htm
- Man pages  


### File Transfer Server
#### Project Design
The overall goal of the Transfer server is to be able to receive connections from different clients and send some data in a local file over the network. This is achieved through the concept of Socket programming. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this.  
#### Trade-offs and Design decisions
1. Similar design decisions as the Echo server. Follows similar approach of using sockaddr_in6 instead of sockaddr_in and sin6_family, sin6_port and sin6_addr along with in6addr_any to work with IPv4 and IPv6 hosts
2. Send method is expected to handle cases where the entire data isnt' sent in one request and also allows for chunking of the send buffer. So the file data is sent in a loop as long as the `fread()` returns a positive number indicating the number of bytes read from a local file
3. fseek is used to reset the file pointer to the start of the file and fwrite is used to write the data in a buffer to the file
4. I was getting "Address already in use" when testing the server locally and found setsockopt to use SO_REUSEADDR along with an optval in Beej's Guide to network programming  
#### Control Flow
1. We follow the exact same steps for Echo server's steps 1-5
2. Once a new connection is accepted, we open a file pointer to the local file on the server and fseek to the beginning of the file
3. In a loop, as long as fread returns a positive number indicating the number of bytes read from the file, `send()` to the client at the socket file descriptor from the accepted connection
4. Close the client socket after the file is completed but keep the server open for future client requests  
#### Testing Process
Similar to the Echo server, had the server up and running in a terminal and used an app called Packet Sender on Mac to simulate a client connecting to the server on localhost at port 30605. To simulate an IPv6 client I used the address as ::1 address. No additional tests were written or binaries used for this part.  
#### References
1. Heavily referenced Beej's guide to network programming for the socket programming parts including socket, bind, accept, receive, send, and setting up the sockaddr_in struct
2. Directly used some of the boilerplate code that was written for Echo Server
3. Some online links referenced:
- https://stackoverflow.com/questions/15697783/how-does-fread-know-when-the-file-is-over-in-c
- https://stackoverflow.com/questions/33327720/how-to-use-fread-to-read-the-entire-file-in-a-loop
- https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
- https://stackoverflow.com/questions/584142/what-is-the-difference-between-read-and-fread


## Part 1: Implementing the Getfile Protocol

### GET File Client
#### Project Design
The overall goal of the GET File client is to be able to connect to a GET File server counterpart and receive files in a single threaded fashion and store it in a local file. This is achieved through the concept of Socket programming. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this. The protocol resembles HTTP loosely. The difference of this from the previous parts is here the data is not assumed to be null terminating, making it language neutral and allowing to transfer files of any file format.
#### Trade-offs and Design decisions
1. Design patterns of clients from previous parts to accomodate for IPv6 and IPv4 clients is continued to be in use
2. `gfcrequest_t` struct which has information about the client request has been populated primarily from the getter and setter functions in the header file, and in addition is made to hold a key other parameters relevant to a request like bytes received, file len and request status
3. `gfcrequest_t` is initialized by allocating it memory and setting some defaults which help when the request has to be exited early because of an Invalid response from the server
4. Port number has been converted from long to char to use inside of getaddrinfo() and in conjunction with inet_pton, this has been done to accomodate for both IPv4 and IPv6 addresses
5. The whole set of variables have been grouped into - Header out, Header in, and Data in to accomodate for the different steps of the client process
6. Since Header Out (request) to the server is simple string, the previous parts of the project has been leveraged mostly for this part and nothing new in that section
7. Header response in from the server is received in a loop (for partial header receives) until strstr() finds a match in the header received so far. I was having issues getting INVALID conditions handled so as a first step, it does some sanity checks to exit out of the request if those conditions are met
8. The first version of the client I had was using strcpy and other str functions and I was having a lot of issues (this was warned in the project readme). So I changed them to memcpy functions and that resolved the issues
9. I'm using strtok() to split the received header at the header delimiter and get two pointers, one at the start of the header and another at the start of the spillover content after the delimeter (if there's any)
10. I make a copy of the header and use some pointer arithmetic to find the index of the start of the content (if there's any) and use that to populate a data buffer that gets sent first to the client
11. I have a lot of previous experiences with Regexes and although C had it's nuances, I was able to use Regex to validate different conditions and extract the status and filelen (if status was OK) and set the respective variables
12. From there it's standard receiving of file again similar to previous parts
13. The different exit codes have been set based on some of the test case fail messages from Gradescope and also from the header files that describe different status codes expected by the client to return
#### Control Flow
1. gfclient_download.c describes the flow of calls that will be made from gfclient.c and it starts with creating a `gfcrequest_t` struct and initializing it to hold the information regarding the client's request
2. gfc_perform is called to make the actual request and this function is iniatilized will the variables needed in the scope of this function
3. All the steps in the previous parts up to connect() are similar and setup the client to be able to connect to the server
4. Using the path variable, the header out is constructed and sent to the server
5. The response from the server is received in 2 parts and the first part is receiving the header, which is received in a loop to account for partial receives until the response delimeter is found
6. The initial response received is broken down into header in and spillover content to process them separately. The header in is used to parse information regarding status and filelen or other conditions like an invalid or file not found status and handle them accordingly
7. The spillover content along with data from subsequent `recv()` calls in a repeat loop (to allow for partial data receive in the buffer) are saved in a file locally using writefunc callback function which has been provided
#### Testing Process
- Unlike the previous parts, this part called for some serious testing and debugging and even though I didn't write any of my own test cases, I heavily used GDB
- Setting breakpoints and stepping through the lines wathing the variables was incredibly helpful in testing and debugging a lot of the errors
- I also started to use valgrind to debug a lot of the memory leak issues and help to fix them
- Finally, the Savyas binaries in the Interop thread on Piazza has been extremely helpful to test the client against and making sure all the files are transferred successfuly
#### References
1. All the basic socket programming boilerplate code from the previous parts
2. Some online links referenced:
- https://www.geeksforgeeks.org/double-pointer-pointer-pointer-c/
- https://www.geeksforgeeks.org/function-pointer-in-c/
- https://stackoverflow.com/questions/17226876/recv-call-hangs-after-remote-host-terminates
- https://www.tutorialspoint.com/c_standard_library/c_function_strstr.htm
- https://www.educative.io/edpresso/c-copying-data-using-the-memcpy-function-in-c
- https://www.educative.io/edpresso/how-to-write-regular-expressions-in-c
- https://stackoverflow.com/questions/4090434/strtok-char-array-versus-char-pointer

### GET File Server
#### Project Design
The overall goal of the GET File server is to be able to connect to a GET File client counterpart and receive a request and send the appropriate file in a single threaded fashion. This is achieved through the concept of Socket programming. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this. The protocol resembles HTTP loosely. The difference of this from the previous parts is here the data is not assumed to be null terminating, making it language neutral and allowing to transfer files of any file format.
#### Trade-offs and Design decisions
1. We create and populate the `gfserver_t` struct to hold the information relevant to the server and `gfcontext_t` struct to hold the relevant information related to a client's request. This is iniatilized similar to how we initialized gfcrequest_t in the GET File client
2. The `gfs_send` takes in the client context set up in the previous step and the send() is done with the arguments that the function gets. This sends the data of the file
3. The `gfs_sendheader` is for the client to send a header response back to the client. Based on the status the function receives as an argument, the appropriate header is sent back to the client to the client socket from the client context defined in step 1
4. gfs_perform is responsible for serving the client's request and is iniatilized with a couple of local variables to serve the request
5. The request header is received from the client in a loop as long as the response delimeter is not found and the size of the header is not more than a pre-defined maximum length for header. Did this so that we don't keep on receiving junk header which might never have the response delimeter but send thousands of bytes
6. Using some regex functions to validate the expected request header pattern and send appropriate response back to the client in case of issues. If no issues in validating the header request, we parse out the path and copy it into the client context from step 1
7. The handler is provided for this part and we just invoke this function with the client context and the file path from step 6 and store the status of the response in the client context struct
#### Control Flow
1. Helper setter functions for the server are defined and similar to the GET File client, we create and initialize the server context
2. Other helper functions like `gfs_send` and `gfs_sendheader` are defined as mentioned in the previous section
3. The main gfserver_serve function is initalized with variables to hold the server details
4. All the steps to get the server started follow server defined in previous parts up to the listen() call
5. The while loop will accept new connections in a forever loop and a new client context is created and initialized for each new request synchronously
6. We accept() a new request and assign the client socket file descriptor to the client context
7. While the request header delimitor is not found, we `recv()` the header string from the client
8. Using regex, we extract the file path if the pattern is matched or send the appropriate error message in case the pattern is not found
9. The given handler is called with the client context and file path to serve the request and send the file to the client
#### Testing Process
- Similar to the GET File client, this part called for some serious testing and debugging and even though I didn't write any of my own test cases, I heavily used GDB
- Setting breakpoints and stepping through the lines wathing the variables was incredibly helpful in testing and debugging a lot of the errors
- I also used valgrind to debug a lot of the memory leak issues and help to fix them
- Finally, the Savyas binaries and some other binaries posted in the Interop thread on Piazza has been extremely helpful to test the server against and making sure all the files are transferred successfuly
#### References
1. All the basic socket programming boilerplate code from the previous parts
2. Some online links referenced:
- https://stackoverflow.com/questions/52129581/in-c-how-to-get-capturing-group-regex
- https://stackoverflow.com/questions/53560654/how-to-extract-a-substring-from-a-string-in-c-with-a-fixed-pattern
- https://stackoverflow.com/questions/2023947/append-formatted-string-to-string-in-c-not-c-without-pointer-arithmetic
- https://stackoverflow.com/questions/30302294/extract-string-between-two-specific-strings-in-c
- https://stackoverflow.com/questions/39463073/c-concatenate-string-in-while-loop
- https://stackoverflow.com/questions/11377631/regex-in-c-language-using-functions-regcomp-and-regexec-toggles-between-first-an


## Part 2: Implementing a Multithreaded Getfile Server

### Multithreaded GET File Client
#### Project Design
The overall goal of the Multithreaded GET File client is to be able to connect to a GET File server counterpart and receive files in a multi threaded fashion and store it in a local file in parallel. This is achieved through the concept of Socket programming and multithreading with pThreads. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this. The protocol resembles HTTP loosely. The difference of this from the previous parts is here the data is not assumed to be null terminating, making it language neutral and allowing to transfer files of any file format. In addition now the client makes request for multiple files in parallel, each handled in it's own thread
#### Trade-offs and Design decisions
1. Initially started off with one Mutex and Condition Variable for locking access to the steque. But towards the end of the process added another Mutex and Condition Variable to be able to keep track of number of jobs left so that a signal could be broadcasted to all the active threads to exit and close the client connection to the server
2. Leveraged worker arguments struct to pass in the information from the main function of the client to the worker being assigned the task
3. To handle edge cases where the file path might be randomly provided, there's a max file path check to exit out of the process early for that particular request
4. The main thread locking the queue and enqueuing a new request and notifying the workers is straightforward but was having issues with unused threads being left open and pthread_join not working
5. To address the previous issue, from main thread, the jobsleft variable is locked and it's put on wait as long as there are jobs left to process
6. To continue addressing the previous issue, from a worker, at the end of it processing the request, we lock the jobsleft variable and decrement it. We signal the main thread and it enters step 5 and check if there are still any jobs being processed so it can wait
7. A steque can only be empty when either there are pending jobs to process but they haven't been queued yet or all the jobs have been processed. In the later case when all jobs have been processed, the last worker thread that makes jobsleft to 0 signals the Condition Variable in main and it in turn broadcasts to pending workers to close out. This is achieved by checking the value of jobsleft and exiting out of that thread early so that the main thread then can continue to proceed joining the threads
8. The main worker thread code had been provided and the key challenges was handling the mutexes and condition variables and more importantly, to cleanly shut down the threads
#### Control Flow
1. From the main thread, we create and initialize the number of threads as specified and we initialize the steque queue to hold the outgoing requests to process
2. We populate the worker argument values and this gets passed along with the thread information and a function pointer for the worker to step 1
3. For each of the number of requests we need to make, we lock the mutex, enqueue the requested path on to the queue and signal the workers to pick up the jobs
4. The main thread is then suspended in wait condition there until the number of jobs left to process is a positive integer
5. The worker counterpart function is initialized with required variables and the boilerplate code is used inside the worker function to connect to the server and download the file from the requested path
6. The worker thread waits until the steque has some new requests to process. Inside here we have an early exit logic in the cases where there are no more jobs left to process. The last worker signals the main thread and in turn the main thread broadcasts all pending workers to exit out if all jobs have been processed
7. Once the job is done by the worker, we lock the jobsleft variable and decrement it and then signal the main thread to check if that was the last job. If it is the last job the main thread sends a broadcast to pending workers to close out and if it isn't the last job, the main thread continues to wait
#### Testing Process
- Similar to the single threaded GET File client, this part called for some serious testing and debugging and even though I didn't write any of my own test cases, I heavily used GDB
- Setting breakpoints and stepping through the lines wathing the variables was incredibly helpful in testing and debugging a lot of the errors
- I also used valgrind to debug a lot of the memory leak issues and help to fix them
- Finally, the Savyas binaries and some other binaries posted in the Interop thread on Piazza has been extremely helpful to test the client against and making sure all the files are transferred successfuly
#### References
1. All the basic socket programming boilerplate code from the previous parts and some pre-canned code for the worker thread given in the project
2. Class lectures in P2L3 and https://computing.llnl.gov/tutorials/pthreads/
3. Some online links referenced:
- https://stackoverflow.com/questions/6154539/how-can-i-wait-for-any-all-pthreads-to-complete
- https://stackoverflow.com/questions/20824229/when-to-use-pthread-exit-and-when-to-use-pthread-join-in-linux
- https://stackoverflow.com/questions/38027534/pthread-join-not-working
- https://stackoverflow.com/questions/26601525/why-is-pthread-join-not-returning
- https://stackoverflow.com/questions/15676027/pthread-broadcast-and-then-wait
- https://stackoverflow.com/questions/1352749/multiple-arguments-to-function-called-by-pthread-create


### Multithreaded GET File Server
#### Project Design
The overall goal of the Multithreaded GET File server is to be able to connect to a GET File client counterpart and send files in a multi threaded fashion after validating an incoming request from a client. This is achieved through the concept of Socket programming and multithreading with pThreads. The process follows a certain sequence of steps (described in the control flow) as described in the sockets API to be able to accomplish this. The protocol resembles HTTP loosely. The difference of this from the previous parts is here the data is not assumed to be null terminating, making it language neutral and allowing to transfer files of any file format. In addition now the client makes request for multiple files in parallel, and each incoming request has to be handled in a separate thread
#### Trade-offs and Design decisions
1. Initially the plan was to have the mutex and steque as extern to the handler so the handler can take the client context and path and queue it to the queue. But because of issues ran into while doing so, I made the decision to make the handler make a call to an extern function which is defined in gfserver_main to enqueue the request
2. Because of the point above, the extern function took a triple pointer as an argument, which got the address of the double pointer ctx client context and this function would be called in gfserver_main, to which the queue and mutex are local
3. The gfserver_main follows a similar approach as the Multithreaded GET File client until the thread creation, initialization and steque initialization
4. The worker thread waits until the queue has some client requests, and once it does, it locks the mutex, pops an item off the queue to get the context
5. content_get helper function that was provided returns -1 in case the file path is not found which makes it easy to check for cases of FILE_NOT_FOUND response status to send back to the client
6. Used fstat to get the file stat, primarily for the file length. And also to test for the ability to read the file and this function is thread safe. We exit out early in the case this returns -1
7. We use pread instead of read, which is thread-safe as well and the data is sent to the client through `gfs_send` and the header is sent using the gfs_sendheader
#### Control Flow
1. The main thread of the server creates and initializes the defined number of threads tied with a worker function pointer and a steque to hold the incoming requests
2. From inside gfserver_serve, when a new request from a client is received, the handler is called. Which in turn locks the queue mutex and enqueues the context and path from the client request
3. The enqueue from step 2 signals the workers waiting on the condition variable and they pick up the task and we enter the worker function
4. Inside the worker, we lock the mutex to pop an in item off the queue and then unlock back the mutex
5. We get the client context and path from the item on the queue and use it for subsequent steps
6. With some helper functions like content_get to see if the file exists and fstat to get status on the requested file descriptor to gets is size and check any issues with it, we're able to identify the appropriate header to send back to the client and send it via gfs_sendheader
7. Finally, we read in the file contents with thread-safe pread and as long as this returns a positive number (data being read from the file), we use `gfs_send` to send the data of the file contents to the client. This process follows similar pattern used in servers in previous parts
8. Finally we exit out of the worker after freeing up the worker resources and memory
9. The main thread loops forever was the requirment for this project but in case it does exit the loop, the threads are joined at the end to exit all the threads
#### Testing Process
- Similar to the GET File server, this part called for some serious testing and debugging and even though I didn't write any of my own test cases, I heavily used GDB
- Setting breakpoints and stepping through the lines wathing the variables was incredibly helpful in testing and debugging a lot of the errors
- I also used valgrind to debug a lot of the memory leak issues and help to fix them
- Finally, the Savyas binaries and some other binaries posted in the Interop thread on Piazza has been extremely helpful to test the server against and making sure all the files are transferred successfuly
#### References
1. All the basic socket programming boilerplate code from the previous parts and some pre-canned code for the worker thread given in the project
2. Class lectures in P2L3 and https://computing.llnl.gov/tutorials/pthreads/
- https://stackoverflow.com/questions/39938648/copy-one-pointer-content-to-another
- https://stackoverflow.com/questions/4339201/c-how-to-pass-a-double-pointer-to-a-function
- https://stackoverflow.com/questions/60024682/how-to-signal-the-worker-threads-that-there-is-some-work-to-be-finished
- https://stackoverflow.com/questions/28939904/how-to-use-values-of-local-variable-in-another-function-in-c
- https://stackoverflow.com/questions/35231110/passing-double-pointer-as-argument
- https://stackoverflow.com/questions/51579267/addresssanitizer-heap-buffer-overflow-on-address
- https://stackoverflow.com/questions/6537436/how-do-you-get-file-size-by-fd
- https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c