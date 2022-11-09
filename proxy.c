#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

#define NTHREADS 8
#define SBUFSIZE 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3";

void *thread(void *vargp);
void proxy(int fd);
void parse_url(char *url, char *hostname, char *uri, char *ip, char *port);
void make_request_header(int fd, rio_t *rp, char *hostname, char *uri, char *headers);
void read_response_headers(rio_t *rp, char *response_headers, char *response_content_length);

sbuf_t sbuf;
cache_header *ch;

int main(int argc, char **argv)
{
  int i, listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  // port를 입력하지 않았으면
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  ch = cache_init();

  // 입력 받은 port에 듣기 식별자 열기
  listenfd = Open_listenfd(argv[1]);

  sbuf_init(&sbuf, SBUFSIZE);
  for (i = 0; i < NTHREADS; i++)
  {
    Pthread_create(&tid, NULL, thread, NULL);
  }

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("=====Accepted connection from (%s, %s)=====\r\n", hostname, port);
    sbuf_insert(&sbuf, connfd);
  }
}

void *thread(void *vargp)
{
  Pthread_detach(pthread_self());
  while (1)
  {
    int connfd = sbuf_remove(&sbuf);

    printf("=====Start Work : THREAD %d=====\r\n\r\n", pthread_self());
    proxy(connfd);
    Close(connfd);
    printf("=====Disconnect to Client=====\r\n\r\n");
  }
}

void proxy(int fd)
{
  cache_t *cache;

  int clientfd, filesize;
  char buf[MAXLINE], method[MAXLINE], url[MAXLINE], origin_url[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], ip[MAXLINE], port[MAXLINE], uri[MAXLINE], headers[MAXLINE];
  char response_headers[MAXLINE], response_content_length[MAXLINE];
  char *srcp;
  rio_t rio_p_c, rio_s_p;

  Rio_readinitb(&rio_p_c, fd);
  Rio_readlineb(&rio_p_c, buf, MAXLINE);
  printf("=====Recieve request from Client=====\r\n\r\n");
  sscanf(buf, "%s %s %s", method, url, version);
  strcpy(origin_url, url);

  cache = cache_search(ch, origin_url);

  if (cache != NULL)
  {
    printf("=====Proxy have Cached Data=====\r\n");
    Rio_writen(fd, cache->response_header, strlen(cache->response_header));
    Rio_writen(fd, cache->response_body, cache->filesize);
    printf("=====Send Response=====\r\n\r\n");
    printf("***Saved Cache:\r\n");
    cache_all_display(ch);

    return;
  }
  else
    printf("=====Proxy don't have Cached Data=====\r\n\r\n");

  printf("***Request header from Client:\r\n");
  printf("%s", buf);

  parse_url(url, hostname, uri, ip, port);
  make_request_header(fd, &rio_p_c, hostname, uri, headers);

  printf("***Requset header to Server:\r\n");
  printf("%s", headers);

  clientfd = Open_clientfd(ip, port);
  Rio_readinitb(&rio_s_p, clientfd);
  printf("=====Connect to Server=====\r\n\r\n");

  Rio_writen(clientfd, headers, strlen(headers));

  read_response_headers(&rio_s_p, response_headers, response_content_length);
  printf("=====Read Response Header=====\r\n\r\n");
  printf("***Response Header from Server to Client:\r\n");
  printf("%s\r\n\r\n", response_headers);

  filesize = atoi(response_content_length);
  srcp = (char *)Malloc(filesize);
  Rio_readnb(&rio_s_p, srcp, filesize);
  printf("=====Read Response Body=====\r\n\r\n");

  if ((cache = cache_search(ch, origin_url)) == NULL)
  {
    printf("=====Try to Save Response to Cache=====\r\n");
    cache_append(ch, origin_url, response_headers, srcp, filesize);
    printf("***Saved Cache:\r\n");
    cache_all_display(ch);
  }

  Rio_writen(fd, response_headers, strlen(response_headers));
  printf("=====Send Response Header to Client=====\r\n\r\n");
  Rio_writen(fd, srcp, filesize);
  printf("=====Send Response Body to Client=====\r\n\r\n");

  Free(srcp);

  Close(clientfd);
  printf("=====Disconnect to Server=====\r\n\r\n");
}

void parse_url(char *url, char *hostname, char *uri, char *ip, char *port)
{
  char *ptr, *start;
  char webserver_port[] = "80";

  start = url;
  if (strstr(url, "http://"))
    start = url + 7; // skip 'http://'

  ptr = index(start, '/');
  if (ptr)
  {
    strncpy(hostname, start, (size_t)(ptr - start));
    strcpy(uri, ptr);
  }

  ptr = index(hostname, ':');
  if (ptr)
  {
    strncpy(ip, hostname, (size_t)(ptr - hostname));
    strcpy(port, ptr + 1);
  }
  else
  {
    strcpy(ip, hostname);
    strcpy(port, webserver_port);
  }
}

void make_request_header(int fd, rio_t *rp, char *hostname, char *uri, char *headers)
{
  char *header_data;
  char header_buf[MAXLINE], buf[MAXLINE], header_name[MAXLINE];

  sprintf(header_buf, "GET %s HTTP/1.0\r\n", uri);
  sprintf(header_buf, "%sHost: %s\r\n", header_buf, hostname);
  sprintf(header_buf, "%s%s\r\n", header_buf, user_agent_hdr);
  sprintf(header_buf, "%sConnection: close\r\n", header_buf);
  sprintf(header_buf, "%sProxy-Connection: close\r\n", header_buf);

  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);

    header_data = index(buf, ':');
    if (header_data)
    {
      strncpy(header_name, buf, (size_t)(header_data - buf));
      header_name[(size_t)(header_data - buf)] = '\0';
      header_data += 2;

      if (!strcasecmp(header_name, "Host") || !strcasecmp(header_name, "User-Agent") || !strcasecmp(header_name, "Connection") || !strcasecmp(header_name, "Proxy-Connection"))
        continue;
      else
        sprintf(header_buf, "%s%s: %s\r\n", header_buf, header_name, header_data);
    }
  }

  sprintf(header_buf, "%s\r\n", header_buf);
  strcpy(headers, header_buf);
  return;
}

void read_response_headers(rio_t *rp, char *response_headers, char *response_content_length)
{
  char *header_data;
  char buf[MAXLINE], header_name[MAXLINE], headers[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  sprintf(headers, "%s", buf);

  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);

    header_data = index(buf, ':');
    if (header_data)
    {
      strncpy(header_name, buf, (size_t)(header_data - buf));
      header_name[(size_t)(header_data - buf)] = '\0';
      header_data += 2;

      if (!strcasecmp(header_name, "Content-length"))
      {
        strcpy(response_content_length, header_data);
      }
    }

    sprintf(headers, "%s%s", headers, buf);
  }

  strcpy(response_headers, headers);

  return;
}

/*
GET localhost:8000/godzilla.jpg HTTP/1.0
Host:52.78.57.212:5000
User-Agent: "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3"
Connection: keep
Proxy-Connection: keep
abc: 123
*/