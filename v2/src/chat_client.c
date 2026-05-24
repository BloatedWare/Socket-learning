#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> //added this just for aesthetics 
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define DEFAULT_QUEUE_LENGTH 10
#define MAX_Q_LENGTH 128
#define MAX_BUFFER_SIZE 1024

#define BAD_PORT -10
#define SOCKET_FAILED -11
#define BAD_ARGS -12
#define MALLOC_FAILED -17
#define BAD_IP_ADDRESS -18
#define CONNECT_FAILED -19
#define SEND_FAILED -20
#define SESSION_END -21
#define RECV_FAILED -22

bool recv_thread_alive = false;
bool send_thread_alive = false;

//MARK:PROTOTYPES
int get_port(const char* str);
void chat_client(int sd);
char *get_string(const char* prompt);
void* recv_thread_entry(void* arg);
void* send_thread_entry(void* arg);

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
    pthread_t recv_thread;
    pthread_t send_thread;

    recv_thread_alive = true;
    send_thread_alive = true;
    //i know i should these inside of the thread but i don't know how to use wait and notify equivalent in C yet
    pthread_create(&recv_thread, NULL, (void* (*)(void*))recv_thread_entry, (void*)&sd);
    pthread_create(&send_thread, NULL, (void* (*)(void*))send_thread_entry, (void*)&sd);
    
    pthread_join(send_thread, NULL);
    pthread_cancel(recv_thread);//kill the recv since send is dead, in the opposite case (recv() thread dies), send() will die after key press
    pthread_join(recv_thread, NULL);
    close(sd);
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

    while ((c = getchar()) != '\n') {
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



void* recv_thread_entry(void* arg) {
    
    int sd = *(int*)arg;//copy value at address which is sd
    int bytes_read = 0;
    int exit_status;
    char recv_buffer[MAX_BUFFER_SIZE];
    while (true) {
        if (!send_thread_alive) {//always check if the other thread is alive 
            recv_thread_alive = false;
            
            return NULL;//will return to main thread where the socket will close
        }
        bytes_read = recv(sd, recv_buffer, MAX_BUFFER_SIZE-1, 0);
        if (bytes_read == -1) {
            perror("recv");
            recv_thread_alive = false;
            
            return NULL;// threads return a generic pointer, i've set it to NULL for now
        } 
        recv_buffer[bytes_read] = '\0';
    

        if (bytes_read == 0 || !strncmp(recv_buffer, "/exit", 5) ){
            printf("Disconnected from server.\n");
            printf("Press any key to exit...\n");
            recv_thread_alive = false;
            
            return NULL;//exit thread to perform clean up
        }
        
        printf("\nServer: %s\n>>> ", recv_buffer);
        fflush(stdout);
            
    } 

}

void* send_thread_entry(void* arg) {
    int sd = *(int*)arg;
    char* client_msg;//renamed it to client msg, cuz i keep accidently free msg_length when i press TAB
    int msg_len;
    int bytes_sent = 0;
    
    while (true) {
        
        client_msg = get_string(">>> ");
        if(!recv_thread_alive) {
            free(client_msg);
            send_thread_alive = false;
            
            return NULL;
        }

        msg_len = strlen(client_msg);
        if (msg_len > 0 && msg_len < MAX_BUFFER_SIZE) {
            bytes_sent = send(sd, client_msg, msg_len, 0);
            if (bytes_sent == -1) {
                perror("send");
                free(client_msg);
                send_thread_alive = false;
                return NULL;
            }

            if (!strncmp(client_msg, "/exit", 5)) {
                free(client_msg);
                printf("You have ended the session!\n");
                send_thread_alive = false;
                return NULL;
            }
            
        } else {
            printf("BAD MESSAGE! ( 1 <= msg_length <= %d)\n", MAX_BUFFER_SIZE-1);
        }
        free(client_msg);

    }
}
