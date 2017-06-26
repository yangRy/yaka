CC = gcc
CFLAGS = -Wall -g
LIBS = -lpthread
TARGET = yaka
RM = rm -f 
#OBJS = util.o yaka.o http.o http_parse.o http_request.o epoll.o rio.o threadpool.o timer.o priority_queue.o
OBJS = http_request.o http.o http_parse.o timer.o util.o threadpool.o priority_queue.o rio.o yaka.o epoll.o
all:$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)
clean:
	$(RM) $(TARGET) $(OBJS)
