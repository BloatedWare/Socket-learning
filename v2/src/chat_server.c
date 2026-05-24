#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> //added this just for aesthetics 
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define DEFAULT_QUEUE_LENGTH 1//since only 1 client at a time anyway
#define MAX_Q_LENGTH 128
#define MAX_BUFFER_SIZE 1024

#define BAD_PORT -10
#define SOCKET_FAILED -11
#define INSUFFICIENT_ARGS -12
#define BIND_FAILED -13
#define BAD_Q_LENGTH -14
#define LISTEN_FAILED -15
#define FORK_FAILED -16
#define MALLOC_FAILED -17
#define BAD_IP_ADDRESS -18
#define SEND_FAILED -20
#define SESSION_END -21
#define RECV_FAILED -22

//MARK:PROTOTYPES
int get_port(const char* str);
// int get_q_length(const char* str);
void chat_service(int sd, struct sockaddr_in client_addr);// will define later
char *get_string(const char* prompt);
void* send_thread_entry(void* arg);
void* recv_thread_entry(void* arg);

int recv_thread_alive = false;
int send_thread_alive = false;


//MARK:MAIN
int main(int argc, char** argv) {
    int queue_length = DEFAULT_QUEUE_LENGTH;
    int server_port;
    int interface;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;//to capture client address

    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(server_address));
    //these must be 0 initialized or the OS might drop them due to padding (a char array inside) being non 0 (idk)

    socklen_t client_struct_len = sizeof(client_address);//need as well for accept
    

    if (argc < 2 || argc > 3) {
        // printf("Usage: %s <port-number> [queue-length=%d]\n", argv[0], queue_length);
        printf("Usage: %s <port-number> [ip-address]\n", argv[0]);

        exit(INSUFFICIENT_ARGS);
    }
    
    server_port = get_port(argv[1]);


    if (argc == 3) {

        if (inet_pton(AF_INET, argv[2], &server_address.sin_addr) != 1 ) {
            printf("Bad IP address!\n");
            exit(BAD_IP_ADDRESS);
        } 
        
    } else {
        server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }

    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sd < 0) {
        perror("socket");
        exit(SOCKET_FAILED);
    }


    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((u_int16_t)server_port);// port must be 16 bits AND in big endian, which our system stores in little endian
    // server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);//macro for 127.0.0.1 in binary and htonl to convert to big endian (same as port)

    if (bind(sd, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) {
        perror("bind");
        exit(BIND_FAILED);
    }

    if (listen(sd, queue_length) != 0) {//honestly I added this for multiclient receptient but I m gonna comment this out for now
        perror("listen");
        exit(LISTEN_FAILED);
    }

    printf("\n\n");
    while (true) {
        printf("chat server listening on port: %d...\n[Ctrl+C] to exit\n", server_port);
        int connection_sd = accept(sd, (struct sockaddr*)&client_address, &client_struct_len);//I can do this because i don't wanna store the client address and their port
        //will change this later
        if (connection_sd == -1) {
            printf("Failed to establish connection!\n");
            //decided to not exit because i don't think servers should crash because of one failed client connection
        } else {
            printf("New connection established!\n");
            chat_service(connection_sd, client_address); 
        }

    }

    close(sd);//close listening socket

    return 0;
}


//MARK: DEFINITIONS
void chat_service(int connection_sd, struct sockaddr_in client_addr) {
    
    pthread_t send_thread, recv_thread;
    int client_port;
    char client_ip_str[INET_ADDRSTRLEN];//255.255.255.255 is 15 characters + \0 is 16

    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip_str ,INET_ADDRSTRLEN);//function that takes the binary form the the address and turns it into string
    //it's reverse is inet_pton() 
    client_port = ntohs(client_addr.sin_port);//turns big endian back to little endian (forgot to add this last commit)
    printf("Client connected [IP: %s | PORT: %d ]\nTIP:'/exit' to end session\n", client_ip_str, client_port);
    //here i need to specifiy or implement the chatting feature :|

    send_thread_alive = true;
    recv_thread_alive = true;
    pthread_create(&send_thread, NULL, (void* (*)(void*))send_thread_entry, (void*)&connection_sd);
    pthread_create(&recv_thread, NULL, (void* (*)(void*))recv_thread_entry, (void*)&connection_sd);

    pthread_join(send_thread, NULL);
    pthread_cancel(recv_thread);
    pthread_join(recv_thread, NULL);
    close(connection_sd);//same behavior as client, if send dies, kill recv() thread, if recv dies, send() will die on its own (after key press)
    
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
            printf("Client disconnected!\n");
            printf("Press any key to exit...\n");
            recv_thread_alive = false;  
            return NULL;//exit thread to perform clean up
        }
        
        printf("\nClient: %s\n>>> ", recv_buffer);
        fflush(stdout);
    } 

}

void* send_thread_entry(void* arg) {
    int sd = *(int*)arg;
    char* server_msg;//renamed it to client msg, cuz i keep accidently free msg_length when i press TAB
    int msg_len;
    int bytes_sent = 0;
    while (true) {
        
        server_msg = get_string(">>> ");
        if(!recv_thread_alive) {
            free(server_msg);
            send_thread_alive = false;  
            return NULL;
        }
        msg_len = strlen(server_msg);
        if (msg_len > 0 && msg_len < MAX_BUFFER_SIZE) {
            bytes_sent = send(sd, server_msg, msg_len, 0);
            if (bytes_sent == -1) {
                perror("send");
                free(server_msg);
                send_thread_alive = false;
                close(sd);
                *(int*)arg = -1;
                return NULL;
            }

            if (!strncmp(server_msg, "/exit", 5)) {
                free(server_msg);
                printf("You have ended the session!\n");
                send_thread_alive = false;
                close(sd);
                *(int*)arg = -1;
                return NULL;
            }
            
        } else {
            printf("BAD MESSAGE! ( 1 <= msg_length <= %d)\n", MAX_BUFFER_SIZE-1);
        }
        free(server_msg);

    }
}