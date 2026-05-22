# Socket-learning

Here in this repo, we are learning how to work with sockets by trying to code a chat_server(host not hub) and chat_client that connect to said host.

## Project Structure

This repository contains two architectural stages of the chat application:

* **[v1/](./v1/) - Single-Threaded Architecture:** A basic chat client and host where communication is sequential ("ping-pong" style). Both components run on single threads using process-level separation.
* **[v2/](./v2/) - Parallelized Architecture (Planned):** An upcoming version designed to break the ping-pong limitation by running the sending and receiving routines in parallel, allowing simultaneous, full-duplex messaging.
## How to Navigate
To compile and run a specific version, navigate into its respective directory and follow the instructions in its local README:

```bash
# To check out Version 1
cd v1/

# To check out Version 2
cd v2/