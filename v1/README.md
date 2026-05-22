### Getting Started

### Prerequisites
Make sure you have a C compiler (like `gcc`)

### Compilation
Before running the programs, ensure the output directory exists and compile the source files:

```bash
# Create the binaries directory if it doesn't exist
mkdir -p bin/

# Compile the programs
gcc src/chat_server.c -o bin/chat_server
gcc src/chat_client.c -o bin/chat_client
```

## Running the Programs

Because these programs need to run at the same time, you will need to open **two separate terminal windows**.

> 💡 **Important Note:** The **client** is the one who gets to type first. Once both programs are running, switch to the client terminal to begin inputting data
 
### Terminal 1
In your first terminal, run the chat_server program along with a port number of your choosing range[1024 , 65535]:
```bash
bin/chat_server <port-number>
```

### Terminal 2
In your 2nd terminal, run the chat_client program along with the `<server-ip-address>` you wanna connect to (since chat_server runs in the same machine, we use 127.0.0.1) and give it the same port number you ran the chat_server program with:

```bash
bin/chat_client 127.0.0.1 <port-number>
```
