#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> //added this just for aesthetics 
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

#define DEFAULT_QUEUE_LENGTH 10
#define MAX_Q_LENGTH 128
#define MAX_BUFFER_SIZE 1024


bool recv_thread_alive = false;
bool send_thread_alive = false;

//MARK:PROTOTYPES

void chat_client(int sd);
char *get_string(const char* prompt);
void* recv_thread_entry(void* arg);
void* send_thread_entry(void* arg);

//MARK:MAIN
int main(int argc, char** argv) {
    int server_port, sd;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    //apparent according to gemini, the OS expects the sockaddr_in to have it's padding (it inside of the struct 0 initialized) 
    //or it will potentially reject it

    if ( argc != 3) {
        printf("Usage: %s {domain-name | ip-address} <server-port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0; //choose appropriate protocol for the socket type

    if (getaddrinfo(argv[1], argv[2], &hints, &res) != 0) {//first arg is domain name or ip, 2nd arg is port or service(default port for service), hints is the template, res stores the resulted ips after dns lookup with families included
        printf("Domain name , ip address or port number are invalid!\n");
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }
    /*
    getaddr info now does our work for us, it will resolve the domain name 
    and get ip address (and its family), 
    put the port num we gave it in big endian AND put our socktype (TCP) as specified in hints 
    for us and get a list of addresses we can try to connect to (really cool)

    */

    while(true) {
        if (res != NULL && (sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            res = res->ai_next;
            continue;
        }
        if (res != NULL && connect(sd, res->ai_addr, res->ai_addrlen) != 0) {
            res = res->ai_next;
            continue;;
        }

        break;
    }

    if (res == NULL) {
        printf("Failed to connect!\n");
        perror("socket or bind");
        exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);//will kill child process and terminate session (idk if I should kill the parent or no)
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
