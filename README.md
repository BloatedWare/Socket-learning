# Socket-learning

Here in this repo, we are learning how to work with sockets by trying to code a chat_server(host not hub) and chat_client that connect to said host.

## Project Structure

This repository contains two architectural stages of the chat application:

* **[v1/](./v1/) - Single-Threaded Architecture:** A basic chat client and host where communication is sequential ("ping-pong" style). Both components run on single threads using process-level separation.
* **[v2/](./v2/) - Parallelized Architecture (Planned):** An upcoming version designed to break the ping-pong limitation by running the sending and receiving routines in parallel, allowing simultaneous, full-duplex messaging.
*  **[v3/](./v3/) - Domain Name Architecture (Planned):** An upcoming version where we update the client to take domain names instead of just raw IP addresses, allowing clients from the web to connect.
---

## How to Navigate

To compile, run, or view the roadmap for a specific version, navigate into its respective directory and follow the instructions in its localized `README.md`:

```bash
# To check out the current Single-Threaded baseline
cd v1/

# To check out the Parallelized layout
cd v2/

# To check out the Domain-Name Ready layout
cd v3/