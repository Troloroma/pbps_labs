#include "httpd.h"
#include <sys/stat.h>
#include <stdio.h>

#define CHUNK_SIZE 1024 // read 1024 bytes at a time

// Public directory settings
#define PUBLIC_DIR "/var/www/pico/webroot"
#define INDEX_HTML "/index.html"
#define NOT_FOUND_HTML "/404.html"
#define LOG_FILE "/var/log/foxweb.log"

void log_data_request(const char *method, const char *uri, int status, int response_size) {
    FILE *log_file = fopen(LOG_FILE, "a");
    if (!log_file) {
        syslog(LOG_ERR, "Failed to open log file");
        return;
    }

    time_t time_now = time(NULL);
    struct tm *time_info = localtime(&time_now);
    char timestamp[32];
    strftime(timestamp, 32, "%d/%b/%Y:%H:%M:%S %z", time_info);

    fprintf(log_file, "%s - - [%s] \"%s %s HTTP/1.1\" %d %d \"%s\" \"%s\"\n",
            request_header("X-Forwarded-For") ? request_header("X-Forwarded-For") : "127.0.0.1", 
            timestamp, 
            method, 
            uri, 
            status, 
            response_size, 
            request_header("Referer") ? request_header("Referer") : "-", 
            request_header("User-Agent") ? request_header("User-Agent") : "-");

    fclose(log_file);
}

int main(int c, char **v) {
  char *port = c == 1 ? "8000" : v[1];
  serve_forever(port);
  return 0;
}

int file_exists(const char *file_name) {
  struct stat buffer;
  int exists;

  exists = (stat(file_name, &buffer) == 0);

  return exists;
}

int read_file(const char *file_name) {
  char buf[CHUNK_SIZE];
  FILE *file;
  size_t nread;
  int err = 1;

  file = fopen(file_name, "r");

  if (file) {
    while ((nread = fread(buf, 1, sizeof buf, file)) > 0)
      fwrite(buf, 1, nread, stdout);

    err = ferror(file);
    fclose(file);
  }
  return err;
}

void route() {
  ROUTE_START()

  GET("/") {
    char index_html[20];
    sprintf(index_html, "%s%s", PUBLIC_DIR, INDEX_HTML);

    HTTP_200;
    if (file_exists(index_html)) {
      read_file(index_html);
      log_data_request("GET", "/", 200, CHUNK_SIZE);
    } else {
      printf("Hello! You are using %s\n\n", request_header("User-Agent"));
      log_data_request("GET", "/", 200, 0);
    }
  }

  GET("/test") {
    HTTP_200;
    printf("List of request headers:\n\n");

    header_t *h = request_headers();

    while (h->name) {
      printf("%s: %s\n", h->name, h->value);
      h++;
    }
    log_data_request("GET", "/test", 200, 0);
  }

  POST("/") {
    HTTP_201;
    printf("Wow, seems that you POSTed %d bytes.\n", payload_size);
    printf("Fetch the data using `payload` variable.\n");
    if (payload_size > 0)
      printf("Request body: %s", payload);
      log_data_request("POST", "/", 201, payload_size);
  }

  GET(uri) {
    char file_name[255];
    sprintf(file_name, "%s%s", PUBLIC_DIR, uri);

    if (file_exists(file_name)) {
      HTTP_200;
      read_file(file_name);
      log_data_request("GET", uri, 200, CHUNK_SIZE);
    } else {
      HTTP_404;
      sprintf(file_name, "%s%s", PUBLIC_DIR, NOT_FOUND_HTML);
      if (file_exists(file_name))
        read_file(file_name);
        log_data_request("GET", uri, 404, 0);
    }
  }

  ROUTE_END()
}
