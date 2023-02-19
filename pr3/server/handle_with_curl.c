#include "gfserver.h"
#include "proxy-student.h"

#define BUFSIZE (6201)

/*
 * Replace with an implementation of handle_with_curl and any other
 * functions you may need.
 */
ssize_t handle_with_curl(gfcontext_t *ctx, const char *path, void* arg){
	(void) ctx;
	(void) path;
	(void) arg;

	char url[PATH_MAX];
	strncpy(url,arg,PATH_MAX);
	strncat(url,path,PATH_MAX);
	struct customdata chunk;
	chunk.response=malloc(1);
	chunk.ctx=ctx;
	chunk.filesize=0;
	CURL *curl;
	size_t bytes_sent=0;
	int result;
	curl = curl_easy_init();
	printf("Requesting url-->%s\n",url);
	curl_easy_setopt(curl,CURLOPT_URL,url);
	curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,write_callback);
	curl_easy_setopt(curl,CURLOPT_WRITEDATA,(void*)&chunk);
	//printf("BEFORE CALLING PERFORM\n");
	result=curl_easy_perform(curl);
	if(result == CURLE_OK)
	{
		size_t nbytes=0;
		printf("INSIDE CURL OK--->\n");
		double filelength=0;
		size_t response_code=0;
		curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code);
		curl_easy_getinfo(curl,CURLINFO_CONTENT_LENGTH_DOWNLOAD,&filelength);
		
		if(response_code == 200)
		{
				printf("SENDING HEADER FILE OK\n");
				gfs_sendheader(ctx,GF_OK,filelength);
				while(bytes_sent < chunk.filesize)
						{
							nbytes=gfs_send(ctx,chunk.response-bytes_sent,chunk.filesize);
							if(nbytes ==0)
							{
								
								break;
							}
							bytes_sent+=nbytes;
						}
			printf("FINISHED SENDING FILE %ld\n",chunk.filesize);
			
			
		}
		else if(response_code == 404)
		{
			printf("SENDING HEADER FILE NOT FOUND\n");
			
			size_t bytes=0;
			bytes= gfs_sendheader(ctx,GF_FILE_NOT_FOUND,0);
			free(chunk.response);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return bytes;
		}
		else
		{
			//for errors other than FNF and OK
			return SERVER_FAILURE;
		}
		
	}
	free(chunk.response);
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return bytes_sent;
}
// copied from https://curl.haxx.se/libcurl/c/getinmemory.html
size_t write_callback( void *data, size_t size,size_t nmemb,void* userdata)
{
	printf("Inside write callback\n");

	size_t realsize = size * nmemb;
   struct customdata *mem = (struct customdata *)userdata;
   char *ptr = realloc(mem->response, mem->filesize + realsize + 1);
   if(ptr == NULL)
     return 0;  /* out of memory! */
   mem->response = ptr;
   memcpy(&(mem->response[mem->filesize]), data, realsize);
   mem->filesize += realsize;
   mem->response[mem->filesize] = 0;
   return realsize;
}