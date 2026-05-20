#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#define BAD_PORT -10
#define SOCKET_FAILED -11
#define INSUFFICIENT_ARGS -12

int get_port(const char* str) {
    int port;
    const char* str_head;
    if (str == NULL) return BAD_PORT;
    while (*str == ' ') str++;//skip white space
    str_head = str;//point to first non white space char

    do { //will return false for (+/-) which i want
        if (!(*str >= '0' && *str <= '9')) return BAD_PORT;
        str++;
    } while (*str != '\0');
    port = atoi(str_head);

    if (port < 1024 || port > 65535) return BAD_PORT; //we are only allowed from 1024 and up
    return port;
}

void chats();

int main(int argc, char** argv) {

    int port;
    struct sockaddr_in address;
    char buffer[1024];

    if (argc != 2) {
        printf("Usage: %s <port-number>\n", argv[0]);
        exit(INSUFFICIENT_ARGS);
    }
    port = get_port(argv[1]);
    printf("port: %d\n", port);


    // int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // if (sd < 0) {
    //     perror("socket");
    //     exit(SOCKET_FAILED);
    // }

    // address.sin_family = AF_INET;
    // address.sin_port = htons();


    return 0;
}