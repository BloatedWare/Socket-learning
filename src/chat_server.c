#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> //added this just for aesthetics 
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DEFAULT_QUEUE_LENGTH 10
#define MAX_Q_LENGTH 128

#define BAD_PORT -10
#define SOCKET_FAILED -11
#define INSUFFICIENT_ARGS -12
#define BIND_FAILED -13
#define BAD_Q_LENGTH -14
#define LISTEN_FAILED -15
#define FORK_FAILED -16

int get_port(const char* str);
int get_q_length(const char* str);
void chats();// will define later

int main(int argc, char** argv) {
    int queue_length = DEFAULT_QUEUE_LENGTH;
    int server_port, client_port;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;//to capture client address
    socklen_t client_struct_len = sizeof(client_address);//need as well for accept
    char client_ip_str[INET_ADDRSTRLEN];//255.255.255.255 is 15 characters + \0 is 16
    char buffer[1024];
    
    

    if (argc < 2 || argc > 3) {
        printf("Usage: %s <port-number> [queue-length=%d]\n", argv[0], queue_length);
        exit(INSUFFICIENT_ARGS);
    }
    
    server_port = get_port(argv[1]);

    if (argc == 3) {
        queue_length = get_q_length(argv[2]);
    } 


    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sd < 0) {
        perror("socket");
        exit(SOCKET_FAILED);
    }


    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((u_int16_t)server_port);// port must be 16 bits AND in big endian, which our system stores in little endian
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);//macro for 127.0.0.1 in binary and htonl to convert to big endian (same as port)

    if (bind(sd, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        perror("bind");
        exit(BIND_FAILED);
    }

    if (listen(sd, queue_length) != 0) {
        perror("listen");
        exit(LISTEN_FAILED);
    }

    printf("chat server open on port: %d...\n", server_port);

    while (true) {
        int connection_sd = accept(sd, (struct sockaddr*)&client_address, &client_struct_len);//I can do this because i don't wanna store the client address and their port
        //will change this later
        if (connection_sd == -1) {
            printf("Failed to establish connection!\n");
            //decided to not exit because i don't think servers should crash because of one failed client connection
        } else {
             
            switch (fork()) {
                case -1:
                    printf("Failed to fork new process for client...aborting\n");
                    //same reason as the above, no need to kill the server yet for 1 failed connection
                    break;
                case 0: 
                    inet_ntop(AF_INET, &(client_address.sin_addr), client_ip_str ,INET_ADDRSTRLEN);
                    client_port = ntohs(client_address.sin_port);
                    printf("Client connected [IP: %s | PORT: %d ]\n", client_ip_str, client_port);
                    //here i need to specifiy or implement the chatting feature :|
                    break;
                default:// I am parent
                    printf("Connection established! :)\n");
                    break;
            }
        }

    }


    close(sd);

    return 0;
}

int get_port(const char* str) {
    int port;
    const char* str_head;
    if (str == NULL) return BAD_PORT;
    while (*str == ' ') str++;//skip white space
    str_head = str;//point to first non white space char

    do { //will return false for (+/-) which i want
        if (!(*str >= '0' && *str <= '9')) {
            printf("Port: Not a number!\n");
            exit(BAD_PORT);
        }
        str++;
    } while (*str != '\0');
    port = atoi(str_head);

    if (port < 1024 || port > 65535) {
        printf("Port: port must be within [1024, 65535]!\n");
        exit(BAD_PORT); //we are only allowed from 1024 and up
    }
    return port;
}

int get_q_length(const char* str) {
   int q_length;
    const char* str_head;
    if (str == NULL) return BAD_PORT;
    while (*str == ' ') str++;//skip white space
    str_head = str;//point to first non white space char

    do { //will return false for (+/-) which i want
        if (!(*str >= '0' && *str <= '9')) {
            printf("queue length: not a number!\n");
            exit(BAD_Q_LENGTH);
        } 
        str++;
    } while (*str != '\0');
    q_length = atoi(str_head);

    if (q_length < 0 || q_length > MAX_Q_LENGTH) {
        printf("queue length must be non negative and musn't surpass %d!\n", MAX_Q_LENGTH);
        exit (BAD_Q_LENGTH); //we are only allowed from 1024 and up
    }
    

    return q_length; 
}