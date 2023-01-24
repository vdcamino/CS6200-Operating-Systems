#include <map>
#include <mutex>
#include <shared_mutex>
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

#include "proto-src/dfs-service.grpc.pb.h"
#include "src/dfslibx-call-data.h"
#include "src/dfslibx-service-runner.h"
#include "dfslib-shared-p2.h"
#include "dfslib-servernode-p2.h"

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

using dfs_service::WriteLockRequest;
using dfs_service::WriteLockReply;

using dfs_service::CallbackListRequest;
using dfs_service::CallbackListReply;

//
// STUDENT INSTRUCTION:
//
// Change these "using" aliases to the specific
// message types you are using in your `dfs-service.proto` file
// to indicate a file request and a listing of files from the server
//
using FileRequestType = dfs_service::CallbackListRequest;
using FileListResponseType = dfs_service::CallbackListReply;

extern dfs_log_level_e DFS_LOG_LEVEL;

//
// STUDENT INSTRUCTION:
//
// As with Part 1, the DFSServiceImpl is the implementation service for the rpc methods
// and message types you defined in your `dfs-service.proto` file.
//
// You may start with your Part 1 implementations of each service method.
//
// Elements to consider for Part 2:
//
// - How will you implement the write lock at the server level?
// - How will you keep track of which client has a write lock for a file?
//      - Note that we've provided a preset client_id in DFSClientNode that generates
//        a client id for you. You can pass that to the server to identify the current client.
// - How will you release the write lock?
// - How will you handle a store request for a client that doesn't have a write lock?
// - When matching files to determine similarity, you should use the `file_checksum` method we've provided.
//      - Both the client and server have a pre-made `crc_table` variable to speed things up.
//      - Use the `file_checksum` method to compare two files, similar to the following:
//
//          std::uint32_t server_crc = dfs_file_checksum(filepath, &this->crc_table);
//
//      - Hint: as the crc checksum is a simple integer, you can pass it around inside your message types.
//
class DFSServiceImpl final :
    public DFSService::WithAsyncMethod_CallbackList<DFSService::Service>,
        public DFSCallDataManager<FileRequestType , FileListResponseType> {

private:

    /** The runner service used to start the service and manage asynchronicity **/
    DFSServiceRunner<FileRequestType, FileListResponseType> runner;

    /** The mount path for the server **/
    std::string mount_path;

    /** Mutex for managing the queue requests **/
    std::mutex queue_mutex;
    
    std::mutex lock_map_mutex;

    /** The vector of queued tags used to manage asynchronous requests **/
    std::vector<QueueRequest<FileRequestType, FileListResponseType>> queued_tags;

    std::map<std::string, std::string> lockMap;
    /**
     * Prepend the mount path to the filename.
     *
     * @param filepath
     * @return
     */
    const std::string WrapPath(const std::string &filepath) {
        return this->mount_path + filepath;
    }

    /** CRC Table kept in memory for faster calculations **/
    CRC::Table<std::uint32_t, 32> crc_table;

public:

    DFSServiceImpl(const std::string& mount_path, const std::string& server_address, int num_async_threads):
        mount_path(mount_path), crc_table(CRC::CRC_32()) {

        this->runner.SetService(this);
        this->runner.SetAddress(server_address);
        this->runner.SetNumThreads(num_async_threads);
        this->runner.SetQueuedRequestsCallback([&]{ this->ProcessQueuedRequests(); });

    }

    ~DFSServiceImpl() {
        this->runner.Shutdown();
    }

    void Run() {
        this->runner.Run();
    }

    /**
     * Request callback for asynchronous requests
     *
     * This method is called by the DFSCallData class during
     * an asynchronous request call from the client.
     *
     * Students should not need to adjust this.
     *
     * @param context
     * @param request
     * @param response
     * @param cq
     * @param tag
     */
    void RequestCallback(grpc::ServerContext* context,
                         FileRequestType* request,
                         grpc::ServerAsyncResponseWriter<FileListResponseType>* response,
                         grpc::ServerCompletionQueue* cq,
                         void* tag) {

        std::lock_guard<std::mutex> lock(queue_mutex);
        this->queued_tags.emplace_back(context, request, response, cq, tag);

    }

    /**
     * Process a callback request
     *
     * This method is called by the DFSCallData class when
     * a requested callback can be processed. You should use this method
     * to manage the CallbackList RPC call and respond as needed.
     *
     * See the STUDENT INSTRUCTION for more details.
     *
     * @param context
     * @param request
     * @param response
     */
    void ProcessCallback(ServerContext* context, FileRequestType* request, FileListResponseType* response) {

        //
        // STUDENT INSTRUCTION:
        //
        // You should add your code here to respond to any CallbackList requests from a client.
        // This function is called each time an asynchronous request is made from the client.
        //
        // The client should receive a list of files or modifications that represent the changes this service
        // is aware of. The client will then need to make the appropriate calls based on those changes.
        //

      DIR *dir;
      struct dirent *ent;
      struct stat statBuf;


      if ((dir = opendir(WrapPath("").c_str())) != NULL) {
        while((ent = readdir(dir)) != NULL) {
          stat(WrapPath(ent->d_name).c_str(), &statBuf);
          (*response->mutable_filetimemap())[ent->d_name] = statBuf.st_mtime;
          (*response->mutable_filecrcmap())[ent->d_name] = dfs_file_checksum(WrapPath(ent->d_name), &this->crc_table);
        }
      }

      free(dir);
      free(ent);
    }

    /**
     * Processes the queued requests in the queue thread
     */
    void ProcessQueuedRequests() {
        while(true) {

            //
            // STUDENT INSTRUCTION:
            //
            // You should add any synchronization mechanisms you may need here in
            // addition to the queue management. For example, modified files checks.
            //
            // Note: you will need to leave the basic queue structure as-is, but you
            // may add any additional code you feel is necessary.
            //


            // Guarded section for queue
            {
                dfs_log(LL_DEBUG2) << "Waiting for queue guard";
                std::lock_guard<std::mutex> lock(queue_mutex);


                for(QueueRequest<FileRequestType, FileListResponseType>& queue_request : this->queued_tags) {
                    this->RequestCallbackList(queue_request.context, queue_request.request,
                        queue_request.response, queue_request.cq, queue_request.cq, queue_request.tag);
                    queue_request.finished = true;
                }

                // any finished tags first
                this->queued_tags.erase(std::remove_if(
                    this->queued_tags.begin(),
                    this->queued_tags.end(),
                    [](QueueRequest<FileRequestType, FileListResponseType>& queue_request) { return queue_request.finished; }
                ), this->queued_tags.end());

            }
        }
    }

    //
    // STUDENT INSTRUCTION:
    //
    // Add your additional code here, including
    // the implementations of your rpc protocol methods.
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
      bool first_request = true;

      while (reader->Read(&request)) {
        const std::string filename = request.filename();
        if (first_request) { 
          const std::string client_id = request.client_id();

          Status lock_status = this->checkForLock(client_id, filename); 
          if (!lock_status.ok()) {
            std::cout << "File in use in store" << std::endl;
            return Status(grpc::RESOURCE_EXHAUSTED, "File is locked");
          }

          std::uint32_t server_crc = dfs_file_checksum(WrapPath(filename), &this->crc_table);
          if (request.crc() == server_crc) {
            std::cout << "File already exists on the server" << std::endl;
            this->releaseLock(filename);
            return Status(grpc::ALREADY_EXISTS, "File already exists on the server");
          }

          requestedFile.open(WrapPath(filename), std::ios::out | std::ios::binary);
          if (!requestedFile.is_open()) {
            this->releaseLock(filename);
            std::cout << "Error opening file" << std::endl;
            return Status(grpc::CANCELLED, "File failed to open");
          }
          first_request = false;
        }
        read_len = request.size();
        requestedFile.seekg(total_read);
        requestedFile.write(request.filedata().c_str(), read_len);
        total_read += read_len;
      }
      requestedFile.close();
      this->releaseLock(request.filename());
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
        // make sure we're at the beginning of the file
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
      const std::string client_id = request->client_id();
      struct stat statBuf;

      Status lock_status = this->checkForLock(client_id, filename); 
      if (!lock_status.ok()) {
        std::cout << "File in use in store" << std::endl;
        this->releaseLock(filename);
        return Status(grpc::RESOURCE_EXHAUSTED, "File is locked");
      } else {
        int openStatus = stat(WrapPath(filename).c_str(), &statBuf);

        if (openStatus == -1) {
          this->releaseLock(filename);
          return Status(grpc::NOT_FOUND, "This file does not exist");
        } else {
          int deleteStatus = remove(WrapPath(filename).c_str());
          this->releaseLock(filename);
          if (deleteStatus == -1) {
            return Status(grpc::UNKNOWN, "Could not delete the file");
          } else {
            return Status(grpc::OK, "");
          }
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
        std::uint32_t server_crc = dfs_file_checksum(WrapPath(filename), &this->crc_table);
        reply->set_size(statBuf.st_size);
        reply->set_crc(server_crc);
        reply->set_mtime(server_crc);

        return Status(grpc::OK, "");
      }
    }

    Status acquireWriteLock(
        ServerContext* ctx,
        const WriteLockRequest* request,
        WriteLockReply* response
    ) override {
      const std::string client_id = request->client_id();
      const std::string filename = request->filename();
      return this->checkForLock(client_id, filename); 
    }

    Status checkForLock(const std::string client_id, const std::string filename) {

      std::lock_guard<std::mutex> lock(lock_map_mutex);
      if (this->writeUnlocked(filename)) {
        this->setWriteLock(client_id, filename);
        return Status(grpc::OK, "");
      } else if (this->lockedByOtherClient(client_id, filename)) { 
        return Status(grpc::RESOURCE_EXHAUSTED, "File is already locked");
      } else {
        // locked by the current client
        return Status(grpc::OK, "you currently have the lock");
      }
    }

    void setWriteLock(const std::string &client_id, const std::string &filename) {
      lockMap[filename] = client_id;
    }

    void releaseLock(const std::string &filename) {
      lockMap[filename] = "";
    }

    bool writeUnlocked(const std::string &filename) {
      return (lockMap[filename] == "");
    }

    bool lockedByOtherClient(const std::string &client_id, const std::string &filename) {
      return lockMap[filename] != client_id;
    }
};

//
// STUDENT INSTRUCTION:
//
// The following three methods are part of the basic DFSServerNode
// structure. You may add additional methods or change these slightly
// to add additional startup/shutdown routines inside, but be aware that
// the basic structure should stay the same as the testing environment
// will be expected this structure.
//
/**
 * The main server node constructor
 *
 * @param mount_path
 */
DFSServerNode::DFSServerNode(const std::string &server_address,
        const std::string &mount_path,
        int num_async_threads,
        std::function<void()> callback) :
        server_address(server_address),
        mount_path(mount_path),
        num_async_threads(num_async_threads),
        grader_callback(callback) {}
/**
 * Server shutdown
 */
DFSServerNode::~DFSServerNode() noexcept {
    dfs_log(LL_SYSINFO) << "DFSServerNode shutting down";
}

/**
 * Start the DFSServerNode server
 */
void DFSServerNode::Start() {
    DFSServiceImpl service(this->mount_path, this->server_address, this->num_async_threads);


    dfs_log(LL_SYSINFO) << "DFSServerNode server listening on " << this->server_address;
    service.Run();
}

//
// STUDENT INSTRUCTION:
//
// Add your additional definitions here
//
