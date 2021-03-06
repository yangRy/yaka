
#include <stdint.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "util.h"
#include "timer.h"
#include "http.h"
#include "epoll.h"
#include "threadpool.h"

#define CONF "/home/ubuntu/temp/yaka/src/yaka.conf"
#define PROGRAM_VERSION "0.1"

extern struct epoll_event *events;

static const struct option long_options[]=
{
    {"help",no_argument,NULL,'?'},
    {"version",no_argument,NULL,'V'},
    {"conf",required_argument,NULL,'c'},
    {NULL,0,NULL,0}
};

static void usage() {
   fprintf(stderr,
	"yaka [option]... \n"
	"  -c|--conf <config file>  Specify config file. Default ./yaka.conf.\n"
	"  -?|-h|--help             This information.\n"
	"  -V|--version             Display program version.\n"
	);
}

int main(int argc, char* argv[]) {
    int rc;
    int opt = 0;
    int options_index = 0;
    char *conf_file = CONF;

    /*
    * parse argv 
    * more detail visit: http://www.gnu.org/software/libc/manual/html_node/Getopt.html
    */
/*
    if (argc == 1) {
        usage();
        return 0;
    }

    while ((opt=getopt_long(argc, argv,"Vc:?h",long_options,&options_index)) != EOF) {
        switch (opt) {
            case  0 : break;
            case 'c':
                conf_file = optarg;
                break;
            case 'V':
                printf(PROGRAM_VERSION"\n");
                return 0;
            case ':':
            case 'h':
            case '?':
                usage();
                return 0;
        }
    }

    debug("conffile = %s", conf_file);

    if (optind < argc) {
        log_err("non-option ARGV-elements: ");
        while (optind < argc)
            log_err("%s ", argv[optind++]);
        return 0;
    } */

    /*
    * read confile file
    */
    char conf_buf[BUFLEN];
    yk_conf_t cf;
    rc = read_conf(conf_file, &cf, conf_buf, BUFLEN);
    check(rc == ZV_CONF_OK, "read conf err");


    /*
    *   install signal handle for SIGPIPE
    *   when a fd is closed by remote, writing to this fd will cause system send
    *   SIGPIPE to this process, which exit the program
    */
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL)) {
        log_err("install sigal handler for SIGPIPE failed");
        return 0;
    }

    /*
    * initialize listening socket
    */
    int listenfd;
    struct sockaddr_in clientaddr;
    // initialize clientaddr and inlen to solve "accept Invalid argument" bug
    socklen_t inlen = 1;
    memset(&clientaddr, 0, sizeof(struct sockaddr_in));  
    
    listenfd = open_listenfd(cf.port); //listenfd是bind返回的
    rc = make_socket_non_blocking(listenfd); //监听描述符设置为非阻塞
    check(rc == 0, "make_socket_non_blocking");

    /*
    * create epoll and add listenfd to ep
    */
    //这个函数申请epoll事件组内存
    int epfd = yk_epoll_create(0); //epfd 是epoll_create返回的fd
    struct epoll_event event;
    
    yk_http_request_t *request = (yk_http_request_t *)malloc(sizeof(yk_http_request_t));
    yk_init_request_t(request, listenfd, epfd, &cf);

    event.data.ptr = (void *)request; //
    event.events = EPOLLIN | EPOLLET; //可读，边缘触发
    yk_epoll_add(epfd, listenfd, &event);

    /*
    * create thread pool
    */
    
  //  yk_threadpool_t *tp = threadpool_init(cf.thread_num);
   // check(tp != NULL, "threadpool_init error");
    
    
    /*
     * initialize timer
     */
    yk_timer_init();

    log_info("yaka started.");
    int n;
    int i, fd;
    int time;

    /* epoll_wait loop */
    while (1) {
        time = yk_find_timer();
        debug("wait time = %d", time);
//        printf("wait time = %d\n", time);
        n = yk_epoll_wait(epfd, events, MAXEVENTS, time);
        yk_handle_expire_timers();
        
        for (i = 0; i < n; i++) {
            yk_http_request_t *r = (yk_http_request_t *)events[i].data.ptr;
            fd = r->fd;
            
            if (listenfd == fd) { //如果event事件里面的fd是listenfd
                /* we hava one or more incoming connections */

                int infd;
                while(1) {
                    infd = accept(listenfd, (struct sockaddr *)&clientaddr, &inlen);
                    if (infd < 0) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            /* we have processed all incoming connections */
                            break;
                        } else {
                            log_err("accept");
                            break;
                        }
                    }

                    rc = make_socket_non_blocking(infd);
                    check(rc == 0, "make_socket_non_blocking");
                    log_info("new connection fd %d", infd);
                    //每次有新的连接，都会动态分配一个yk_http_request_t，并且将其地址变成void*,放置到epoll的回调数据中
                    yk_http_request_t *request = (yk_http_request_t *)malloc(sizeof(yk_http_request_t));
                    if (request == NULL) {
                        log_err("malloc(sizeof(yk_http_request_t))");
                        break;
                    }
                    yk_init_request_t(request, infd, epfd, &cf);
                    event.data.ptr = (void *)request; //event.data.fd 似乎是要监听的fd
                    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT; //只监听一次 边缘触发

                    yk_epoll_add(epfd, infd, &event);
                    yk_add_timer(request, TIMEOUT_DEFAULT, yk_http_close_conn);

                }   // end of while of accept

            } else { //aceept fd
                if ((events[i].events & EPOLLERR) || //描述符错误
                    (events[i].events & EPOLLHUP) || //描述符被挂断
                    (!(events[i].events & EPOLLIN))) {
                    log_err("epoll error fd: %d", r->fd);
                    close(fd);
                    continue;
                }

                log_info("new data from fd %d", fd);
      //          rc = threadpool_add(tp, do_request, events[i].data.ptr);
      //          check(rc == 0, "threadpool_add");

             do_request(events[i].data.ptr);
            }
        }   //end of for
    }   // end of while(1)
    
/*
   
    if (threadpool_destroy(tp, 1) < 0) {
        log_err("destroy threadpool failed");
    }*/
    

    return 0;
}
