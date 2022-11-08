#include <stdio.h>
#include "csapp.h"

int main(int argc, char **argv)
{
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;
    char header[MAXLINE];

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);

    sprintf(header, "GET / HTTP/1.0\r\n");
    sprintf(header, "%sheader_name: header_data\r\n\r\n", header);

    Rio_writen(clientfd, header, strlen(header));

    Rio_readnb(&rio, buf, MAXLINE);
    printf("response%s", buf);

    Close(clientfd);
    exit(0);
}