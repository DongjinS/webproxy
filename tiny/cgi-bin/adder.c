/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) { // fork된 자식 프로세스의 main 함수
  char *buf, *p, *method;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE], first_num[MAXLINE], second_num[MAXLINE];
  int n1=0, n2=0;
  /* Extract the two arguments */
  if ((buf = getenv("QUERY_STRING")) != NULL) { // 환경변수로 넘겨 받은 QUERY_STRING의 값이 NULL이 아니라면
      p = strchr(buf, '&'); // & 문자 위치 찾고 \0으로 만든다.
      *p = '\0'; //first=23\0second=123
      strcpy(arg1, buf); // & 문자 앞부분 first=23
      strcpy(arg2, p+1); // & 문자 뒷부분 second=123
      p = strchr(arg1, '='); // first=23 에서 = 문자 찾아서 \0으로 만든다.
      *p = '\0'; // first \0 23
      strcpy(first_num, p+1); // = 문자 뒷부분(23) 저장 first_num에 저장
      p = strchr(arg2, '='); // second=123 에서 = 문자 찾고 \0으로 만든다
      *p = '\0'; // second \0 123
      strcpy(second_num, p+1); // = 문자 뒷부분(123) 저장 second_num에 저장
      n1 = atoi(first_num); // 문자열 숫자 정수 타입으로 변환
      n2 = atoi(second_num);
  }
  /*  
    response body form tag 추가 - 동저건텐츠안에서도 동적 요청 보낼 수 있도록
    
      <form align="middle" action="/cgi-bin/adder" method="GET">
        <p>first number: <input type="text" name="first" /></p>
        <p>second number: <input type="text" name="second" /></p>
        <input type="submit" value="Submit" />
      </form>
  */
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "<div align=center>Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%s<form align='middle' action='/cgi-bin/adder' method='GET'>\r\n",content);
  sprintf(content, "%s<p>first number: <input type='text' name='first' /></p>\r\n",content);
  sprintf(content, "%s<p>second number: <input type='text' name='second' /></p>\r\n",content);
  sprintf(content, "%s<input type='submit' value='Submit' />\r\n",content);
  sprintf(content, "%s</form>\r\n", content);
  sprintf(content, "%s<div align=center>The answer is: %d + %d = %d\r\n<p>",
          content, n1, n2, n1 + n2);
  sprintf(content, "%s<div align=center>Thanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n"); // 리스폰스 헤더 전송 by 표준 출력 to 연결 소켓
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  /* cgi 환경변수에서 request method 참조 */
  method = getenv("REQUEST_METHOD");
  /* method가 GET 이라면 content(response body) 전송(출력) */
  if (strcasecmp(method, "GET")==0){
    printf("%s", content);
  }
  fflush(stdout); // 표준 출력 버퍼 비우기
  exit(0); // 프로그램 종료.
}
/* $end adder */
