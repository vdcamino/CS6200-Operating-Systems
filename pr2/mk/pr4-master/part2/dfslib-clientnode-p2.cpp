#include <regex>
#include <mutex>
#include <vector>
#include <string>
#include <thread>
#include <cstdio>
#include <chrono>
#include <errno.h>
#include <csignal>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>
#include <sys/inotify.h>
#include <grpcpp/grpcpp.h>
#include <utime.h>

#include "src/dfs-utils.h"
#include "src/dfslibx-clientnode-p2.h"
#include "dfslib-shared-p2.h"
#include "dfslib-clientnode-p2.h"
#include "proto-src/dfs-service.grpc.pb.h"

using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;


extern dfs_log_level_e DFS_LOG_LEVEL;

//
// STUDENT INSTRUCTION:
//
// Change these "using" aliases to the specific
// message types you are using to indicate
// a file request and a listing of files from the server.
//
using FileRequestType = dfs_service::CallbackListRequest;
using FileListResponseType = dfs_service::CallbackListReply;

using dfs_service::DFSService;

using dfs_service::StoreFileRequest;
using dfs_service::StoreFileReply;

using dfs_service::FetchFileRequest;
using dfs_service::FetchFileReply;

using dfs_service::FetchFileStat;
using dfs_service::FileStatReply;

using dfs_service::ListFilesRequest;
using dfs_service::ListFilesReply;

using dfs_service::DeleteFileRequest;
using dfs_service::DeleteFileReply;

using dfs_service::WriteLockRequest;
using dfs_service::WriteLockReply;

using dfs_service::CallbackListRequest;
using dfs_service::CallbackListReply;

DFSClientNodeP2::DFSClientNodeP2() : DFSClientNode() {}
DFSClientNodeP2::~DFSClientNodeP2() {}

grpc::StatusCode DFSClientNodeP2::RequestWriteAccess(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to obtain a write lock here when trying to store a file.
    // This method should request a write lock for the given file at the server,
    // so that the current client becomes the sole creator/writer. If the server
    // responds with a RESOURCE_EXHAUSTED response, the client should cancel
    // the current file storage
    //
    // The StatusCode response should be:
    //
    // OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
    // StatusCode::CANCELLED otherwise

  WriteLockRequest request;
  WriteLockReply response;
  ClientContext ctx;

  ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));
  request.set_filename(filename);
  request.set_client_id(this->ClientId());
  Status status = service_stub->acquireWriteLock(&ctx, request, &response);

  if (status.ok()) {
    return grpc::OK;
  } else if (status.error_code() == 8) {
    return grpc::RESOURCE_EXHAUSTED;
  } else if (status.error_code() == 4) {
    return grpc::DEADLINE_EXCEEDED;
  } else {
    return grpc::CANCELLED;
  }
}

grpc::StatusCode DFSClientNodeP2::Store(const std::string &filename) {

    // STUDENT INSTRUCTION:
    //
    // Add your request to store a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation. However, you will
    // need to adjust this method to recognize when a file trying to be
    // stored is the same on the server (i.e. the ALREADY_EXISTS gRPC response).
    //
    // You will also need to add a request for a write lock before attempting to store.
    //
    // If the write lock request fails, you should return a status of RESOURCE_EXHAUSTED
    // and cancel the current operation.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::ALREADY_EXISTS - if the local cached file has not changed from the server version
    // StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
    // StatusCode::CANCELLED otherwise
    //
    //
    StoreFileRequest request;
    StoreFileReply response;
    ClientContext ctx;
    long read_len, file_len, total_bytes = 0;
    read_len = 4096;
    char buffer[read_len];
    struct stat statBuf;
    std::fstream requestedFile;
    requestedFile.open(WrapPath(filename), std::ios::in | std::ios::binary);
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));

    if (requestedFile.is_open()) {
      std::unique_ptr<ClientWriter<StoreFileRequest> > writer(service_stub->storeFile(&ctx, &response));
      int getStat = stat(WrapPath(filename).c_str(), &statBuf);
      if (getStat == -1) {
        std::cout << "Error: " << strerror(errno) << std::endl;
      }
      file_len = (long) statBuf.st_size;
      if (file_len < read_len) {
        read_len = file_len;
      }

      StatusCode statusCode = this->RequestWriteAccess(filename);
      if (statusCode == grpc::OK) {
        std::uint32_t client_crc = dfs_file_checksum(WrapPath(filename), &this->crc_table);
        request.set_crc(client_crc);
        request.set_client_id(this->ClientId());

        while(total_bytes < file_len) {
          if (file_len - total_bytes <  read_len) {
            read_len = file_len - total_bytes;
          }
          requestedFile.seekg(total_bytes);
          requestedFile.read(buffer, read_len);
          request.set_size(read_len);
          request.set_filedata(buffer, read_len);
          request.set_filename(filename);
          if(!writer->Write(request)) {
            break;
          }
          total_bytes += read_len;
        }

        requestedFile.close();
        writer->WritesDone();
        Status status = writer->Finish();
        if (status.ok()) {
          return grpc::OK;
        } else if (status.error_code() == 8) {
          std::cout << "File locked by someone else after initial lock attempt" << std::endl;
          return grpc::RESOURCE_EXHAUSTED; 
        } else if (status.error_code() == 6) {
          std::cout << "File exists" << std::endl;
          return grpc::ALREADY_EXISTS;
        } else if (status.error_code() == 4) {
          return grpc::DEADLINE_EXCEEDED;
        } else {
          std::cout << "Errors: " << status.error_message() << std::endl;
          return grpc::CANCELLED;
        }
      } else {
        std::cout << "File locked on initial lock attempt" << std::endl;
        return grpc::RESOURCE_EXHAUSTED; 
      }
    } else {
      std::cout << "Error: " << strerror(errno) << std::endl;
      return grpc::CANCELLED;
    }
}


grpc::StatusCode DFSClientNodeP2::Fetch(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to fetch a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation. However, you will
    // need to adjust this method to recognize when a file trying to be
    // fetched is the same on the client (i.e. the files do not differ
    // between the client and server and a fetch would be unnecessary.
    //
    // The StatusCode response should be:
    //
    // OK - if all went well
    // DEADLINE_EXCEEDED - if the deadline timeout occurs
    // NOT_FOUND - if the file cannot be found on the server
    // ALREADY_EXISTS - if the local cached file has not changed from the server version
    // CANCELLED otherwise
    //
    // Hint: You may want to match the mtime on local files to the server's mtime
    //
    FetchFileRequest request;
    FetchFileReply response;

    FetchFileStat stat_request;
    FileStatReply stat_response;
    ClientContext ctx;
    ClientContext ctx_2;

    std::fstream requestedFile;
    int read_len;
    long total_read = 0;
    struct stat statBuf;
    ctx_2.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));

    stat_request.set_filename(filename);
    Status status = service_stub->fileStat(&ctx, stat_request, &stat_response);
    std::uint32_t client_crc = dfs_file_checksum(WrapPath(filename), &this->crc_table);

    stat(WrapPath(filename).c_str(), &statBuf);

    if (stat_response.crc() == client_crc) {
      return grpc::ALREADY_EXISTS;
    } else {
      requestedFile.open(WrapPath(filename), std::ios::out | std::ios::binary);
      if(!requestedFile.is_open()) {
        std::cout << "Error opening file" << std::endl;
      }

      request.set_filename(filename);

      std::unique_ptr<ClientReader<FetchFileReply> > reader(service_stub->fetchFile(&ctx_2, request));

      while (reader->Read(&response)) {
        read_len = response.size();
        requestedFile.seekg(total_read);
        requestedFile.write(response.filedata().c_str(), read_len);
        total_read += read_len;
      }

      Status status = reader->Finish();
       if (status.ok()) {
        return grpc::OK;
      } else if (status.error_code() == 5) {
        std::cout << "Error: file not found" << std::endl;
        remove(WrapPath(filename).c_str());
        return grpc::NOT_FOUND;
      } else if (status.error_code() == 4) {
        return grpc::DEADLINE_EXCEEDED;
      } else {
        return grpc::CANCELLED;
      }
    }
}

grpc::StatusCode DFSClientNodeP2::Delete(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to delete a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You will also need to add a request for a write lock before attempting to delete.
    //
    // If the write lock request fails, you should return a status of RESOURCE_EXHAUSTED
    // and cancel the current operation.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::RESOURCE_EXHAUSTED - if a write lock cannot be obtained
    // StatusCode::CANCELLED otherwise
    //
    //
    
    DeleteFileRequest request;

    DeleteFileReply response;
    ClientContext ctx;

    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));
    request.set_filename(filename);
    request.set_client_id(this->ClientId());
    StatusCode statusCode = this->RequestWriteAccess(filename);
    if (statusCode == grpc::OK) {
      Status status = service_stub->deleteFile(&ctx, request, &response);
      remove(WrapPath(filename).c_str());
      if (status.ok()) {
        return grpc::OK;
      } else if (status.error_code() == 4) {
        return grpc::DEADLINE_EXCEEDED;
      } else {
        return grpc::CANCELLED;
      }
    } else {
      return grpc::RESOURCE_EXHAUSTED;
    }
}

grpc::StatusCode DFSClientNodeP2::List(std::map<std::string,int>* file_map, bool display) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to list files here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation and add any additional
    // listing details that would be useful to your solution to the list response.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::CANCELLED otherwise
    //
    //
    ListFilesRequest request;

    ListFilesReply response;
    ClientContext ctx;

    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));
    Status status = service_stub->listFiles(&ctx, request, &response);

    if (status.ok()) {
      for ( auto & pair : response.files()) {
        (*file_map)[pair.first] = pair.second;
      }
      return grpc::OK;
    } else if (status.error_code() == 4) {
      return grpc::DEADLINE_EXCEEDED;
    } else {
      std::cout << "ERROR: " << status.error_message() << std::endl; 
      return grpc::CANCELLED;
    }
}

grpc::StatusCode DFSClientNodeP2::Stat(const std::string &filename, void* file_status) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to get the status of a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // You can start with your Part 1 implementation and add any additional
    // status details that would be useful to your solution.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::NOT_FOUND - if the file cannot be found on the server
    // StatusCode::CANCELLED otherwise
    FetchFileStat request;

    request.set_filename(filename);

    FileStatReply response;
    ClientContext ctx;

    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));
    Status status = service_stub->fileStat(&ctx, request, &response);

    if (status.ok()) {
      std::cout << "File size is: " << response.size() << std::endl;
      return grpc::OK;
    } else if (status.error_code() == 5) {
      std::cout << "ERROR: " << status.error_message() << std::endl; 
      return grpc::NOT_FOUND;
    } else if (status.error_code() == 4) {
      return grpc::DEADLINE_EXCEEDED;
    } else {
      std::cout << "ERROR: " << status.error_message() << std::endl; 
      return grpc::CANCELLED;
    }
    
}

void DFSClientNodeP2::InotifyWatcherCallback(std::function<void()> callback) {

    //
    // STUDENT INSTRUCTION:
    //
    // This method gets called each time inotify signals a change
    // to a file on the file system. That is every time a file is
    // modified or created.
    //
    // You may want to consider how this section will affect
    // concurrent actions between the inotify watcher and the
    // asynchronous callbacks associated with the server.
    //
    // The callback method shown must be called here, but you may surround it with
    // whatever structures you feel are necessary to ensure proper coordination
    // between the async and watcher threads.
    //
    // Hint: how can you prevent race conditions between this thread and
    // the async thread when a file event has been signaled?
    //

    std::lock_guard<std::mutex> lock(file_mutex);
    callback();

}

//
// STUDENT INSTRUCTION:
//
// This method handles the gRPC asynchronous callbacks from the server.
// We've provided the base structure for you, but you should review
// the hints provided in the STUDENT INSTRUCTION sections below
// in order to complete this method.
//
void DFSClientNodeP2::HandleCallbackList() {

    void* tag;

    bool ok = false;

    //
    // STUDENT INSTRUCTION:
    //
    // Add your file list synchronization code here.
    //
    // When the server responds to an asynchronous request for the CallbackList,
    // this method is called. You should then synchronize the
    // files between the server and the client based on the goals
    // described in the readme.
    //
    // In addition to synchronizing the files, you'll also need to ensure
    // that the async thread and the file watcher thread are cooperating. These
    // two threads could easily get into a race condition where both are trying
    // to write or fetch over top of each other. So, you'll need to determine
    // what type of locking/guarding is necessary to ensure the threads are
    // properly coordinated.
    //

    DIR *dir;
    struct dirent *ent;
    struct stat statBuf;

    bool mounted = false;
    // Block until the next result is available in the completion queue.
    while (completion_queue.Next(&tag, &ok)) {
        {
            //
            // STUDENT INSTRUCTION:
            //
            // Consider adding a critical section or RAII style lock here
            //

            std::lock_guard<std::mutex> lock(file_mutex);
            // The tag is the memory location of the call_data object
            AsyncClientData<FileListResponseType> *call_data = static_cast<AsyncClientData<FileListResponseType> *>(tag);

            dfs_log(LL_DEBUG2) << "Received completion queue callback";

            // Verify that the request was completed successfully. Note that "ok"
            // corresponds solely to the request for updates introduced by Finish().
            // GPR_ASSERT(ok);
            if (!ok) {
                dfs_log(LL_ERROR) << "Completion queue callback not ok.";
            }

            if (ok && call_data->status.ok()) {

                dfs_log(LL_DEBUG3) << "Handling async callback ";

                //
                // STUDENT INSTRUCTION:
                //
                // Add your handling of the asynchronous event calls here.
                // For example, based on the file listing returned from the server,
                // how should the client respond to this updated information?
                // Should it retrieve an updated version of the file?
                // Send an update to the server?
                // Do nothing?
 
                auto& timeMap = *call_data->reply.mutable_filetimemap(); 
                auto& crcMap = *call_data->reply.mutable_filecrcmap(); 

                for (auto & pair : crcMap) {
                  std::string filename = pair.first;
                  int res = access(WrapPath(filename).c_str(), R_OK);
                  // file doesn't exist in client
                  if (res < 0) {
                    this->Fetch(filename);
                  } else {
                  // file exists in client
                    std::uint32_t client_crc = dfs_file_checksum(WrapPath(filename), &this->crc_table);
                    stat(WrapPath(filename).c_str(), &statBuf);
                    if (crcMap[filename] != client_crc) {
                      if (timeMap[filename] > statBuf.st_mtime) {
                        this->Fetch(filename);
                      } else {
                        this->Store(filename);
                      }
                    }
                  }
                }

                // file exists in client but not server
                if ((dir = opendir(WrapPath("").c_str())) != NULL) {
                  while((ent = readdir(dir)) != NULL) {
                    std::string filename = ent->d_name;
                    if (crcMap.find(filename) == crcMap.end() ) {
                      // if the directory has already been mounted we know any files in client but not server should
                      // be deleted, but if it's the first mount then it's files that have not been synced yet
                      if (mounted) {
                        this->Delete(filename);
                      } else {
                        mounted = true;
                        this->Store(filename);
                      }
                    }
                  }
                }
            } else {
                dfs_log(LL_ERROR) << "Status was not ok. Will try again in " << DFS_RESET_TIMEOUT << " milliseconds.";
                dfs_log(LL_ERROR) << call_data->status.error_message();
                std::this_thread::sleep_for(std::chrono::milliseconds(DFS_RESET_TIMEOUT));
            }

            // Once we're complete, deallocate the call_data object.
            delete call_data;

            //
            // STUDENT INSTRUCTION:
            //
            // Add any additional syncing/locking mechanisms you may need here

        }


        // Start the process over and wait for the next callback response
        dfs_log(LL_DEBUG3) << "Calling InitCallbackList";
        InitCallbackList();

    }
    free(dir);
    free(ent);
}

/**
 * This method will start the callback request to the server, requesting
 * an update whenever the server sees that files have been modified.
 *
 * We're making use of a template function here, so that we can keep some
 * of the more intricate workings of the async process out of the way, and
 * give you a chance to focus more on the project's requirements.
 */
void DFSClientNodeP2::InitCallbackList() {
    CallbackList<FileRequestType, FileListResponseType>();
}

//
// STUDENT INSTRUCTION:
//
// Add any additional code you need to here
//


