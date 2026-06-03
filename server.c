#include <err.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const char *get_status_message(int status_code);
int handle_request_path(const char *request_path, const char **filename,
                        const char **content_type);
int handle_request(int client_fd, const char *buffer);
int send_headers(int client_fd, int status_code, const char *content_type,
                 size_t content_length);
int send_content(int client_fd, const char *content_body, size_t bytes);
int start_server(in_port_t port, struct sockaddr_in *);
int stop_server(int server_fd);
ssize_t handle_client(int client_fd);
void *routine(void *client_fd);

int main(void) {
  constexpr in_port_t port = 8080;

  int listen_fd;
  struct sockaddr_in server_addr;
  if ((listen_fd = start_server(port, &server_addr)) == -1) {
    err(EXIT_FAILURE, "start_server");
  }

  printf("Server listening on port %d\n", port);
  while (true) {
    int client_fd;
    size_t addrlen = sizeof(server_addr);
    if ((client_fd = accept(listen_fd, (struct sockaddr *)&server_addr,
                            (socklen_t *)&addrlen)) == -1) {
      err(EXIT_FAILURE, "accept");
    }

    pthread_t thread;
    pthread_create(&thread, nullptr, routine, (void *)(intptr_t)client_fd);
    pthread_detach(thread);
  }

  if (stop_server(listen_fd) == -1) {
    err(EXIT_FAILURE, "stop_server");
  }

  return 0;
}

int start_server(in_port_t port, struct sockaddr_in *server_addr) {
  constexpr int opt = 1;
  constexpr int backlog = 255;

  int listen_fd;
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    goto cleanup;
  }

  *server_addr = (struct sockaddr_in){.sin_family = AF_INET,
                                      .sin_port = htons(port),
                                      .sin_addr.s_addr = htonl(INADDR_ANY)};

  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                 (socklen_t)sizeof(opt)) < 0) {
    perror("setsockopt");
    goto cleanup;
  }

  if (bind(listen_fd, (struct sockaddr *)server_addr,
           sizeof(struct sockaddr_in)) != 0) {
    perror("bind");
    goto cleanup;
  }

  if (listen(listen_fd, backlog) == -1) {
    perror("listen");
    goto cleanup;
  }

  return listen_fd;

cleanup:
  close(listen_fd);
  return -1;
}

ssize_t handle_client(int client_fd) {
  char buffer[BUFSIZ];
  ssize_t request_length;

  if ((request_length = recv(client_fd, buffer, BUFSIZ - 1, 0)) == -1) {
    perror("recv");
    return -1;
  }

  buffer[request_length] = '\0';
  handle_request(client_fd, buffer);

  printf("%s\n", buffer);

  return request_length;
}

int send_headers(int client_fd, int status_code, const char *content_type,
                 size_t content_length) {
  char response[BUFSIZ];
  snprintf(response, BUFSIZ - 1,
           "HTTP/1.1 %d %s\r\nServer: sserver-c\r\nContent-Type: "
           "%s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
           status_code, get_status_message(status_code), content_type,
           content_length);

  if (send(client_fd, response, strlen(response), 0) == -1) {
    perror("send");
    return -1;
  }

  return 0;
}

const char *get_status_message(int status_code) {
  switch (status_code) {
  case 200:
    return "Ok";
  case 404:
    return "Not Found";
  default:
    return "Unkown Status Code";
  }
}

int stop_server(int server_fd) {
  if (close(server_fd) == -1) {
    perror("close");
    return -1;
  }

  return 0;
}

int handle_request(int client_fd, const char *buffer) {
  const char *bad_request =
      "<!DOCTYPE html><html><h1>400 Bad Request</h1></html>";
  const char *internal_server_error =
      "<!DOCTYPE html><html><h1>500 Internal Server Error</h1></html>";
  char request[BUFSIZ];
  strncpy(request, buffer, BUFSIZ);

  if (strtok(request, " ") == nullptr) {
    perror("handle_request: empty request");
    size_t len = strlen(bad_request);
    send_headers(client_fd, 400, "text/html", len);
    send_content(client_fd, bad_request, len);
    return -1;
  }
  char *request_path;
  if ((request_path = strtok(nullptr, " ")) == nullptr) {
    perror("handle_request: invalid request");
    size_t len = strlen(bad_request);
    send_headers(client_fd, 400, "text/html", len);
    send_content(client_fd, bad_request, len);
    return -1;
  }

  const char *request_filename = nullptr;
  const char *content_type;
  int status_code =
      handle_request_path(request_path, &request_filename, &content_type);

  FILE *request_file;
  if ((request_file = fopen(request_filename, "rb")) == nullptr) {
    perror("fopen");
    size_t len = strlen(internal_server_error);
    send_headers(client_fd, 500, content_type, len);
    send_content(client_fd, internal_server_error, len);
    return EXIT_FAILURE;
  }

  fseek(request_file, 0, SEEK_END);
  size_t content_length = (size_t)ftell(request_file);
  send_headers(client_fd, status_code, content_type, content_length);
  fseek(request_file, 0, SEEK_SET);
  char request_file_buffer[BUFSIZ];
  size_t bytes_read;
  while ((bytes_read = fread(request_file_buffer, sizeof(char), BUFSIZ,
                             request_file)) > 0) {
    if (send_content(client_fd, request_file_buffer, bytes_read) == -1) {
      perror("send_response");
      fclose(request_file);
      return EXIT_FAILURE;
    }
  }
  fclose(request_file);

  return EXIT_SUCCESS;
}

int handle_request_path(const char *request_path, const char **filename,
                        const char **content_type) {
  *content_type = "text/html";
  if (strcmp(request_path, "/") == 0 ||
      strcmp(request_path, "/index.html") == 0) {
    *filename = "content/index.html";
    return 200;
  } else if (strcmp(request_path, "/lento") == 0) {
    sleep(10);
    *filename = "content/index.html";
    return 200;
  }
  *filename = "content/not-found.html";
  return 404;
}

void *routine(void *client_fd) {
  ssize_t request = handle_client((int)(intptr_t)client_fd);
  close((int)(intptr_t)client_fd);

  return (void *)request;
}

int send_content(int client_fd, const char *content, size_t bytes) {
  if (send(client_fd, content, bytes, 0) == -1) {
    perror("send");
    return -1;
  }

  return 0;
}
