# GIOS-SPR-19-PR1 Project README file

This is **Yiran Xu (email: yxu629@gatech.edu)**'s CS-6200-O01 Graduate Introduction to Operating Systems Project 1 Readme file.

## Project Description
In this project, we implement the communication between a server and a client via socket programming. It aims to let me get familiar with C programming, C standard socket API, and threads (pthread). The whole projects includes a warm-up, a part1, and a part2.

###Warm-up: Sockets
The warm-up project includes two small sub-tasks: **echo client-server** and **file transfer**. This project is mainly to let me get familiar with the socket API.

- **echo client-server** is to implement a client-server communication with socket API. A client sends a message to a server, and the server receives the message and sends it back to the client. To complete this programming task, I follow the steps mentioned in the reference [Echo Client-Server Tutorial with Source Code](http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html). I changed the `bzeros()` to `memset()` as the former has been deprecated and the latter is recommended.
I have faced some problems during the submission. One is on the server side, I need to set socket option *SO_REUSEADDR* to solve the error "Address already in use". The other is **DO NOT** add `\n` at client to display the message from server. When I put `\n`, the Bonnie showed error that "Tests that the client prints the message sent from the server".

- **file transfer** is to send a file from a server to a client. I made a few modifications on **echo client-server** to fulfill the requirements. The server first reads data from a file, and then write the data to socket and send it to a client. It should be noticed that socket cannot guarantee that it can send all the data copied in the buffer, i.e., `data_sent != data_to_send` Instead, it will return a value to indicate how many data is successfully sent out. Therefore, when I implement the transfer functionality of the server, I set a point `*p` to track the position where the socket sends the data in the buffer. Once the server cannot read any data from the file, and the `data_to_sent` is equal to 0. The `while()`for transferring data exits .

###Part 1: Implementing the Getfile Protocol

The part1 project is to implement the getfile protocol between a client and a server. The client first sends a request to the server, the request format is following the getfile protocol. Upon reception of the request from the client, the server checks the header and extracts the path. Then the server will send a header message with a specific status along with the file contents to the client. The client will handle the incoming message by detect the status information and file length included in the header and receive the file contents from the server. 

- **gfserver** implementation is pretty straightforward. Generally, after establish the socket connection wit the client, the server keeps receiving the data for multiple times until it detects the end mark `\r\n\r\n` or the receiving round achieves a predefined maximum number. I did this to handle the situation that the header message can be very long so that the server cannot receive a completed header one time. Here in my implementation, I set the maximum number **`TIMER = 5`**. Therefore, the server can attempt to receive the data 5 times and concatenate the received data together. 

- Then, the server parse the received message to filter out the header information, such as _scheme_, _method_, and _path_. It is noted that the path should be start with `/`. If all the checks are successful, then server callback the registered `handler`. If the return value of the `handler` function is negative, the server will send back a response with status `ERROR` in header. Otherwise, the server will send back a response with status `OK` in header, along with the file length and the file contents. If the checks for the header elements are failed, for example, maybe the _method_ is not _GET_ or the end mark is not `\r\n\r\n`.

- **gfclient** is a bit complicated since it need to handler two types of header format, either with the file length (**`OK`**) or without the file length (**`INVALID`**, **`FILE NOT FOUND`**, **`ERROR`**). 

- The client first sends a request to the server to ask for file. I assembly the request message, mainly the header, and send it to the server. Then, the client starts to receive the data sent from the server. I use a flag **`is_header_received`** to indicate whether the header is received or not. The client need to first confirm the reception of a header message, and the header message can either include some contents or not. If **`is_header_received==0`**, the client will proceed the received message as header. I consider the following possibilities when receiving the header message. 
	- The server might close the socket during header transmission, so that the client will first check whether the return value of `recv()` is equal to 0. If yes, then it means the server prematurely closed the socket before all the header message are transmitted to the client. The status is set to **`ERROR`**.
	- Then the client verify the header format to detect the malformed header, in which the _scheme_ might not be correct, and the end mark is missing. The status is set to **`INVALID`**.
	- Then the header is parsed to get the status information and possible file length. It should be noted that when `sscanf()` return value is 2, which means the file length is included in the header message, indicating a **`OK`** status. If the return value of `sscanf()` is 1, then the status might be one of:  **`INVALID`**, **`FILE NOT FOUND`**, or **`ERROR`**. I will set **`is_header_received`** to 1. For other return value, the client will treat it as **`INVALID`**. If the status is **`INVALID`**, **`FILE NOT FOUND`**, or **`ERROR`**, the return value is set to **`-1`**.
	- If the client has successfully extracted the file length, and the received message not only includes the header, but also includes file contents. The client will read the file contents.
- When **`is_header_received==1`**, the messages received from the server are all file contents. Herein, the client need to handle the situation that the server might close the socket before all the file data are received by the client. In this case, because the header has already successfully received and is verified, the status should be set to  **`OK`**, and the return value is set to **`-1`**. If all the file contents are successfully received without loss, return value is set to **`-1`**.
- I followed [Guide to Solve IPv6 problem](https://beej.us/guide/bgnet/html/multi/getaddrinfoman.html) to use `getaddrinfo()` instead of `gethostbyname()`. It is noted that I need to convert the port number and pass by as a string.
- Also, in this project, I obtained a in-depth understanding of `strtok()`. It splits a string into multiple tokens, which are sequences of contiguous characters separated by any of the characters that are part of delimiters. Previously when I used it, I choose `\r\n\r\n` as delimiter to split the header part and content part. The results cannot give me the whole content part. Then I did some tests and finally figured out the reason: The `strtok()` scans the string based on the delimiter and stops if the terminating null character is found. For example, in the following sample code, I suppose the **`token2`** output `\r\n\r\nFile`, not my expected `\r\n\r\nFile contents starts here`. Therefore, I chose not to use `strtok()` to help me get the content part from the received message. Instead, I declared another buffer to store the content part.
```
 char samp_string[]= "GETFILE OK 200 \r\n\r\nFile contents starts here";
 printf("string is %s\n", samp_string);
 char *token1 = strtok(samp_string, "\r\n\r\n");
 char *token2 = strtok(NULL, " ");
 printf("first token is %s\n", token1);
 printf("second token is %s\n", token2);
```

###Part 2: Implementing a Multithreaded Getfile Server

In Part 1, the Getfile server can only handle a single request at a time. To overcome this limitation, I make Getfile server multi-threaded by implementing my own version of the handler in **handler.c** and updating **gfserver_main.c** as needed. Similarly on the client side, the Getfile workload generator can only download a single file at a time. I made the client multi-threaded by modifying **gfclient_download.c**.

- I have put a lot of efforts on understanding the project description. I found the course lecture is extremely useful for this part. I also read the reference [Multithread tutorials](https://randu.org/tutorials/threads/). To implement multi-threading in **gfserver**, I chose to put a `gfserver_set_pthreads()` that initializes work queue, threads and creates worker threads, a `gfs_handler()` that enqueues the requests from the client, a `threadFunc()` that proceeds the request by each worker thread, and a `gfserver_cleanup()` that destroys the queue, together in **handler.c**. By doing so, it is easy to make sure the work queue and mutex are globally declared and can be used appropriately by different functions, without causing possible issues. Herein, I used mutex lock/unlock to protect the work queue when the boss thread en-queues a request, and the worker thread pops a request from the queue. 

- To implement multi-threading **gfclient**, I generally followed what I did in **gfserver** and made a few changes. I assume `main()` is the boss thread, which parses and sets command line arguments, establishes the socket connection, initializes the request queue, and threads. Herein, I first locked the mutex, en-queued a request, then unlocked the mutex, and called `pthread_cond_signal()` to notify the worker threads. I also used `pthread_join()` to make sure all the worker threads completed their jobs, and at last, destroy the request queue and cleanup the global variables and their occupied memory spaces. 
- In the worker thread function `threadFunc()`, I used mutex lock and unlock to protect the request queue and make sure only one single worker thread can pop a request from the queue. I also used mutex lock and unlock to protect a global variable **`total_requests`**, which is equal to the product of **`nrequests`** and **`nthreads`**. Each worker thread will reduce **`total_requests`** by **`1`** when it pops a request from the queue. Once the worker thread found **`total_requests==0`**, all the workers stop and exit their threads. 
- I dealt with a few problems in this part2 project. First one is I put a `while(queue is not empty)` outside the mutex lock/unlock, and within the mutex lock/unlock, I also called a `pthread_cond_wait()` while `while(queue is empty)`, the code is look like this:

```
 while (!steque_isempty(req_queue)){
 	pthread_mutex_lock(&mutex);
        while(steque_isempty(req_queue)){
          pthread_cond_wait(&mutex_cv, &mutex);
        }
    req_obj = steque_pop(req_queue);
    total_requests = total_requests-1;

    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&mutex_cv);
 }
```
This will prevent the workers from calling `pthread_cond_wait()`. I also changed the order of thread creation and request en-queuing. I previously en-queued all the requests first, and then created the threads. After I discussed with TAs and other students in Slack/Piazza. I found my logic is not correct and then corrected them.
- In this part, I also found a hard time to understand the project description. I would like to improve the project description to add more examples, illustrations, and more detailed flowchart descriptions. For example, the project description mentioned "Each worker thread should issue up to a number of requests, also specified as a command line argument.". This assumes each thread has a predefined maximum number of requests. However, the reason why I introduce multi-threading in this project is because different threads can proceed different number of requests so that the overall system can be more efficient and productive. I previously used the  number of per-thread requests **`total_requests`** to determine when to exit the `while()` loop, but later on I realized maybe it would be better to use the number of total requests  **`total_requests`** to determine when to exit `while()` loop.
- Another problem costed me a lot of time is I did not put **`total_requests=nrequests*nthreads`** at the right place. I should put it after the "parse and set the command line arguments" part. Instead, I previous put it before the "parse and set the command line arguments" part. Thereby, **`total_requests`**
 is set to a fixed value **`54`**. When the input arguments of **`nrequests`** and **`pthreads`** has a small product number(**`<54`**), some of the worker threads did not exit the thread function, and it resulted in "The client is taking too long(probably hung)". Fortunately, I found and corrected it finally. 

## Known Bugs/Issues/Limitations
- In **gfclient**  of **Part 1**, I found that it is not necessary to call `gfc_set_headerfunc()` in `gfc_perform()`. I previously called `gfc_set_headerfunc()`, and then I faced a lot of errors, such as "pointers to zero pages". When I debug, I found out that `gfr->headerfunc()` is always **`NULL`**. After discussing with TAs and other students, I noticed that I do not need to call `gfr->headerfunc()` in my `gfc_perform()` function, just determine different status and based on each status, return a value (**`0`** or **`-1`**).

- I found that in **Part 2**, my code can still pass all the Bonnie tests even I did not use mutex lock/unlock in `gfs_handler()` that en-queues the requests from the client.

- I am not sure in **gfserver**, if the socket `recv()` or `send()` returns a negative value, should I consider it is an  **`ERROR`** and send it to the **gfclient**? In my implementation, I just called 
```
        fprintf(stderr, "ERROR reading from socket\n");
        exit(1);
```
or
```
      fprintf(stderr, "ERROR writing to socket\n");
      exit(1);
```

## Personal thoughts and suggestions
- Because I do not have too much background in computer science, I took a lot of time to understand what the project description is talking about. When I asked questions in slack and piazza, people are super friendly and they response very quickly. Yet, sometimes they use very specific CS-related terminologies or words that I cannot fully understand it. I know they might have solid CS background so that they assumed these kind of terminologies and words are very common and easy to understand. But the reality is I do not quite follow what they said. It would be better to use some plain words to explain something.
- To make the project description more read-friendly, it would be better to describe the functionalities of the server and the client separately: what they do? and how they do it? Then we can describe how server and client interact with each other in operation, e.g., how they communicate with each other. For the status, we can provides couple examples to help students understand why this status should be used for this action. For example, if the header is `SETFILE GET /Path\rnrn `, then the status should be **`INVALID`** since the header is malformed, `SETFILE` is not a legit _scheme_. 
- At last, I would like to thank everyone (Professor, TAs, ans classmates) in the class that helps me in the project!

## References

* [Server and Client Example Code](https://github.com/zx1986/xSinppet/tree/master/unix-socket-practice) - note the link referenced from the readme does not work, but the sample code is valid.
* [ Echo Client-Server Tutorial with Source Code](http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html)
* [Guide to Solve IPv6 problem](https://beej.us/guide/bgnet/html/multi/getaddrinfoman.html)
* [Multithread tutorials](https://randu.org/tutorials/threads/)
* [understanding of pthread_cond_wait() and pthread_cond_signal()](https://stackoverflow.com/questions/16522858/understanding-of-pthread-cond-wait-and-pthread-cond-signal)
* [pthread signal](https://stackoverflow.com/questions/29295280/pthread-cond-signal-from-multiple-threads)
* [High-level code design of multi-threaded GetFile server and client](https://docs.google.com/drawings/d/1a2LPUBv9a3GvrrGzoDu2EY4779-tJzMJ7Sz2ArcxWFU/edit?usp=sharing)