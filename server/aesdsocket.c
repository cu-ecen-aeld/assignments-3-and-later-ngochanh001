#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define SIZE_BUF 100000

int server_fd = -1, client_fd = -1;
volatile sig_atomic_t exit_flag = 0;

void signal_handler(int signo)
{
    syslog(LOG_INFO, "Get signal. Exit");
    exit_flag = 1;
    if (server_fd != -1)
        close(server_fd);
    if (client_fd != -1)
        close(client_fd);
    remove(FILE_PATH);
}

int main(int argc, char *argv[])
{

    int daemon_mode = 0;
    if (argc == 2 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    // Signal
    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = 0,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Socket setup
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        syslog(LOG_ERR, "Failed to create socket");
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    // Fix bug Address already in use
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        syslog(LOG_ERR, "setsockopt failed %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        syslog(LOG_ERR, "Socket bind failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1)
    {
        syslog(LOG_ERR, "Socket listen failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    // Daemon
    if (daemon_mode)
    {
        if (daemon(0, 0) != 0)
        {
            syslog(LOG_ERR, "Failed to daemon");
            return -1;
        }
    }
    char *buf = malloc(SIZE_BUF);

    while (!exit_flag)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd == -1)
            break;

        // Logs message
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        FILE *file = fopen(FILE_PATH, "a+");
        if (!file)
        {
            syslog(LOG_ERR, "Failed to open file");
            close(client_fd);
            continue;
        }

        // Receives data over the connection and appends to file /var/tmp/aesdsocketdata
        size_t rcv_cnt = 0;
        while (!exit_flag)
        {
            ssize_t rcv = recv(client_fd, buf + rcv_cnt, SIZE_BUF - rcv_cnt - 1, 0);
            if (rcv <= 0)
                break;
            rcv_cnt += rcv;
            buf[rcv_cnt] = '\0';
            if (strchr(buf, '\n'))
                break;
        }

        fwrite(buf, 1, rcv_cnt, file);
        fclose(file);

        // Returns the full content of /var/tmp/aesdsocketdata
        file = fopen(FILE_PATH, "r");
        if (file)
        {
            while ((rcv_cnt = fread(buf, 1, SIZE_BUF, file)) > 0)
                send(client_fd, buf, rcv_cnt, 0);
            fclose(file);
        }

        // Logs message
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
        close(client_fd);
        client_fd = -1;
    }

    // Cleanup
    free(buf);
    if (server_fd != -1)
    {
        close(server_fd);
    }
    remove(FILE_PATH);
    closelog();
    return 0;
}
