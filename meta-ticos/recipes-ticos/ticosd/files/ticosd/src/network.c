//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! Network POST & GET API wrapper around libCURL
//!

#include "network.h"

#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ticos/util/json-c.h"
#include "ticos/util/string.h"
#include "ticosd.h"

struct TicosdNetwork {
  sTicosd *ticosd;
  bool during_network_failure;
  CURL *curl;
  const char *base_url;
  char *project_key_header;
  const char *software_type;
  const char *software_version;
};

struct _write_callback {
  char *buf;
  size_t size;
};

/**
 * @brief libCURL write callback
 *
 * @param contents Data received
 * @param size Size of buffer received
 * @param nmemb Number of elements in buffer
 * @param userp Write callback context
 * @return size_t Bytes processed
 */
static size_t prv_network_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  struct _write_callback *recv_buf = userp;
  size_t total_size = (size * nmemb);

  if (!recv_buf->buf) {
    return total_size;
  }

  char *ptr = realloc(recv_buf->buf, recv_buf->size + total_size + 1);
  if (!ptr) {
    fprintf(stderr, "network:: Failed to allocate memory for network GET.\n");
    return 0;
  }

  recv_buf->buf = ptr;
  memcpy(&(recv_buf->buf[recv_buf->size]), contents, total_size);
  recv_buf->size += total_size;
  recv_buf->buf[recv_buf->size] = 0;

  return total_size;
}

static void prv_log_first_failed_request(sTicosdNetwork *handle, const char *fmt, ...) {
  if (!handle->during_network_failure) {
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    handle->during_network_failure = true;
  }
}

static void prv_log_first_succeeded_request(sTicosdNetwork *handle, const char *fmt, ...) {
  if (handle->during_network_failure) {
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    handle->during_network_failure = false;
  }
}

static ticosdNetworkResult prv_check_error(sTicosdNetwork *handle, const CURLcode res,
                                               const char *method, const char *url) {
  long http_code = 0;
  curl_easy_getinfo(handle->curl, CURLINFO_HTTP_CODE, &http_code);

  // Note: assuming NOT using CURLOPT_FAILONERROR!
  if (res != CURLE_OK) {
    prv_log_first_failed_request(
      handle, "network:: Failed to perform %s request to %s, %d: %s (HTTP code %ld).\n", method,
      url, res, curl_easy_strerror(res), http_code);
    return kTicosdNetworkResult_ErrorRetryLater;
  }

  prv_log_first_succeeded_request(
    handle,
    "network:: Network recovered, successfully performed %s request to %s (HTTP code %ld).\n",
    method, url, http_code);

  if (http_code >= 400 && http_code <= 499) {
    // Client error:
    fprintf(stderr, "network:: client error for %s request to %s (HTTP code %ld).\n", method, url,
            http_code);
    return kTicosdNetworkResult_ErrorNoRetry;
  } else if (http_code >= 500 && http_code <= 599) {
    // Server error:
    fprintf(stderr, "network:: server error for %s request to %s (HTTP code %ld).\n", method, url,
            http_code);
    return kTicosdNetworkResult_ErrorRetryLater;
  }
  return kTicosdNetworkResult_OK;
}

static char *prv_create_url(sTicosdNetwork *handle, const char *endpoint) {
  char *url;
  if (ticos_asprintf(&url, "%s%s", handle->base_url, endpoint) == -1) {
    return NULL;
  }
  return url;
}

static bool prv_parse_file_upload_prepare_response(const char *recvdata, char **upload_url,
                                                   char **upload_token) {
  json_object *payload_object = NULL;
  *upload_url = NULL;
  *upload_token = NULL;

  if (!(payload_object = json_tokener_parse(recvdata))) {
    fprintf(stderr, "network:: Failed to parse file upload request response\n");
    goto cleanup;
  }

  json_object *data_object;
  if (!json_object_object_get_ex(payload_object, "data", &data_object) ||
      json_object_get_type(data_object) != json_type_object) {
    fprintf(stderr, "network:: File upload request response missing 'data'\n");
    goto cleanup;
  }

  json_object *object;
  if (!json_object_object_get_ex(data_object, "upload_url", &object) ||
      json_object_get_type(object) != json_type_string) {
    fprintf(stderr, "network:: File upload request response missing 'upload_url'\n");
    goto cleanup;
  }
  *upload_url = strdup(json_object_get_string(object));

  if (!json_object_object_get_ex(data_object, "token", &object) ||
      json_object_get_type(object) != json_type_string) {
    fprintf(stderr, "network:: File upload request response missing 'token'\n");
    goto cleanup;
  }
  *upload_token = strdup(json_object_get_string(object));

  json_object_put(payload_object);
  return true;

cleanup:
  free(*upload_url);
  free(*upload_token);
  json_object_put(payload_object);
  return false;
}

static struct json_object *prv_prepare_request_body(const sTicosdNetwork *handle,
                                                    const size_t filesize,
                                                    const char *content_encoding) {
  const sTicosdDeviceSettings *settings = ticosd_get_device_settings(handle->ticosd);
  struct json_object *request_body = json_object_new_object();
  if (request_body == NULL) {
    goto fail;
  }
  if (content_encoding != NULL) {
    if (json_object_object_add_ex(request_body, "content_encoding",
                                  json_object_new_string(content_encoding),
                                  JSON_C_OBJECT_ADD_CONSTANT_KEY) != 0) {
      goto fail;
    }
  }
  if (json_object_object_add_ex(request_body, "kind", json_object_new_string("ELF_COREDUMP"),
                                JSON_C_OBJECT_ADD_CONSTANT_KEY) != 0) {
    goto fail;
  }
  if (json_object_object_add_ex(request_body, "size", json_object_new_int64((int64_t)filesize),
                                JSON_C_OBJECT_ADD_CONSTANT_KEY) != 0) {
    goto fail;
  }
  struct json_object *device = json_object_new_object();
  if (device == NULL) {
    goto fail;
  }
  if (json_object_object_add_ex(request_body, "device", device, JSON_C_OBJECT_ADD_CONSTANT_KEY) !=
      0) {
    json_object_put(device);
    goto fail;
  }
  if (json_object_object_add_ex(device, "device_serial",
                                json_object_new_string(settings->device_id),
                                JSON_C_OBJECT_ADD_CONSTANT_KEY) != 0) {
    goto fail;
  }
  if (json_object_object_add_ex(device, "hardware_version",
                                json_object_new_string(settings->hardware_version),
                                JSON_C_OBJECT_ADD_CONSTANT_KEY) != 0) {
    goto fail;
  }
  if (json_object_object_add_ex(device, "software_version",
                                json_object_new_string(handle->software_version),
                                JSON_C_OBJECT_ADD_CONSTANT_KEY) != 0) {
    goto fail;
  }
  if (json_object_object_add_ex(device, "software_type",
                                json_object_new_string(handle->software_type),
                                JSON_C_OBJECT_ADD_CONSTANT_KEY) != 0) {
    goto fail;
  }
  return request_body;

fail:
  json_object_put(request_body);
  fprintf(stderr, "network:: failed to create prepared upload request body\n");
  return NULL;
}

static ticosdNetworkResult prv_file_upload_prepare(sTicosdNetwork *handle,
                                                       const char *endpoint, const size_t filesize,
                                                       char **upload_url, char **upload_token,
                                                       bool is_gzipped) {
  char *recvdata = NULL;
  size_t recvlen;
  ticosdNetworkResult rc;

  const char *content_encoding = is_gzipped ? "gzip" : NULL;
  struct json_object *request_body = prv_prepare_request_body(handle, filesize, content_encoding);
  if (request_body == NULL) {
    rc = kTicosdNetworkResult_ErrorRetryLater;
    goto cleanup;
  }

  rc = ticosd_network_post(handle, endpoint, kTicosdHttpMethod_POST,
                              json_object_to_json_string(request_body), &recvdata, &recvlen);
  if (rc != kTicosdNetworkResult_OK) {
    goto cleanup;
  }
  recvdata[recvlen - 1] = '\0';

  if (!prv_parse_file_upload_prepare_response(recvdata, upload_url, upload_token)) {
    rc = kTicosdNetworkResult_ErrorRetryLater;
    goto cleanup;
  }

  rc = kTicosdNetworkResult_OK;

cleanup:
  json_object_put(request_body);
  free(recvdata);
  return rc;
}

static ticosdNetworkResult prv_file_upload(sTicosdNetwork *handle, const char *url,
                                               const char *filename, const size_t filesize,
                                               bool is_gzipped) {
  ticosdNetworkResult rc;
  struct curl_slist *headers = NULL;

  FILE *fd;
  if (!(fd = fopen(filename, "rb"))) {
    fprintf(stderr, "network:: Failed to open upload file %s : %s", filename, strerror(errno));
    rc = kTicosdNetworkResult_ErrorNoRetry;
    goto cleanup;
  }

  curl_easy_setopt(handle->curl, CURLOPT_URL, url);
  curl_easy_setopt(handle->curl, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt(handle->curl, CURLOPT_READDATA, fd);
  curl_easy_setopt(handle->curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)filesize);

  if (is_gzipped) {
    headers = curl_slist_append(headers, "Content-Encoding: gzip");
    curl_easy_setopt(handle->curl, CURLOPT_HTTPHEADER, headers);
  }

  const CURLcode res = curl_easy_perform(handle->curl);
  rc = prv_check_error(handle, res, "PUT", url);

cleanup:
  curl_slist_free_all(headers);
  curl_easy_reset(handle->curl);
  fclose(fd);
  return rc;
}

static ticosdNetworkResult prv_file_upload_commit(sTicosdNetwork *handle,
                                                      const char *endpoint, const char *token) {
  ticosdNetworkResult rc;
  char *payload = NULL;
  const sTicosdDeviceSettings *settings = ticosd_get_device_settings(handle->ticosd);

  const char *payload_fmt = "{"
                            "  \"file\": {"
                            "    \"token\": \"%s\""
                            "  },"
                            "  \"device\": {"
                            "    \"device_serial\": \"%s\","
                            "    \"hardware_version\": \"%s\","
                            "    \"software_version\": \"%s\","
                            "    \"software_type\": \"%s\""
                            "  }"
                            "}";
  if (ticos_asprintf(&payload, payload_fmt, token, settings->device_id,
                        settings->hardware_version, handle->software_version,
                        handle->software_type) == -1) {
    rc = kTicosdNetworkResult_ErrorRetryLater;
    goto cleanup;
  }

  rc = ticosd_network_post(handle, endpoint, kTicosdHttpMethod_POST, payload, NULL, 0);

cleanup:
  free(payload);

  return rc;
}

static const char *prv_method_as_string(enum TicosdHttpMethod method) {
  switch (method) {
    case kTicosdHttpMethod_POST:
      return "POST";
    case kTicosdHttpMethod_PATCH:
      return "PATCH";
    default:
      return "UNKNOWN";
  }
}

/**
 * @brief Initialises the network object
 *
 * @param ticosd Main ticosd handle
 * @return ticosd_network_h network object
 */
sTicosdNetwork *ticosd_network_init(sTicosd *ticosd) {
  sTicosdNetwork *handle = calloc(sizeof(sTicosdNetwork), 1);
  if (!handle) {
    fprintf(stderr, "network:: Failed to allocate memory for handle\n");
    goto cleanup;
  }

  handle->ticosd = ticosd;
  handle->during_network_failure = false;

  if (!(handle->curl = curl_easy_init())) {
    fprintf(stderr, "network:: Failed to initialise CURL.\n");
    goto cleanup;
  }

  if (!ticosd_get_string(handle->ticosd, "", "software_type", &handle->software_type) ||
      strlen(handle->software_type) == 0) {
    fprintf(stderr, "network:: Failed to get software_type\n");
    goto cleanup;
  }

  if (!ticosd_get_string(handle->ticosd, "", "software_version", &handle->software_version) ||
      strlen(handle->software_version) == 0) {
    fprintf(stderr, "network:: Failed to get software_version\n");
    goto cleanup;
  }

  if (!ticosd_get_string(handle->ticosd, "", "base_url", &handle->base_url) ||
      strlen(handle->base_url) == 0) {
    fprintf(stderr, "network:: Failed to get base_url\n");
    goto cleanup;
  }

  const char *project_key;
  if (!ticosd_get_string(handle->ticosd, "", "project_key", &project_key) ||
      strlen(project_key) == 0) {
    fprintf(stderr, "network:: Failed to get project_key\n");
    goto cleanup;
  }

  char *project_key_fmt = "Ticos-Project-Key: %s";
  if (ticos_asprintf(&handle->project_key_header, project_key_fmt, project_key) == -1) {
    goto cleanup;
  }

  return handle;

cleanup:
  free(handle->project_key_header);
  free(handle);
  return NULL;
}

/**
 * @brief Destroy the network object
 *
 * @param handle network object
 */
void ticosd_network_destroy(sTicosdNetwork *handle) {
  if (handle) {
    if (handle->curl) {
      curl_easy_cleanup(handle->curl);
    }
    free(handle->project_key_header);
    free(handle);
  }
}

/**
 * @brief Perform POST against a given endpoint
 *
 * @param handle network object
 * @param endpoint Path
 * @param payload Data to send
 * @param data Data returned if available
 * @param len Length of data returned
 * @return A ticosdNetworkResult value indicating whether the POST was successful or not.
 */
ticosdNetworkResult ticosd_network_post(sTicosdNetwork *handle, const char *endpoint,
                                               enum TicosdHttpMethod method, const char *payload,
                                               char **data, size_t *len) {
  char *url = prv_create_url(handle, endpoint);
  if (!url) {
    return kTicosdNetworkResult_ErrorRetryLater;
  }

  struct _write_callback recv_buf = {0};
  if (data) {
    recv_buf.buf = malloc(1);
    recv_buf.size = 0;
  }

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "charset: utf-8");
  headers = curl_slist_append(headers, handle->project_key_header);

  curl_easy_setopt(handle->curl, CURLOPT_URL, url);
  curl_easy_setopt(handle->curl, CURLOPT_POSTFIELDS, payload);
  curl_easy_setopt(handle->curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(handle->curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(handle->curl, CURLOPT_WRITEFUNCTION, prv_network_write_callback);
  curl_easy_setopt(handle->curl, CURLOPT_WRITEDATA, (void *)&recv_buf);
  if (method == kTicosdHttpMethod_PATCH) {
    curl_easy_setopt(handle->curl, CURLOPT_CUSTOMREQUEST, "PATCH");
  }
  const CURLcode res = curl_easy_perform(handle->curl);
  curl_slist_free_all(headers);

  const ticosdNetworkResult result =
    prv_check_error(handle, res, prv_method_as_string(method), url);

  free(url);

  if (result == kTicosdNetworkResult_OK) {
    if (data) {
      *len = recv_buf.size;
      *data = recv_buf.buf;
    }
  } else {
    if (data) {
      free(recv_buf.buf);
    }
  }

  curl_easy_reset(handle->curl);
  return result;
}

ticosdNetworkResult ticosd_network_file_upload(sTicosdNetwork *handle,
                                                      const char *commit_endpoint,
                                                      const char *filename, bool is_gzipped) {
  ticosdNetworkResult rc;
  char *upload_url = NULL;
  char *upload_token = NULL;

  struct stat st;
  if (stat(filename, &st) == -1) {
    fprintf(stderr, "network:: Failed to stat file '%s' : %s\n", filename, strerror(errno));
    rc = kTicosdNetworkResult_ErrorNoRetry;
    goto cleanup;
  }

  rc = prv_file_upload_prepare(handle, "/api/v0/upload", st.st_size, &upload_url, &upload_token,
                               is_gzipped);
  if (rc != kTicosdNetworkResult_OK) {
    goto cleanup;
  }

  rc = prv_file_upload(handle, upload_url, filename, st.st_size, is_gzipped);
  if (rc != kTicosdNetworkResult_OK) {
    goto cleanup;
  }

  rc = prv_file_upload_commit(handle, commit_endpoint, upload_token);
  if (rc != kTicosdNetworkResult_OK) {
    goto cleanup;
  }

  fprintf(stderr, "network:: Successfully transmitted file '%s'\n", filename);

  unlink(filename);
  rc = kTicosdNetworkResult_OK;

cleanup:
  free(upload_url);
  free(upload_token);
  return rc;
}
