
# Project README file

This is **YOUR** Readme file.

## Project Description

Please see [Readme.md](Readme.md) for the rubric we use for evaluating your submission.

We will manually review your file looking for:

- A summary description of your project design.  If you wish to use grapics, please simply use a URL to point to a JPG or PNG file that we can review

- Any additional observations that you have about what you've done. Examples:
	- __What created problems for you?__
	- __What tests would you have added to the test suite?__
	- __If you were going to do this project again, how would you improve it?__
	- __If you didn't think something was clear in the documentation, what would you write instead?__

## Known Bugs/Issues/Limitations

__Please tell us what you know doesn't work in this submission__

## References

__Please include references to any external materials that you used in your project__

## Part 1
![part1](https://omscs6200-jmaguire.s3.amazonaws.com/pr4_part1.png)

Please see the above graphic for the control flow of the Part 1 of this assignment. `Streams` were used for the file transfers
for `fetch` and `store` as stream seemed like a more appropriate method to use over the `repeated` message type, which seemed
more appropriate to use for something like sending repeated file names. `List` used a map in order to convey the
information needed for each, as it was just a listing of the filenames with a particular attribute of that file conveyed as well.

The majority of the testing implemented for part 1 involved manual testing, so running the server and performing different actions
on the client side and comparing what happened to what was expected to occur.

## Part 2

![part2](https://omscs6200-jmaguire.s3.amazonaws.com/pr4_part2.png)

Please see the above graphic for the control flow for Part 2 of this assignment. To note though only the asynchronous parts
for part 2 are diagrammed, as the only change to the existing flow for part are the checks for the file existing in the 
fetch and store methods and the write locks. The `ALREADY_EXISTS` is handled by comparing the `CRC` for the file in question. 
The write locks are handled by the server keeping a map where the keys are the filenames and the values are the client id
that holds the lock (or an empty string if the file is unlocked). On a write request (store or delete) the client first
requests a lock, and the server checks if the client has the lock, if not the `RESOURCE_EXHAUSTED` status is returned.
For the write locks the acquiring, checking, and releasing were abstracted into their own methods to keep the flow a bit cleaner,
as well as make the implementation a bit easier. On the client side, prior to any call to `Store` or `Delete` the lock was 
first attempted to be acquired, and if it was not the client would return that the resource was exhausted. In addition to 
the client specifically acquiring the lock, on the server side the lock was also checked for and given if not held in 
order to account for misbehaving clients that did not request a lock prior to their request to `Store` or `Delete`. In a 
similar manner, the RPC methods that were brought over from part 1 generally function the same with some extra checks for 
the `ALREADY_EXISTS` and `DEADLINE_TIMEOUT` cases.

For the asynchrounous background threads on the client side, some extra checks needed to be done. For the `CallbackList` RPC
two maps were returned, one a mapping of the filename to its CRC and one a mapping of the filename to its MTIME. First, the 
CRC map is iterated through. If the file does not exist on the client it is fetched, if the file does exist on the client
then the CRC's are compared. In the case of matching CRC's nothing is done, but if the CRC's do not match then the file
that was most recently updated is considered the source of truth and the file is then either fetched from the server
or stored to server depending on where the most recently modified file currently resides. After that iteration completes, we
then iterate over the files that currently exist on the client. For each file on the client, if that file does not exist on the
server we then either call the `Delete` rpc method or the `Store` rpc method depending on if the client is mouting for the first time
or if the client has already been mounted. If the client is mounting for the first time we store the files, as these were files
created during a session when the client was offline and should be synced to the rest of the system, otherwise we delete the file
as it is a file that was deleted on the server and should not exist on the client.

Again, here the majority of the testing was manual testing. For testing the client locks `sleeps` were inserted into the 
RPC calls that require a write lock in order for one client to try to request a file that is currently locked by another.
For the asynchronous methods, multiple clients were run with different directories mounted and changes were made to the
files in each directory via the command line (as making changes through the GUI did not trigger the `inotify` calls in the
docker container) and then checking that the changes propogated across the various clients.

## Suggestions
My main suggestion would be an addition to the readme about changes to files made through the GUI (such as additions or deletions)
would not trigger the `inotify` callbacks within the docker container or VM.

## Sources
- [GRPC Deadlines](https://grpc.io/blog/deadlines/)
- [GRPC Status Codes](https://github.com/grpc/grpc/blob/master/doc/statuscodes.md)
- [GRPC Errors](http://avi.im/grpc-errors/#c)
- [GRPC Tutorial: Basic CPP](https://grpc.io/docs/tutorials/basic/cpp/)
- [GRPC Examples](https://github.com/grpc/grpc/tree/master/examples/cpp) -- mainly `Hello World` and `Route Guide`
- [Proto Buffers Language Guide: Maps](https://developers.google.com/protocol-buffers/docs/proto3#maps)
- [Proto Buffers Basics: C++](https://developers.google.com/protocol-buffers/docs/cpptutorial)
- [Stack Overflow: Using byte array](https://stackoverflow.com/questions/44926791/using-byte-array-cpp-in-grpc)
- [Stack Overflow: Iterate keys in a C++ map](https://stackoverflow.com/questions/1443793/iterate-keys-in-a-c-map/1443798)
- [Proto Buffers: CPP Generated Code: Map Fields](https://developers.google.com/protocol-buffers/docs/reference/cpp-generated#map-fields)
- [Stack Overflow: How to use Google Protobuf Map in C++](https://stackoverflow.com/questions/30622258/how-to-use-google-protobuf-map-in-c)
- [CPP Refernce: RAII Locks](https://en.cppreference.com/w/cpp/language/raii)
- [Stack Overflow: Check if file exists without opening it](https://stackoverflow.com/questions/18100391/check-if-a-file-exists-without-opening-it)
- [Input/Output with Files C++](http://www.cplusplus.com/doc/tutorial/files/)
