#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define SOCKET_FILE "/var/tmp/aesdsocketdata"
#define PORT 9000

char *read_file(const char *filename);
int sockfd;

void clean()
{
    if (sockfd > 0)
    {
        close(sockfd);
    }
    remove(SOCKET_FILE);
    syslog(LOG_INFO, "Removed the data file and closing socket");
    closelog();
}

void handle_signal(int sig)
{
    if (sig == SIGINT || sig == SIGTERM)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
        clean();
        exit(0);
    }
}

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file for read");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_contents = malloc(sizeof(char) * (file_size + 1));
    if (file_contents == NULL)
    {
        perror("Unable to allocated memory to read file");
        fclose(file);
        return NULL;
    }

    long bytes_read = fread(file_contents, sizeof(char), file_size, file);
    if (bytes_read != file_size)
    {
        perror("Error reading the file");
        fclose(file);
        free(file_contents);
        return NULL;
    }

    file_contents[file_size] = '\0'; //terminate

    fclose(file);
    return file_contents;
}

int main(int argc, char **argv)
{
    int enable = 1;
    struct sockaddr_in server_addr;
    int opt;
    int is_daemon = 0;

    while ((opt = getopt(argc, argv, "d")) != -1)
    {
        switch (opt)
        {
        case 'd':
            is_daemon = 1;
            break;
        }
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        syslog(LOG_ERR, "Unable to create a socket: %s", strerror(errno));
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
    {
        syslog(LOG_ERR, "Unable to set socket options: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        syslog(LOG_ERR, "Unable to bind socket: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    if (is_daemon)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            syslog(LOG_ERR, "Unable to fork process : %m");
            exit(EXIT_FAILURE);
        }
        if (pid > 0)
        {
            exit(EXIT_SUCCESS);
        }

        umask(0);

        if (setsid() < 0)
        {
            syslog(LOG_ERR, "Failed to create new session: %m");
            exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    if (listen(sockfd, 10) < 0)
    {
        syslog(LOG_ERR, "Socket listen failed : %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    syslog(LOG_INFO, "Socket server running on port %i", PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));

        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            perror("Error in accepting client");
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        int recv_bytes = 0;
        while ((recv_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0)
        {
            FILE *file = fopen(SOCKET_FILE, "a");
            if (file == NULL)
            {
                perror("Error opening file to write");
                close(client_socket);
                continue;
            }
            fwrite(buffer, sizeof(char), recv_bytes, file);
            fclose(file);

            char *newline = strchr(buffer, '\n');
            if (newline != NULL)
            {
                char *file_contents = read_file(SOCKET_FILE);
                send(client_socket, file_contents, strlen(file_contents), 0);
                free(file_contents);
            }

            memset(buffer, 0, BUFFER_SIZE);
        }

        if (recv_bytes == -1)
        {
            perror("Error receiving data from client");
        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_socket);
    }

    close(sockfd);
    remove(SOCKET_FILE);
    syslog(LOG_INFO, "Exiting...");
    closelog();

    return 0;

}

