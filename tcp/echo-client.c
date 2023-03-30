#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "err.h"

#define BUFFER_SIZE 20

char shared_buffer[BUFFER_SIZE];

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fatal("Usage: %s host port n k\n", argv[0]);
  }

  char *host = argv[1];
  uint16_t port = read_port(argv[2]);
  long n = atol(argv[3]), k = atol(argv[4]);
  char *msg = malloc(sizeof(*msg) * k);
  memset(msg, 1, k);
  msg[k - 1] = '\0';
  struct sockaddr_in server_address = get_address(host, port);

  int socket_fd = open_socket();

  connect_socket(socket_fd, &server_address);

  char *server_ip = get_ip(&server_address);
  uint16_t server_port = get_port(&server_address);

  int flags = 0;
  for (long i = 0; i < n; i++) {
    send_message(socket_fd, msg, k, flags);
    printf("[%ld] Sent %zd bytes to %s:%u\n", i, k, server_ip, server_port);
  }

  // Notify server that we're done sending messages
  CHECK_ERRNO(shutdown(socket_fd, SHUT_WR));

  size_t received_length;
  do {
    memset(shared_buffer, 0, BUFFER_SIZE); // clean the buffer
    size_t max_length = BUFFER_SIZE - 1; // leave space for the null terminator
    int flags = 0;
    received_length =
        receive_message(socket_fd, shared_buffer, max_length, flags);
    printf("Received %zd bytes from %s:%u: '%s'\n", received_length, server_ip,
           server_port, shared_buffer);
  } while (received_length > 0);

  CHECK_ERRNO(close(socket_fd));

  return 0;
}
