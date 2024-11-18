#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define MY_BUF_SZ 256

#define FILE_PATH "/dev/my_dev_0"
#define FILE_PATH2 "test2"

// test open, read, write system call
void test() {
    int fd;

    fd = open(FILE_PATH, O_WRONLY);
    char buf[MY_BUF_SZ] = "Hello World";
    int len = strlen(buf);

    int wret = write(fd, buf, len);
    printf("Write %d bytes to fd\n", wret);
    close(fd);

    fd = open(FILE_PATH, O_RDONLY);
    char recv_buf[MY_BUF_SZ];
    int rret = read(fd, recv_buf, len);
    recv_buf[rret] = '\0';
    printf("Read %d bytes from fd: %s\n", rret, recv_buf);
    close(fd);
}

// test synchronization for opening 2 files simultaneously 
void test2() {
    int fd;
    fd = open(FILE_PATH, O_RDONLY);
    sleep(15);
    close(fd);
}

int main() {
    test();
}