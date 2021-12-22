/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);                                                       /* 클라이언트와 연결 후 작업 - 정적 or 동적 */
void read_requesthdrs(rio_t *rp);                                        /* 클라이언트로 부터 온 리퀘스트 헤더를 읽는다 */
int parse_uri(char *uri, char *filename, char *cgiargs);                 /* 리퀘스트 헤더에 있던 uri 를 파싱한다 */
void serve_static(int fd, char *filename, int filesize, char *method);   /* 정적 컨텐츠 요청이 왔을 때 */
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method); /* 동적  컨텐츠 요청이 왔을때 */
void get_filetype(char *filename, char *filetype);                       /* 요청 받은 file type을 얻는다 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,      /* 잘못된 요청이 온 경우 클라이언트에게 에러를 보낸다 */
                 char *longmsg);

int main(int argc, char **argv) // 메인함수 - 듣기소켓을 만들고 클라이언트로 부터 연결을 기다린다
{
  int listenfd, connfd;                  // 듣기 소켓 식별자, 연결 소켓 식별자
  char hostname[MAXLINE], port[MAXLINE]; // 리퀘스트를 보낸 호스트(클라이언트)의 이름(주소)와 포트번호를 담을 배열
  socklen_t clientlen;                   // 클라이언트의 길이를 담을 변수
  struct sockaddr_storage clientaddr;    // 클라이언트의 주소를 담을 구조체

  /* Check command line args */
  if (argc != 2) // tiny를 실행했을때 주어진 인자가 두개가 아니라면 에러 출력하고 종료(1)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 주어진 포트를 갖고 듣기 소켓을 만들고 식별자를 저장한다
  while (1)                          // 클라이언트로부터 연결이 올때 까지 반복 - 연결이 되도 doit 실행하고 다시 반복
  {
    clientlen = sizeof(clientaddr);                                                 // clientlen = 클라리언트 주소의 구조체 크기
    connfd = Accept(listenfd, (SA *)&clientaddr,                                    // 연결되면 연결 소켓 만들고 식별자 저장
                    &clientlen);                                                    // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 연결된 클라이언트의 주소와 포트번호를 얻어
    printf("Accepted connection from (%s, %s)\n", hostname, port);                  //출력
    doit(connfd);                                                                   // line:netp:tiny:doit - doit 함수 실행
    Close(connfd);                                                                  // line:netp:tiny:close - doit 종료되면 연결식별자 닫기
  }
}

void doit(int fd) // 클라이언트와 연결된 후 실행
{
  int is_static;    // 컨텐츠 요청 정적인지 동적인지 알아 볼 flag
  struct stat sbuf; // 요청받은 파일의 stat을 확인하고 저장할 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  /* buf - 연결소켓으로 부터 읽어올 정보를 갖고올 배열, 메소드 배열, uri배열, (HTTP)`version배열 */
  char filename[MAXLINE], cgiargs[MAXLINE]; // 파일이름 저장할 배열, cgi 인자 저장할 배열
  rio_t rio;                                // rio 함수를 이용할때 소켓 식별자와 연결될 rio 구조체

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);                       // rio 구조체와 연결식별자 rio로 연결 - rio를 통해 식별자(소켓)를 읽고 쓰겠다
  Rio_readlineb(&rio, buf, MAXLINE);             //클라이언트로 부터 온 리퀘스트 읽어서 버퍼에 저장
  printf("Request headers:\n");                  // 리퀘스트 헤더는
  printf("%s", buf);                             // 버퍼 출력 - 첫번째 리퀘스트헤더 ( GET / HTTP/1.1 )
  sscanf(buf, "%s %s %s", method, uri, version); // 버퍼에 담긴 스트링 세개 메소드, uri, 버전으로 저장

  if (!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) // method가 get이거나 head면 !true -> false
  {                                                                         //get , head 메소드 아니면 에러 출력
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");
    return;
  }

  read_requesthdrs(&rio); // ( GET / HTTP/1.1 ) 제외한 레퀘스트 헤더 읽기
  /* Parse URI from GET request */
  is_static = parse_uri(uri, filename, cgiargs); // uri 에서 filename 과 cgiargs 추출 후 is_static 확인
  if (stat(filename, &sbuf) < 0)                 //요청받은 파일 존재하지 않으면 에러 출력
  {
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn’t find this file");
    return;
  }
  if (is_static)
  {                                                            /* Serve static content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) // 요청받은 파일 권한 없으면 에러 출력
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn’t read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size, method); // 함수 실행 - 요청받은 정적 컨텐츠 전달
  }
  else
  {                                                            /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) // 요청받은 파일 권한 없으면 에러 출력
    {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn’t run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method); //함수 실행 - 요청받은 동적 컨텐츠 전달
  }
}

void read_requesthdrs(rio_t *rp) // 리퀘스트 헤더 읽고 출력,
{
  char buf[MAXLINE]; //읽은 헤더를 저장할 버퍼

  Rio_readlineb(rp, buf, MAXLINE); //출력에서 제외 - ( Host: 54.180.150.171:8000 )
  while (strcmp(buf, "\r\n"))      // \r\n 나올 때까지 읽고 출력
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;
  if (!strstr(uri, "cgi-bin"))       // uri에 cgi-bin이 없으면
  {                                  /* Static content */
    strcpy(cgiargs, "");             // cgiargs 비워놓고
    strcpy(filename, ".");           // 파일명 시작 = .
    strcat(filename, uri);           // . 뒤에 uri를 붙인다.
    if (uri[strlen(uri) - 1] == '/') // uri가 /로 끝나면 파일명에 home.html 붙여주기
      strcat(filename, "home.html");
    return 1; //리턴 1 = is_static = 1
  }
  else                     // uri에 cgi-bin 있으면
  {                        /* Dynamic content */
    ptr = index(uri, '?'); // ? 의 위치 찾고 저장
    if (ptr)               // ? 있으면
    {
      strcpy(cgiargs, ptr + 1); // ? 뒤부터 cgiargs에 넣어준다.
      *ptr = '\0';              // ? 는 \0 (=NULL)로 바꿔줌 = 출력(읽거나 쓰기)의 끝
    }
    else                   // ? 없으면
      strcpy(cgiargs, ""); // cgiargs 비우고
    strcpy(filename, "."); // 파일명 .으로 시작
    strcat(filename, uri); // 파일명 . 뒤에 uri 붙여주기
    return 0;              // 리턴 0 = is_static = 0
  }
}

void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd; // 정적 컨텐츠 파일 식별자
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  get_filetype(filename, filetype);    // 파일타입 저장
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // response header 시작 - buf에 쭉 담아서
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);   // 파일 사이즈 추가
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype); // 파일 타입 추가
  Rio_writen(fd, buf, strlen(buf));                          // 연결소켓을 통해 rio_writen으로 buf를 클라이언트에게 보냄
  printf("Response headers:\n");                             // 서버에서도 리스폰스 헤더 출력 (버퍼 까지)
  printf("%s", buf);

  /* when method = HEAD */
  if (strcasecmp(method, "HEAD") == 0)
  { // 정적 컨텐츠 요청이 HEAD 요청으로 왔다면 response 헤더 까지만 보내고 함수 끝
    return;
  }

  /* Send response body to client */
  srcfd = Open(filename, O_RDONLY, 0); // 요청 받은 파일을 열고 식별자 지정
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // Mmap대신 malloc 사용
  srcp = malloc(filesize);          // 열린 파일을 올릴 메모리 할당
  rio_readn(srcfd, srcp, filesize); // 할당 받은 메모리에 열린 파일 읽고 저장
  Close(srcfd);                     // 파일 닫기
  Rio_writen(fd, srcp, filesize);   // 메모리에 올린 파일 연결 소켓을 통해 클라이언트에게 보낸다.
  // Munmap(srcp, filesize);
  free(srcp); // 메모리 할당 해제
}

/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype) // 요청받은 파일 타입 찾기
{                                                 //파일 이름이 끝나는 확장자에 따라 파일 타입 다르게 저장한ㄷ.
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mpeg"))
    strcpy(filetype, "video/mpeg");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = {NULL};
  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n"); // 동적 컨텐츠 실행전에 먼저 리스폰스 헤더의 첫부분 보낸다.
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  if (Fork() == 0) // 자식 프로세스 생성
  {                /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);                         //환경변수 QUERY_STRING에 cgiargs 저장(first=23&second=123)
    setenv("REQUEST_METHOD", method, 1);                        //환경변수  REQUEST_METHOD에 method 저장 - GET or HEAD
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */    // 표준출력 연결 소켓으로 보내도록 재지정
    Execve(filename, emptylist, environ); /* Run CGI program */ //동적 컨텐츠 실행 with 환경변수
  }
  Wait(NULL); /* Parent waits for and reaps child */ 
  // 부모 프로세스는 wait - 프로그램 끝나고 -> 자식 프로세스 끝나면  부모 다시 시작
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{//클라이언트에게 에러 출력
  char buf[MAXLINE], body[MAXBUF];
  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}