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
#include <time.h>


using grpc::Status;
using grpc::Server;
using grpc::StatusCode;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::ServerContext;
using grpc::ServerBuilder;
using namespace std;
using dfs_service::DFSService;
using dfs_service::fetchRequest;
using dfs_service::fetchResponse;
using dfs_service::storeRequest;
using dfs_service::storeResponse;
using dfs_service::statRequest;
using dfs_service::statResponse;
using dfs_service::listRequest;
using dfs_service::listResponse;
using dfs_service::Item;
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

    ~DFSServiceImpl() { }
/****************************************************************/
    // IMPLEMENTATION FOR FILE FETCH CLIENT -> SERVER     
   Status fetchFile(ServerContext* context, const ::dfs_service::fetchRequest* request, ::grpc::ServerWriter< ::dfs_service::fetchResponse>* writer) override
    {
        
      const string &wrappedPath = WrapPath(request->filepath());
        std::cout<<"Inside server stub...path received "<< wrappedPath;
        struct stat filestat;
         ifstream file_obj;    
        char fileContent[1024];
        //memset(fileContent,'\0',1024);
        fetchResponse response;
        int nbytes=0;
        int bytes_read=0;
        if(stat(wrappedPath.c_str(),&filestat)<0)
        {
            // file not found
            dfs_log(LL_SYSINFO) << "DFSServerNode FILE NOT FOUND\n";
            return Status(StatusCode::NOT_FOUND, "FILE NOT FOUND");
        }
       file_obj.open(wrappedPath,ios::in); //opening the file
      int filesize = filestat.st_size;
       dfs_log(LL_SYSINFO) << "DFSServerNode Size of file-->"<<filesize<<"\n";
      try
      {
          while(bytes_read < filesize)
        {   
            if (context->IsCancelled()) 
            {
                 return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded or Client cancelled\n");
            }
            file_obj.read(fileContent,1024);
            nbytes = file_obj.gcount(); // get the no of bytes read/iteration
            bytes_read+=nbytes;   
            response.set_chunk(fileContent,nbytes);
            //std::cout<<"Contents------>\n"<<response.chunk();
            writer->Write(response);
        }
         file_obj.close();
         dfs_log(LL_SYSINFO) << "DFSClientNode Finished sending file bytes ="<<bytes_read<<"\n";
      }  
      catch(exception e)
      {
          return Status(StatusCode::INTERNAL,"Server Internal Error");
      }
            std::string msg("File fetched successfully\n");
            return Status(StatusCode::OK, msg);
       
    }
/*****************************************************/
// IMPLEMENTATION FOR STORE FILE CLIENT -> SERVER
  Status storeFile(ServerContext* context, ServerReader<::dfs_service::storeRequest>* reader, ::dfs_service::storeResponse* response) override
  {
       // Server receiving the file name metadata from client context
       // copied from https://groups.google.com/forum/#!topic/grpc-io/tNlNfYgWv7k

        std::multimap<grpc::string_ref, grpc::string_ref>::const_iterator metadata =context->client_metadata().find("filename");
        std::string path;
     
     if(metadata != context->client_metadata().end())
        {
           
            string fname((metadata->second).data(),  (metadata->second).length());
            path = fname;
            std::cout << "client metadata for filename: " << path<<"\n";

        }
    std::ofstream file_stream;
    storeRequest filedata;
    std::string wrappedPath=WrapPath(path);
    int total=0;
    try
    {
        // Server reader beginning to read the client file data
        file_stream.open(wrappedPath,std::ios::out);
        while(reader->Read(&filedata))
        {
             if (context->IsCancelled()) 
            {
                 return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded or Client cancelled\n");
            }
            const std::string &data= filedata.content();
            total+=data.size();
            file_stream.write(data.c_str(),data.size());
        }
         std::cout<<"bytes  read -->"<<total<<"\n";
                
        file_stream.close(); // closing the file stream
    }
    catch(std::exception e)
    {
        return Status(StatusCode::CANCELLED,"File storing failed");
    }
    return Status(StatusCode::OK,"File stored successfully");
  }

  /************************************************************/
  //IMPLEMENTATION FOR FILE ATTR CLIENT->SERVER
  Status getFileAttrs(ServerContext* context, const statRequest* request, statResponse* response) override
  {
      struct stat fileInfo;
    
    if (context->IsCancelled()) 
        {
                return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded or Client cancelled\n");
        }

     const std::string  wrappedPath= WrapPath(request->filename());
      if(stat(wrappedPath.c_str(),&fileInfo)!=0)
      {
          std::cout<<"FILE NOT FOUND"<<"\n";
          return Status(StatusCode::NOT_FOUND,"FILE NOT FOUND");
      }
    
   /* std::string str((std::asctime(std::localtime(&fileInfo.st_mtime))));
    std::cout<<"modified time :"<<str<<"\n";
    response->set_allocated_m_time(&str);*/
    response->set_m_time(fileInfo.st_mtime);
    response->set_c_time(fileInfo.st_ctime);
    response->set_filsize(fileInfo.st_size);
    std::cout<<"Modified time:"<<response->m_time()<<"\n";
    std::cout<<"Creation time:"<<response->m_time()<<"\n";
    return Status(StatusCode::OK,"File stat delivered ");
  }

  /***********************************************************************/
   //IMPLEMENTATION FOR FILE DELETION CLIENT->SERVER

  Status deleteFile(ServerContext* context, const fetchRequest* delRequest, storeResponse* delResponse) override
  {
     struct stat fileInfo;
    
    if (context->IsCancelled()) 
        {
                return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded or Client cancelled\n");
        }

     const std::string  wrappedPath= WrapPath(delRequest->filepath());
     delResponse->set_filename(delRequest->filepath());

      if(stat(wrappedPath.c_str(),&fileInfo)!=0)
      {
          std::cout<<"FILE NOT FOUND"<<"\n";
          return Status(StatusCode::NOT_FOUND,"FILE NOT FOUND");
      }
      // file removal
      std::cout<<"Deleting -"<<delRequest->filepath()<<"\n";
      if(std::remove(wrappedPath.c_str())!=0)
      {
          return Status(StatusCode::CANCELLED,"Unable to delete file\n");
      }
      std::cout<<"Deletion successful\n";
      return Status(StatusCode::OK,"File Successfully deleted\n");
  }

  /************************************************************************/
  // IMPLEMENTATION FOR LISTING FILES IN THE SERVER

Status listFiles(ServerContext* context, const listRequest* request,listResponse* response) override
    {
        
        // preparing to fetch the files from the mount path
        DIR *dir;
        struct dirent *ent;
        struct stat fileInfo;
        dir =opendir(mount_path.c_str());
        std::cout<<"mount_path-"<<mount_path.c_str()<<"\n";
        std::vector<string> out;
        while((ent = readdir(dir))!=NULL)
        {
            if (context->IsCancelled()) 
            {
                    return Status(StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded or Client cancelled\n");
            }
            const string filename= ent->d_name;
            
            const string filepath=WrapPath(filename);
            if(stat(filepath.c_str(),&fileInfo)==-1)continue;
            Item *item= response->add_items();
            item->set_filename(filename);
            item->set_mtime(fileInfo.st_mtime);
            std::cout<<"Filename :"<<filename<<"---"<<"m_time :"<<fileInfo.st_mtime<<"\n";
            out.push_back(ent->d_name);
        }
        closedir(dir);
        return Status(StatusCode::OK,"File listing done successfully");
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

