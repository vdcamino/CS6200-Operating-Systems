#include <regex>
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
#include <google/protobuf/map.h>

#include "dfslib-shared-p1.h"
#include "dfslib-clientnode-p1.h"
#include "proto-src/dfs-service.grpc.pb.h"

using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;

//
// STUDENT INSTRUCTION:
//
// You may want to add aliases to your namespaced service methods here.
// All of the methods will be under the `dfs_service` namespace.
//
// For example, if you have a method named MyMethod, add
// the following:
//
//      using dfs_service::MyMethod
//
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


DFSClientNodeP1::DFSClientNodeP1() : DFSClientNode() {}

DFSClientNodeP1::~DFSClientNodeP1() noexcept {}

StatusCode DFSClientNodeP1::Store(const std::string &filename) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to store a file here. This method should
    // connect to your gRPC service implementation method
    // that can accept and store a file.
    //
    // When working with files in gRPC you'll need to stream
    // the file contents, so consider the use of gRPC's ClientWriter.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::CANCELLED otherwise
    StoreFileRequest request;
    StoreFileReply response;
    ClientContext ctx;
    long read_len, file_len, total_bytes = 0;
    read_len = 4096;
    char buffer[read_len];
    struct stat statBuf;
    std::fstream requestedFile;

    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));
    requestedFile.open(WrapPath(filename), std::ios::in | std::ios::binary);

    if (requestedFile.is_open()) {
      std::unique_ptr<ClientWriter<StoreFileRequest> > writer(service_stub->storeFile(&ctx, &response));
      stat(WrapPath(filename).c_str(), &statBuf);  
      file_len = (long) statBuf.st_size;
      if (file_len < read_len) {
        read_len = file_len;
      }

      while(total_bytes < file_len) {
        if (file_len - total_bytes <  read_len) {
          read_len = file_len - total_bytes;
        }
        requestedFile.seekg(total_bytes);
        requestedFile.read(buffer, read_len);
        request.set_size(read_len);
        request.set_filedata(buffer, read_len);
        request.set_filename(filename);
        writer->Write(request);
        total_bytes += read_len;
      }
      requestedFile.close();
      writer->WritesDone();
      Status status = writer->Finish();
      if (status.ok()) {
        return grpc::OK;
      } else {
        std::cout << "Errors: " << status.error_message() << std::endl;
        return grpc::CANCELLED;
      }
    } else {
      std::cout << "Error: " << strerror(errno) << std::endl;
      return grpc::CANCELLED;
    }
}


StatusCode DFSClientNodeP1::Fetch(const std::string &filename) {
    FetchFileRequest request;
    FetchFileReply response;

    FetchFileStat stat_request;
    FileStatReply stat_response;
    ClientContext ctx;

    std::fstream requestedFile;
    int read_len;
    long total_read = 0;
    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));

    requestedFile.open(WrapPath(filename), std::ios::out | std::ios::binary);
    if(!requestedFile.is_open()) {
      std::cout << "Error opening file" << std::endl;
    }
    request.set_filename(filename);
    stat_request.set_filename(filename);
    
    std::unique_ptr<ClientReader<FetchFileReply> > reader(service_stub->fetchFile(&ctx, request));

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

StatusCode DFSClientNodeP1::Delete(const std::string& filename) {
    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to delete a file here. Refer to the Part 1
    // student instruction for details on the basics.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::CANCELLED otherwise
    //
    //
    DeleteFileRequest request;

    DeleteFileReply response;
    ClientContext ctx;

    ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(this->deadline_timeout));
    request.set_filename(filename);
    Status status = service_stub->deleteFile(&ctx, request, &response);

    if (status.ok()) {
      return grpc::OK;
    } else if (status.error_code() == 4) {
      return grpc::DEADLINE_EXCEEDED;
    } else {
      std::cout << "ERROR: " << status.error_message() << std::endl; 
      return grpc::CANCELLED;
    }
}

StatusCode DFSClientNodeP1::List(std::map<std::string,int>* file_map, bool display) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to list all files here. This method
    // should connect to your service's list method and return
    // a list of files using the message type you created.
    //
    // The file_map parameter is a simple map of files. You should fill
    // the file_map with the list of files you receive with keys as the
    // file name and values as the modified time (mtime) of the file
    // received from the server.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::CANCELLED otherwise
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

StatusCode DFSClientNodeP1::Stat(const std::string &filename, void* file_status) {

    //
    // STUDENT INSTRUCTION:
    //
    // Add your request to get the status of a file here. This method should
    // retrieve the status of a file on the server. Note that you won't be
    // tested on this method, but you will most likely find that you need
    // a way to get the status of a file in order to synchronize later.
    //
    // The status might include data such as name, size, mtime, crc, etc.
    //
    // The file_status is left as a void* so that you can use it to pass
    // a message type that you defined. For instance, may want to use that message
    // type after calling Stat from another method.
    //
    // The StatusCode response should be:
    //
    // StatusCode::OK - if all went well
    // StatusCode::DEADLINE_EXCEEDED - if the deadline timeout occurs
    // StatusCode::NOT_FOUND - if the file cannot be found on the server
    // StatusCode::CANCELLED otherwise
    //
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
//
// STUDENT INSTRUCTION:
//
// Add your additional code here, including
// implementations of your client methods
//

