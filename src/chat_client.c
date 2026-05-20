#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> //added this just for aesthetics 
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DEFAULT_QUEUE_LENGTH 10
#define MAX_Q_LENGTH 128
#define MAX_BUFFER_SIZE 1024

#define BAD_PORT -10
#define SOCKET_FAILED -11
#define BAD_ARGS -12
#define MALLOC_FAILED -17
#define BAD_IP_ADDRESS -18
#define CONNECT_FAILED -19

//MARK:PROTOTYPES
int get_port(const char* str);
void chat_client(int sd);
char *get_string(const char* prompt);


//MARK:MAIN
int main(int argc, char** argv) {
    int server_port, sd;
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    //apparent according to gemini, the OS expects the sockaddr_in to have it's padding (it inside of the struct 0 initialized) 
    //or it will potentially reject it

    if ( argc != 3) {
        printf("Usage: %s <ip-address> <server-port>\n", argv[0]);
        exit(BAD_ARGS);
    }
    
    server_port = get_port(argv[2]);
    server_address.sin_port = htons((u_int16_t)server_port);
    server_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) != 1) {//convert ip address string to network
        printf("Bad IP address!\n");
        exit(BAD_IP_ADDRESS);
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(SOCKET_FAILED);
    }

    if (connect(sd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        printf("Failed to establish connection!\n");
        perror("connect");
        exit(CONNECT_FAILED);
    }

    printf("Connection to server successful!\n");
    chat_client(sd);

    

    return 0;
}


//MARK: DEFINITIONS
void chat_client(int sd) {
    char server_buffer[MAX_BUFFER_SIZE];//server_buffer is our receive_buff
    char* msg_to_send;
    int msg_length;
    int bytes_read, bytes_sent;
    bool terminate_session = false;//added this due to server input (look from 145)

    printf("TIP: /exit to end session!\n");


    while(true) {
        
        while(true) {
        //server side communication
            msg_to_send = get_string("Client: ");//remember to free if you reach here
            msg_length = strlen(msg_to_send);
            if (msg_length >= MAX_BUFFER_SIZE || msg_length <= 0) {
                free(msg_to_send);//me no forget :'( so no memory leak
                printf("message too long! ( 1 <= msg_length <= %d)\n", MAX_BUFFER_SIZE-1);
                fflush(stdout);
                continue;
            } else {
                if (!strncmp(msg_to_send, "/exit", 5)) {
                    free(msg_to_send);//me no forget :'( so no memory leak
                    printf("Client ended session!\n");
                    fflush(stdout);

                    terminate_session = true;
                    break;
                }

                //send to server
               
                bytes_sent = send(sd, msg_to_send, msg_length+1, 0);//msg_length + 1 cuz of \0, I think i don't need server_buff, 0 flag makes it behave like write
                if (bytes_sent < 0) {
                    printf("send failed");//also potentially server closed
                    fflush(stdout);

                    free(msg_to_send);
                    terminate_session = true;
                    break;//exit the current loop
                } 

                free(msg_to_send);//free this array for now until next input
                break;//terminate session will still be false so it will now look at client input once again
                
            }
        }

        if(terminate_session) {
            break;//leave while loop
        }


        //receiving client message, i think i will make client only send 1023 length msgs
        bytes_read = recv(sd, server_buffer, MAX_BUFFER_SIZE-1, 0);
        
        if(bytes_read <= 0) {
            printf("recv failed!\n");
            fflush(stdout);
            break;//this will exit the loop into close(connection_sd); 0 means server vanished
        } 
        //moved this here cuz of potential client_buffer[-1] -> segfault
        server_buffer[bytes_read] = '\0';//null terminate after last byte received

        printf("Server: %s\n", server_buffer);
        fflush(stdout);

        if (!strncmp(server_buffer, "/exit", 5)) {//reads first 5 bytes and doesn't care what's after
            printf("Server ended session\n");
            break;//leave instantly, don' waste time waiting for server side input
        }

    }
    
    close(sd);//close connection socket
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

char *get_string(const char* prompt) {
    char *str = (char*)malloc(sizeof(char));//for '\0'
    char *temp = NULL;
    int c;
    int size = 1;
    printf("%s", prompt);

    while ((c = getchar()) != '\n' ) {
        str[size-1] = c;
        temp = (char*)realloc(str ,sizeof(char)*(size+1));
        if (temp == NULL) {
            free(str);//free the memory cuz it turns out realloc doesn't free the memory if it fails aka memory leak
            perror("malloc");
            exit(MALLOC_FAILED);//will kill child process and terminate session (idk if I should kill the parent or no)
        }
        str = temp;//now assign it to str
        size++;
    }
    str[size-1] = '\0';
    return str;
}