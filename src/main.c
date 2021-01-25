/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <civetweb.h>

#include <net/tls_credentials.h>

#include "mfs.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(cv_test, LOG_LEVEL_DBG);

static int check_url_extension(const char *url, const char *ext)
{
    int len = strlen(url);
    int ext_len = strlen(ext);
    if (len < ext_len + 1)
        return -1;

    return strcmp(url + len - strlen(ext), ext);
}

static int send_file(struct mg_connection *conn, struct mfs_file *mfs_file, int chunk_size)
{
    int file_size = mfs_file->fileItem->fileSize;
    int retries;
    char *buf = mfs_file->data + mfs_file->offset;

    retries = 5;
    while (file_size > 0) {
        errno = 0;
        int sent = mg_write(conn, buf, MIN(chunk_size, file_size));
        k_sleep(K_MSEC(1)); // we need to give a chance to other threads to run
        if (sent > 0) {
            file_size -= sent;
            buf += sent;
            retries = 5;
        }
        else {
            LOG_DBG("ferr %d %d", sent, errno);
            if (retries-- <= 0) {
                LOG_ERR("file sending error");
                return -1;
            }
        }
    }

    return 0;
}

int file_handler(struct mg_connection *conn, void *cbdata)
{
    int file_size;
    const struct mg_request_info *info;
    struct mfs_file mfs_file;
    const char *cookie = mg_get_header(conn, "Cookie");
    char sessid_cookie[64];
    const char *requestFile;

    mg_get_cookie(cookie, "sessid", sessid_cookie, sizeof(sessid_cookie));

    info = mg_get_request_info(conn);
    requestFile = info->local_uri;

    // presmerovani
    if (strcmp(info->local_uri, "/") == 0)
        requestFile = "/login.html";

    file_size = mfs_open(requestFile, &mfs_file);

    if (file_size == -1) {
        LOG_ERR("cannot open %s", requestFile);
        mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
                "Connection: close\r\n\r\n");
        return 404;
    }

    mg_printf(conn, "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-length: %d\r\n", file_size);

    if (check_url_extension(requestFile, ".html") == 0)
        mg_printf(conn, "Content-Type: text/html\r\nContent-Encoding: gzip\r\n\r\n");
    else if (check_url_extension(requestFile, ".json") == 0)
        mg_printf(conn, "Content-Type: application/json\r\n\r\n");
    else if (check_url_extension(requestFile, ".js") == 0)
        mg_printf(conn, "Cache-Control: max-age=600\r\nContent-Type: text/javascript\r\nContent-Encoding: gzip\r\n\r\n");
    else if (check_url_extension(requestFile, ".css") == 0)
        mg_printf(conn, "Cache-Control: max-age=600\r\nContent-Type: text/css\r\nContent-Encoding: gzip\r\n\r\n");
    else if (check_url_extension(requestFile, ".svg") == 0)
        mg_printf(conn, "Content-Type: image/svg+xml\r\n\r\n");
    else if (check_url_extension(requestFile, ".ico") == 0)
        mg_printf(conn, "Content-Type: image/webp\r\n\r\n");
    else if (check_url_extension(requestFile, ".png") == 0)
        mg_printf(conn, "Cache-Control: max-age=600\r\nContent-Type: image/png\r\n\r\n");
    else {
        mg_printf(conn, "HTTP/1.1 404 Not Found\r\n"
                "Connection: close\r\n\r\n");
        return 404;
    }

    if (send_file(conn, &mfs_file, 1024) == 0)
        return 200;

    mg_send_http_error(conn, 500, "chyba\r");
    return 500;
}

static int field_found(const char *key,
                       const char *filename,
                       char *path,
                       size_t pathlen,
                       void *user_data)
{
    LOG_DBG("%s %s", log_strdup(key), log_strdup(filename));
    return MG_FORM_FIELD_STORAGE_GET;
}

static int field_get_fw(const char *key, const char *value, size_t valuelen, void *user_data)
{
    LOG_DBG("file chunk %d", valuelen);

    // k_yield();

    return MG_FORM_FIELD_HANDLE_GET;
}

int bigfile_handler(struct mg_connection *conn, void *cbdata)
{
    struct mg_form_data_handler fdh = {field_found, field_get_fw, NULL, NULL};


    mg_handle_form_request(conn, &fdh);

    mg_printf(conn,"HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Connection: close\r\n"
            "\r\n\r\n");
    mg_printf(conn, "{\"err\":%d}", 0);

    return 200;
}

static void *init_thread_cb(const struct mg_context *ctx, int thread_type)
{
   k_tid_t curr_thread = k_current_get();
   int new_prio;

   switch (thread_type) {
      case 0:
         new_prio = K_PRIO_PREEMPT(1);
         LOG_DBG("Master thread started, setting priotity %d", new_prio);
         k_thread_priority_set(curr_thread, new_prio);
         break;
      case 1:
         new_prio = K_PRIO_PREEMPT(1);
         LOG_DBG("Connection thread started, setting priotity %d", new_prio);
         k_thread_priority_set(curr_thread, new_prio);
         break;
      default:
         LOG_DBG("Unknown thread (%d) started", thread_type);
   }

   return NULL;
}

extern const unsigned char server_key_der[];
extern const int sizeof_server_key_der;
extern const unsigned char server_cert_der[];
extern int sizeof_server_cert_der;

static void *civetweb_init(void *arg)
{
    ARG_UNUSED(arg);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
    int err = tls_credential_add(1,
                     TLS_CREDENTIAL_SERVER_CERTIFICATE,
                     server_cert_der,
                     sizeof_server_cert_der);
    if (err < 0) {
        LOG_ERR("Failed to register public certificate: %d", err);
    }

    err = tls_credential_add(1,
                 TLS_CREDENTIAL_PRIVATE_KEY,
                 server_key_der, sizeof_server_key_der);
    if (err < 0) {
        LOG_ERR("Failed to register private key: %d", err);
    }
#endif

    static const char * const options[] = {
            "listening_ports",
            "80,443s",
            "num_threads",
            "1",
            "max_request_size",
            "4096",
            "enable_keep_alive",
            "yes",
            "keep_alive_timeout_ms",
            "1000",
            0
    };

    struct mg_callbacks callbacks;
    struct mg_context *ctx;

    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.init_thread = init_thread_cb;
    ctx = mg_start(&callbacks, 0, (const char **)options);

    if (ctx == NULL) {
        LOG_ERR("Unable to start the server.");
        return 0;
    }
    
    mg_set_request_handler(ctx, "/*$", file_handler, 0);
    mg_set_request_handler(ctx, "/bigfile$", bigfile_handler, 0);

    return 0;
}

#if defined CONFIG_MBEDTLS
static unsigned char _mbedtls_heap[128 * 1024] __attribute__((section(".mbedtlsstack")));
#endif

void main(void)
{
#if defined CONFIG_MBEDTLS
    mbedtls_memory_buffer_alloc_init(_mbedtls_heap, sizeof(_mbedtls_heap));
#endif


    LOG_DBG("Starting civetweb");
	civetweb_init(NULL);

    while (1) {
        k_sleep(K_MSEC(10));
    }
}
