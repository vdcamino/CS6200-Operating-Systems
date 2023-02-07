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

#include "dfslib-shared-p1.h"
#include "dfslib-clientnode-p1.h"
#include "proto-src/dfs-service.grpc.pb.h"

using grpc::Status;
using grpc::Channel;
using grpc::StatusCode;
using grpc::ClientWriter;
using grpc::ClientReader;
using grpc::ClientContext;
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


DFSClientNodeP1::DFSClientNodeP1() : DFSClientNode() {}

DFSClientNodeP1::~DFSClientNodeP1() noexcept {}

//IMPLEMENTATION FOR FILE STORE CLIENT -> SERVER

StatusCode DFSClientNodeP1::Store(const std::string &filename) 
{
    const string &wrappedPath = WrapPath(filename);
    struct stat filestat;
    ifstream file_obj;    
    char fileContent[1024];
    ClientContext context;
    int nbytes=0;
    int bytes_read=0;
    storeResponse response;
    storeRequest request;
    if(stat(wrappedPath.c_str(),&filestat)<0)
        {
            // file not found
            dfs_log(LL_SYSINFO) << "DFSClientNode FILE NOT FOUND\n";
            return StatusCode::NOT_FOUND;
        }
    // setting the file name metadata 
    context.AddMetadata("filename",filename);
    context.set_deadline(std::chrono::system_clock::now()+std::chrono::milliseconds(deadline_timeout));    
    file_obj.open(wrappedPath,ios::in); //opening the file
    int filesize = filestat.st_size;
   
    dfs_log(LL_SYSINFO) << "DFSclient Size of file-->"<<filesize<<"\n";
    // getting the client writer obj from the stub to write the file data
    
    std::unique_ptr<ClientWriter<storeRequest>> writer=service_stub->storeFile(&context,&response);
    try
    {
         while(bytes_read < filesize)
        {   
            file_obj.read(fileContent,1024);
            nbytes = file_obj.gcount(); // get the no of bytes read/iteration
            bytes_read+=nbytes;   
           request.set_content(fileContent,nbytes);
           writer->Write(request);
           
        }
         file_obj.close();
          dfs_log(LL_SYSINFO) << "DFSServerNode Finished sending file bytes ="<<bytes_read<<"\n";
    }
    catch(std::exception e)
    {
        std::cout<<"Status : CANCELLED\n";
        return StatusCode::CANCELLED;
    }
    writer->WritesDone();
    Status status=writer->Finish();
    if(!status.ok())
    {
            return status.error_code();
        
    }
    return StatusCode::OK;
}

//IMPLEMENTATION FOR FILE FETCH CLIENT->SERVER

StatusCode DFSClientNodeP1::Fetch(const std::string &filename)
 {

    std::cout<<"Preparing to send fetch request--->\n";
    ClientContext context;
    fetchRequest request;
    fetchResponse filedata;
    std::ofstream file_stream;
    const std::string &path=WrapPath(filename);
   
    // setting the deadline for client request
    using namespace std::chrono;  
    context.set_deadline(system_clock::now()+milliseconds(deadline_timeout));    
    request.set_filepath(filename);
    
    std::unique_ptr<ClientReader<fetchResponse>> reader=service_stub->fetchFile(&context,request);
     dfs_log(LL_SYSINFO) << "Client Req sent -->\n ";
      int total=0;
        try
      { 
         while(reader->Read(&filedata))
        {
            if(!file_stream.is_open())
            {
                file_stream.open(path,std::ios::out); // open file to write data, trunc to clear the previous data
            }
            const std::string &data= filedata.chunk();
            total+=data.size();
            file_stream.write(data.c_str(),data.size());
            
        }
        
            std::cout<<"bytes  read -->"<<total<<"\n";
                
        file_stream.close(); // closing the file stream
    }  
    catch(std::exception e)
    {
        return StatusCode::CANCELLED;
    }
    Status status = reader->Finish();   // finish the rpc call and get the status
    if(!status.ok())
    {
       if(status.error_code()==StatusCode::INTERNAL)
       {
           std::cout<<"Status : CANCELLED\n";
           return StatusCode::CANCELLED;
       }
        else 
        {
            std::cout<<"Status : FILE_NOT_FOUND\n";
            return status.error_code();
        }
    }
    std::cout<<"FILE FETCH SUCCESSFUL\n";
    return StatusCode::OK;
}

// IMPLEMENTATION FOR FILE DELETION CLIENT->SERVER
StatusCode DFSClientNodeP1::Delete(const std::string& filename) {


    fetchRequest delRequest;
    storeResponse delResponse;
    ClientContext context;
    delRequest.set_filepath(filename);
     using namespace std::chrono;  
    context.set_deadline(system_clock::now()+milliseconds(deadline_timeout));
    std::cout<<"Delete file :"<<filename<<"\n";
    Status status = service_stub->deleteFile(&context,delRequest,&delResponse);

    if(!status.ok())
    {
        if(status.error_code() == StatusCode::NOT_FOUND)
        {
             std::cout<<"Status : FILE_NOT_FOUND\n";
            return status.error_code();
        }
        return status.error_code();
    }
    std::cout<<"Deletion successful"<<"\n";
    return StatusCode::OK;
}

// IMPLEMENTATION FOR LISTING FILES IN THE SERVER

StatusCode DFSClientNodeP1::List(std::map<std::string,int>* file_map, bool display) {
   
    ClientContext context;
    using namespace std::chrono;
    context.set_deadline(system_clock::now()+milliseconds(deadline_timeout)); 

    listRequest request;
    listResponse response;
    
    Status status = service_stub->listFiles(&context,request,&response);

    if(!status.ok())
    {
        return status.error_code();
    }
    // Iterating through the response array and populating the map with filename & mtime
    for(const Item &item:response.items())
    {
        std::cout<<"Item :"<<item.filename()<<"---"<<"mtime :"<<item.mtime()<<"\n";
        file_map->insert({item.filename(),item.mtime()});
    }
    std::cout<<"File listing successful\n";
    return StatusCode::OK;
}

//IMPLEMENTATION FOR FILE ATTR CLIENT->SERVER

StatusCode DFSClientNodeP1::Stat(const std::string &filename, void* file_status) {

    ClientContext context;
    using namespace std::chrono;
    context.set_deadline(system_clock::now()+milliseconds(deadline_timeout));    

    statRequest request;
    statResponse response;
    
    request.set_filename(filename);

   std::cout<<"Requesting for stat of "<<filename<<"\n";
  Status status=service_stub->getFileAttrs(&context,request,&response);
  
  if(!status.ok())
  {
     std::cout<<"Error code :"<<status.error_code()<<"\n";
      return status.error_code();
  }
   file_status =&response;
     std::cout<<"File size ="<<response.filsize()<<"\n";
  std::cout<<"File m_time ="<<response.m_time()<<"\n";
  std::cout<<"File c_time ="<<response.c_time()<<"\n";
   return StatusCode::OK;
}

