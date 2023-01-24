

#include "gfserver.h"
#include "proxy-student.h"

#define BUFSIZE (630)

// Function handler to handle the Header received from Curl
size_t get_header(char *buffer, size_t itemsize, size_t nitems, void *ignorethis)
{
	size_t bytes = itemsize * nitems;
	return bytes;
}

// Function handler to handle the Data received from Curl
size_t send_data(char *buffer, size_t itemsize, size_t nitems, void *ctx)
{
	size_t bytes = itemsize * nitems;
	return gfs_send(ctx, buffer, bytes);
}

/*
 * Replace with an implementation of handle_with_curl and any other
 * functions you may need.
 */
ssize_t handle_with_curl(gfcontext_t *ctx, const char *path, void *arg)
{
	// Getting the Request URL which is Server+Path
	int requestUrlSize = strlen((char *)arg) + strlen(path) + 1;
	char *requestURL = (char *)malloc(requestUrlSize);
	strcpy(requestURL, (char *)arg);
	strcat(requestURL, path);

	// Variables
	double content_length;
	int response_code;

	////////////////////// HEADER START //////////////////////
	// Initializing the Libcurl library
	CURL *curl = curl_easy_init();

	if (!curl) // Check if Initializing failed
	{
		free(requestURL);
		curl_easy_cleanup(curl);
		perror("Libcurl initialization failed: ");
		return SERVER_FAILURE;
	}
	// Set options for getting Header
	curl_easy_setopt(curl, CURLOPT_URL, requestURL);
	curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &get_header);

	CURLcode result = curl_easy_perform(curl);
	if (result != CURLE_OK) // Check if request failed
	{
		// Error case sending GF_ERROR and cleaning up resources
		gfs_sendheader(ctx, GF_ERROR, 0);
		free(requestURL);
		curl_easy_cleanup(curl);
		perror("Request failed: ");
		return SERVER_FAILURE;
	}
	// Get Content-Length from Header response of libcurl
	result = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
	// Get Response Code from Header response of libcurl
	result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	if (content_length <= 0.0 && (response_code >= 400 && response_code < 500))
	{
		gfs_sendheader(ctx, GF_FILE_NOT_FOUND, (size_t)0.0);
		free(requestURL);
		curl_easy_cleanup(curl);
		return SERVER_FAILURE;
	}
	else if (content_length <= 0.0 && response_code >= 500)
	{
		// Error case sending GF_ERROR and cleaning up resources
		gfs_sendheader(ctx, GF_ERROR, 0);
		free(requestURL);
		curl_easy_cleanup(curl);
		perror("Request failed: ");
		return SERVER_FAILURE;
	}
	else
	{
		gfs_sendheader(ctx, GF_OK, content_length);
	}
	//////////////////// HEADER END //////////////////////

	////////////////////// BODY START //////////////////////
	// Initializing the Libcurl library
	curl = curl_easy_init();

	if (!curl) // Check if Initializing failed
	{
		free(requestURL);
		curl_easy_cleanup(curl);
		perror("Libcurl initialization failed: ");
		return SERVER_FAILURE;
	}

	// Set options for sending Data
	curl_easy_setopt(curl, CURLOPT_URL, requestURL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &send_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, ctx);

	result = curl_easy_perform(curl);
	if (result != CURLE_OK) // Check if request failed
	{
		// Error case cleaning up resources
		free(requestURL);
		curl_easy_cleanup(curl);
		perror("Sending data failed: ");
		return SERVER_FAILURE;
	}
	////////////////////// BODY END //////////////////////

	// Cleanup
	free(requestURL);
	curl_easy_cleanup(curl);
	return (size_t)content_length;
}

/*
 * We provide a dummy version of handle_with_file that invokes handle_with_curl
 * as a convenience for linking.  We recommend you simply modify the proxy to
 * call handle_with_curl directly.
 */
// ssize_t handle_with_file(gfcontext_t *ctx, const char *path, void* arg){
// 	return handle_with_curl(ctx, path, arg);
// }
