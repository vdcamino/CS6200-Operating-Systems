#include <map>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grpcpp/grpcpp.h>

#include "src/dfs-utils.h"
#include "dfslib-shared-p1.h"
#include "dfslib-servernode-p1.h"
#include "proto-src/dfs-service.grpc.pb.h"

using grpc::Status;
using grpc::Server;
using grpc::StatusCode;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::ServerBuilder;

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

//
// STUDENT INSTRUCTION:
//
// DFSServiceImpl is the implementation service for the rpc methods
// and message types you defined in the `dfs-service.proto` file.
//
// You should add your definition overrides here for the specific
// methods that you created for your GRPC service protocol. The
// gRPC tutorial described in the readme is a good place to get started
// when trying to understand how to implement this class.
//
// The method signatures generated can be found in `proto-src/dfs-service.grpc.pb.h` file.
//
// Look for the following section:
//
//      class Service : public ::grpc::Service {
//
// The methods returning grpc::Status are the methods you'll want to override.
//
// In C++, you'll want to use the `override` directive as well. For example,
// if you have a service method named MyMethod that takes a MyMessageType
// and a ServerWriter, you'll want to override it similar to the following:
//
//      Status MyMethod(ServerContext* context,
//                      const MyMessageType* request,
//                      ServerWriter<MySegmentType> *writer) override {
//
//          /** code implementation here **/
//      }
//
class DFSServiceImpl final : public DFSService::Service {

private:

    /** The mount path for the server **/
    std::string mount_path;

    /**
     * Prepend the mount path to the filename.
     *
     * @param filepath
     * @return
     */
    const std::string WrapPath(const std::string &filepath) {
        return this->mount_path + filepath;
    }


public:

    DFSServiceImpl(const std::string &mount_path): mount_path(mount_path) {
    }

    ~DFSServiceImpl() {}

    //
    // STUDENT INSTRUCTION:
    //
    // Add your additional code here, including
    // implementations of your protocol service methods
    //

    // STORE
    Status storeFile(
      ServerContext* ctx,
      ServerReader<StoreFileRequest>* reader, 
      StoreFileReply* response
    ) override {
      StoreFileRequest request;
      std::fstream requestedFile;
      int read_len;
      long total_read = 0;
      bool need_to_open_file = true;

      while (reader->Read(&request)) {
        if (need_to_open_file == true) {
          requestedFile.open(WrapPath(request.filename()), std::ios::out | std::ios::binary);
          if (!requestedFile.is_open()) {
            std::cout << "Error opening file" << std::endl;
          }
          need_to_open_file = false;
        }
        read_len = request.size();
        requestedFile.seekg(total_read);
        requestedFile.write(request.filedata().c_str(), read_len);
        total_read += read_len;
      }
     
      return Status(grpc::OK, "");
    }

    // FETCH
    Status fetchFile(
      ServerContext* ctx,
      const FetchFileRequest* request,
      ServerWriter<FetchFileReply>* writer
    ) override {
      std::string filename = request->filename();
      std::fstream requestedFile;

      requestedFile.open(WrapPath(filename), std::ios::in | std::ios::out | std::ios::binary);

      if (requestedFile.is_open()) {
        struct stat statBuf;
        stat(WrapPath(filename).c_str(), &statBuf);
        long file_len = (long) statBuf.st_size;
        long read_len = 4096;
        long total_bytes_sent = 0;
        char buffer[read_len];
        // make sure we're at the beginning of the filek
        if (file_len < read_len) {
          read_len = file_len;
        }

        while(total_bytes_sent < file_len) {
          if (ctx->IsCancelled()) {
            return Status(grpc::DEADLINE_EXCEEDED, "Cancelled request");
          }
          if ((file_len - total_bytes_sent) < read_len) {
            read_len = file_len - total_bytes_sent;
          }
          FetchFileReply reply;
          requestedFile.seekg(total_bytes_sent);
          requestedFile.read(buffer, read_len);
          reply.set_size(read_len);
          reply.set_filedata(buffer, read_len);
          total_bytes_sent += read_len;
          writer->Write(reply);
        }

        requestedFile.close();
        return Status(grpc::OK, "");
      } else {
        std::cout << "Error: " << strerror(errno) << std::endl;
        return Status(grpc::NOT_FOUND, "File could not be found");
      }
    }

    // DELETE
    Status deleteFile (
      ServerContext* ctx,
      const DeleteFileRequest* request,
      DeleteFileReply* reply
    ) override {
      std::string filename = request->filename();
      struct stat statBuf;

      int openStatus = stat(WrapPath(filename).c_str(), &statBuf);

      if (openStatus == -1) {
        return Status(grpc::NOT_FOUND, "This file does not exist");
      } else {
        int deleteStatus = remove(WrapPath(filename).c_str());
        if (deleteStatus == -1) {
          return Status(grpc::UNKNOWN, "Could not delete the file");
        } else {
          return Status(grpc::OK, "");
        }
      }
    };


    // LIST
    Status listFiles (
      ServerContext* ctx,
      const ListFilesRequest* request,
      ListFilesReply* reply
    ) override {
      DIR *dir;
      struct dirent *ent;
      std::map<std::string,int> fileMap;
      struct stat statBuf;


      if ((dir = opendir(WrapPath("").c_str())) != NULL) {
        while((ent = readdir(dir)) != NULL) {
          stat(WrapPath(ent->d_name).c_str(), &statBuf);
          (*reply->mutable_files())[ent->d_name] = statBuf.st_mtime;
        }
      }

      free(dir);
      free(ent);
      return Status(grpc::OK, "");
    };

    // STAT
    Status fileStat(
        ServerContext* ctx,
        const FetchFileStat* request,
        FileStatReply* reply
    ) override {
      std::string filename = request->filename();
      struct stat statBuf;

      int openStatus = stat(WrapPath(filename).c_str(), &statBuf);

      if (openStatus == -1) {
        reply->set_size(-1);
        return Status(grpc::NOT_FOUND, "This file does not exist");
      } else {
        reply->set_size(statBuf.st_size);

        return Status(grpc::OK, "");
      }
    }
};

//
// STUDENT INSTRUCTION:
//
// The following three methods are part of the basic DFSServerNode
// structure. You may add additional methods or change these slightly,
// but be aware that the testing environment is expecting these three
// methods as-is.
//
/**
 * The main server node constructor
 *
 * @param server_address
 * @param mount_path
 */
DFSServerNode::DFSServerNode(const std::string &server_address,
        const std::string &mount_path,
        std::function<void()> callback) :
    server_address(server_address), mount_path(mount_path), grader_callback(callback) {}

/**
 * Server shutdown
 */
DFSServerNode::~DFSServerNode() noexcept {
    dfs_log(LL_SYSINFO) << "DFSServerNode shutting down";
    this->server->Shutdown();
}

/** Server start **/
void DFSServerNode::Start() {
    DFSServiceImpl service(this->mount_path);
    ServerBuilder builder;
    builder.AddListeningPort(this->server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    this->server = builder.BuildAndStart();
    dfs_log(LL_SYSINFO) << "DFSServerNode server listening on " << this->server_address;
    this->server->Wait();
}

//
// STUDENT INSTRUCTION:
//
// Add your additional DFSServerNode definitions here
//

