
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <time.h>
#include "http.h"

#define ZV_AGAIN    EAGAIN

#define ZV_HTTP_PARSE_INVALID_METHOD        10
#define ZV_HTTP_PARSE_INVALID_REQUEST       11
#define ZV_HTTP_PARSE_INVALID_HEADER        12

#define ZV_HTTP_UNKNOWN                     0x0001
#define ZV_HTTP_GET                         0x0002
#define ZV_HTTP_HEAD                        0x0004
#define ZV_HTTP_POST                        0x0008

#define ZV_HTTP_OK                          200

#define ZV_HTTP_NOT_MODIFIED                304

#define ZV_HTTP_NOT_FOUND                   404

#define MAX_BUF 8124

typedef struct yk_http_request_s {
    void *root;
    int fd; //被监听的描述符
    int epfd;
    char buf[MAX_BUF];  /* ring buffer */ //环形缓冲区
    size_t pos, last; //last表示最后数据的位置 //初始化的时候last为0
    int state; //状态机相关，初始化为0
    void *request_start; //记录请求体的开始
    void *method_end;   /* not include method_end*/
    int method; //请求方法,GET POST ...
    void *uri_start; //请求的url在缓冲区的起始位置
    void *uri_end;   //请求的url再缓冲区的终止位置   /* not include uri_end*/
    void *path_start;
    void *path_end;
    void *query_start;
    void *query_end;
    int http_major;
    int http_minor;
    void *request_end;

    struct list_head list;  /* store http header */
    void *cur_header_key_start;
    void *cur_header_key_end;
    void *cur_header_value_start;
    void *cur_header_value_end;

    void *timer;
} yk_http_request_t; //epoll回调的数据结构

typedef struct {
    int fd;
    int keep_alive;
    time_t mtime;       /* the modified time of the file*/
    int modified;       /* compare If-modified-since field with mtime to decide whether the file is modified since last time*/

    int status;
} yk_http_out_t;

typedef struct yk_http_header_s {
    void *key_start, *key_end;          /* not include end */
    void *value_start, *value_end;
    list_head list;
} yk_http_header_t;

typedef int (*yk_http_header_handler_pt)(yk_http_request_t *r, yk_http_out_t *o, char *data, int len);

typedef struct {
    char *name;
    yk_http_header_handler_pt handler;
} yk_http_header_handle_t;

void yk_http_handle_header(yk_http_request_t *r, yk_http_out_t *o);
int yk_http_close_conn(yk_http_request_t *r);

int yk_init_request_t(yk_http_request_t *r, int fd, int epfd, yk_conf_t *cf);
int yk_free_request_t(yk_http_request_t *r);

int yk_init_out_t(yk_http_out_t *o, int fd);
int yk_free_out_t(yk_http_out_t *o);

const char *get_shortmsg_from_status_code(int status_code);

extern yk_http_header_handle_t     yk_http_headers_in[];

#endif

