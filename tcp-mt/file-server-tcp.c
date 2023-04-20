#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "err.h"
#include "common.h"

#define BUFFER_SIZE 100
#define H_FRAG_LEN_SIZE 4
#define H_NAME_LEN_SIZE 2
#define QUEUE_LENGTH 5

size_t total_received = 0;
void *handle_connection(void *client_fd_ptr)
{
    int client_fd = *(int *)client_fd_ptr;
    free(client_fd_ptr);

    char *base_location = "./";
    size_t name_offset = strlen(base_location);
    char *ip = get_ip_from_socket(client_fd);
    int port = get_port_from_socket(client_fd);

    char file_len_buffer[H_FRAG_LEN_SIZE], name_len_buffer[H_NAME_LEN_SIZE];
    if ((ssize_t)receive_message(client_fd, file_len_buffer, H_FRAG_LEN_SIZE, NO_FLAGS) < 0)
    {
        PRINT_ERRNO();
    }
    if ((ssize_t)receive_message(client_fd, name_len_buffer, H_NAME_LEN_SIZE, NO_FLAGS) < 0)
    {
        PRINT_ERRNO();
    }

    uint32_t file_len = ntohl(((uint32_t *)file_len_buffer)[0]);
    uint16_t name_len = ntohs(((uint16_t *)name_len_buffer)[0]) + 1 + name_offset;
    char *file_name = (char *)malloc(sizeof(*file_name) * name_len);
    file_name[name_len - 1] = 0;
    memcpy(file_name, base_location, name_offset);
    if ((ssize_t)receive_message(client_fd, file_name + name_offset, name_len - name_offset - 1, NO_FLAGS) < 0)
    {
        PRINT_ERRNO();
    }

    printf("new client [%s:%d] size=%u file=%s\n", ip, port, file_len, file_name);
    int out_file_fd = open(file_name, O_CREAT | O_WRONLY, 0777);
    if (out_file_fd < 0)
    {
        PRINT_ERRNO();
    }
    char fragment_buffer[BUFFER_SIZE];
    memset(fragment_buffer, 0, BUFFER_SIZE);
    for (;;)
    {
        ssize_t read = receive_message(client_fd, fragment_buffer, BUFFER_SIZE - 1, NO_FLAGS);
        if (read < 0)
        {
            PRINT_ERRNO();
        }
        if (read == 0)
        {
            break;
        }

        ssize_t written = write(
            out_file_fd,
            fragment_buffer,
            read);
        if (written < 0)
        {
            PRINT_ERRNO();
        }
    }

    total_received += file_len;
    printf("client [%s:%d] has sent its file of size=%u\n", ip, port, file_len);
    printf("total size of uploaded files %lu\n", total_received);
    CHECK_ERRNO(close(client_fd));
    CHECK_ERRNO(close(out_file_fd));
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fatal("usage: file-server-tcp port\n");
    }
    int socket_fd = open_socket();
    if (socket_fd < 0)
    {
        PRINT_ERRNO();
    }

    int port = atoi(argv[1]);

    bind_socket(socket_fd, port);
    printf("Listening on port %d\n", port);

    start_listening(socket_fd, QUEUE_LENGTH);

    for (;;)
    {
        struct sockaddr_in client_addr;

        int client_fd = accept_connection(socket_fd, &client_addr);

        // Arguments for the thread must be passed by pointer
        int *client_fd_pointer = malloc(sizeof(int));
        if (client_fd_pointer == NULL)
        {
            fatal("malloc");
        }
        *client_fd_pointer = client_fd;

        pthread_t thread;
        CHECK_ERRNO(pthread_create(&thread, 0, handle_connection, client_fd_pointer));
        CHECK_ERRNO(pthread_detach(thread));
    }
}
