#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <libgen.h>

int main(int argc, char *argv[])
{
    // printf("%d %s %s %s\n", argc, argv[0], argv[1], argv[2]);
    if (argc != 3)
    {
        fprintf(stderr, "Error: Two arguments required - <writefile> <writestr>\n");
        return 1;
    }

    const char *writefile = argv[1];
    const char *writestr = argv[2];

    openlog("writer", LOG_PID, LOG_USER);
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);


    int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
    {
        fprintf(stderr, "Error: Could not write to file %s\n", writefile);
        syslog(LOG_ERR, "Error: Could not write to file %s\n", writefile);
        return 1;
    }

    ssize_t ret =  write(fd, writestr, strlen(writestr));
    if(ret == -1)
    {
        fprintf(stderr, "Error: Could not write to file %s\n", writefile);
        syslog(LOG_ERR, "Error: Could not write to file %s\n", writefile);
        close(fd);
        return 1;
    }

    close(fd);

    return 0;
}