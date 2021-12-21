/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE], first_num[MAXLINE], second_num[MAXLINE];
  int n1=0, n2=0;
  /* Extract the two arguments */
  //http://54.180.150.171:8000/cgi-bin/form-adder?first=1&second=1
  if ((buf = getenv("QUERY_STRING")) != NULL) {
      p = strchr(buf, '&');
      *p = '\0';
      strcpy(arg1, buf); //first=1
      strcpy(arg2, p+1); //second=1
      p = strchr(arg1, '=');
      *p = '\0';
      strcpy(first_num, p+1);
      p = strchr(arg2, '=');
      *p = '\0';
      strcpy(second_num, p+1);
      n1 = atoi(first_num);
      n2 = atoi(second_num);
  }
  /*  
      content 추가
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
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);
  exit(0);
}
/* $end adder */
