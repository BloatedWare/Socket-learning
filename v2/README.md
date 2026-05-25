# Version 2: Parallelized Chat (Using Threads)

Welcome to Version 2! In `v1`, the chat was stuck in a strict "ping-pong" loop where you had to take turns typing. 

In `v2`, the program uses threads so that **sending** and **receiving** happen at the exact same time. You can type whenever you want, and messages pop up instantly.

### How it works under the hood
Instead of doing everything in one loop, the main program starts up **two separate threads**:
1. **Send Thread:** Runs your input function in a loop, waits for you to type, and sends it out.
2. **Receive Thread:** Sits and waits for incoming network data, then prints it to the screen instantly.

The main program just starts these two threads and waits for them to finish. If one the **Send Thread** closes down then we kill the recv thread, if the opposite happens, the **Send Thread** closes down on it's own after key press.

---

### Prerequisites
Make sure you have a C compiler installed (like `gcc`).

### Compilation
```bash
# Make sure the bin folder exists
mkdir -p bin/

# Compile the server and client
gcc -pthread src/chat_server.c -o bin/chat_server
gcc -pthread src/chat_client.c -o bin/chat_client 
```

## Running the Programs

Because these programs need to run at the same time, you will need to open **two separate terminal windows**.

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
