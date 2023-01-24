

#include "gfserver.h"
#include "proxy-student.h"
#include "curl/curl.h"

#define BUFSIZE (8803)


typedef struct data_holder {
    char *data;
    size_t data_len;
} data_holder;

size_t write_memory_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {

    data_holder *dh = (data_holder *) userdata;

    size_t data_size = size * nmemb;
    size_t new_len = dh->data_len + data_size + 1;// new size of the buffer (including space for null ternminator)

    dh->data = realloc(dh->data, new_len);  // expand buffer to create extra space for data
    if (!dh->data) {
        perror("Failed to reallocate data_holder");
        return 0;
    }

    char *offset = dh->data + dh->data_len;
    memcpy((void *) offset, ptr, data_size);
    size_t l = new_len - 1;
    dh->data_len = l;
    // null terminate buffer
    dh->data[l] = 0;

    // return the number of bytes dealt with
    return data_size;
}

/*
 * Replace with an implementation of handle_with_curl and any other
 * functions you may need.
 */
ssize_t handle_with_curl(gfcontext_t *ctx, const char *path, void *arg) {

    char buffer[4096];
    char *data_dir = arg;

    data_holder dh;
    dh.data = malloc(1);// todo: free this memory
    dh.data_len = 0;

    strcpy(buffer, data_dir);
    strcat(buffer, path);

    CURL *curl = curl_easy_init();
    if (!curl) {
        perror("FAILED TO INITIALIZE CURL EASY HANDLE");
        return EXIT_FAILURE;
    }

    curl_easy_setopt(curl, CURLOPT_URL, buffer);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &dh);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
    CURLcode return_code = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (return_code == CURLE_OK) {
        fprintf(stdout, "SERVER RETURNED OK. RECIEVED %lu BYTES\n", dh.data_len);
        gfs_sendheader(ctx, GF_OK, dh.data_len);
        // send data to client
        ssize_t total_bytes_written = 0;
        ssize_t bytes_rem;

        while (total_bytes_written < dh.data_len) {
            bytes_rem = dh.data_len - total_bytes_written;
            ssize_t bytes_to_write = (BUFSIZE > bytes_rem) ? bytes_rem : BUFSIZE;
            char *offset_ptr = dh.data + total_bytes_written;
            ssize_t len = gfs_send(ctx, offset_ptr, (size_t) bytes_to_write);
            if (len != bytes_to_write) {
                perror("Failed to write data to client");
                return EXIT_FAILURE;
            }
            total_bytes_written += len;
        }
        // write complete, free memory
        free(dh.data);
        return dh.data_len;
    } else if (return_code == CURLE_HTTP_RETURNED_ERROR) {
        free(dh.data);
        return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
    } else {
        free(dh.data);
        return EXIT_FAILURE;
    }

}

/*
 * We provide a dummy version of handle_with_file that invokes handle_with_curl
 * as a convenience for linking.  We recommend you simply modify the proxy to
 * call handle_with_curl directly.
 */
ssize_t handle_with_file(gfcontext_t *ctx, const char *path, void *arg) {
    return handle_with_curl(ctx, path, arg);
}	
