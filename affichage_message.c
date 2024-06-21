#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

int open_fifo(const char *fifo_path) {
    int fifo_fd = open(fifo_path, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1) {
        perror("Failed to open FIFO");
        exit(EXIT_FAILURE);
    }
    return fifo_fd;
}

int main(int argc, char *argv[]) {
    char *fifo_path = argv[1];
    char *parQui = argv[2];
    printf("content :%s \n",fifo_path);
    int fifo_fd = open_fifo(fifo_path);
    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    struct timeval timeout;

    printf("\t\t COMMUNICATION ... \n\n ");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(fifo_fd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int ret = select(fifo_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(fifo_fd, &read_fds)) {
            ssize_t num_read = read(fifo_fd, buffer, BUFFER_SIZE);
            if (num_read > 0) {
                buffer[num_read] = '\0';
                printf("%s --> %s\n",parQui,buffer);
                fflush(stdout);
            }
        } else if (ret == -1) {
            perror("Select error");
            break; // Quit if select encounters a serious error
        }
    }

    close(fifo_fd);
    return 0;
}
