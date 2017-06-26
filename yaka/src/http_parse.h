
#ifndef HTTP_PARSE_H
#define HTTP_PARSE_H

#define CR '\r'
#define LF '\n'
#define CRLFCRLF "\r\n\r\n"
#define REQUESTLINEBASE 0
#define REQUESTBODYBASE 100 //adder by andreYang

int yk_http_parse_request_line(yk_http_request_t *r);
int yk_http_parse_request_body(yk_http_request_t *r);

#endif
