# Freescord

**Freescord** is a real-time messaging application written in C, allowing multiple users to communicate through a central server. The project uses **TCP/IP sockets** and **POSIX threads** to handle concurrent connections.

---

## 🚀 Features

- **TCP/IP connection** between client and server
- **Multi-client management** with dedicated threads
- **Pseudonym assignment and validation**
- **Broadcasting messages** to all connected clients
- **Automatic CRLF ↔ LF conversion** for network/Unix compatibility
- **Buffered reading** for improved performance
- **Event and message logging** on the server side (with timestamps)
- **Basic user interface** with visual notifications (colors)
- **Special text commands** (ASCII emojis): `/tableflip`, `/unflip`, `/shrug`
- **Personalized display**: current user's messages shown in gray
- **Keyboard shortcuts support** (e.g. Ctrl-D to quit)

---

## ⚙️ Architecture

### Server

- Accepts multiple simultaneous connections
- Creates a detached thread for each client
- A "repeater" thread broadcasts messages to all clients
- Internal pipe ensures thread synchronization
- Mutex protects the list of active clients

### Client

- Connects to the server and chooses a pseudonym
- Uses `poll()` to handle both user input and server messages
- Displays messages with visual formatting (colors, notifications)
- Supports special commands and error management

---

## 📡 Communication Protocol

1. **Connection**: Server sends a welcome message
2. **Pseudonym**: Client sends `/nickname <pseudo>`  
   Possible responses:
   - `0`: accepted
   - `1`: already in use
   - `2`: invalid (too long or contains forbidden characters)
   - `3`: incorrect command format
3. **Messaging**:
   - Messages are prefixed with the sender's pseudonym
   - The sender receives their own messages prefixed with `[you]`

---

## 💡 Technical Highlights

- **Detached threads**: resources are automatically freed on disconnection
- **Mutex** on client list to avoid concurrent access
- **Custom buffer** for efficient reading (see `buffer/buffer.c`)
- **CRLF/LF conversion**: ensures network/Unix compatibility
- **Signal handling** to prevent crashes on disconnections

---

## 🖥️ Quickstart

```bash
git clone https://github.com/NOT16180/freescord.git
cd freescord
make          # Compile server and client
./srv         # Start the server
./clt         # Start a client (then enter server IP)
```

**Example usage:**

```
/nickname Alice
Hello everyone!
/tableflip
```

---

## 🧪 Build & Tests

A Makefile simplifies building and testing.

### Main variables

- `CC = gcc` — C compiler
- `CFLAGS = -g -Wall -Wvla -std=c99 -pthread -D_XOPEN_SOURCE=700` — compile flags
- `LDFLAGS = -pthread -Wall` — linker flags

### Main targets

- `all` — builds `srv` (server) and `clt` (client)
- `srv` — builds the server from `serveur.o`, `list.o` and `user.o`
- `clt` — builds the client from `client.o`, `buffer.o` and `utils.o`
- `test` — runs unit tests on the list (with Valgrind for memory leaks)
- `clean` — removes compiled files and executables

---

## 📂 Repository Structure

```
freescord/
├── buffer/         # Buffered I/O (custom buffer implementation)
├── list/           # Linked list implementation for clients
├── server.c        # Server source code
├── client.c        # Client source code
├── utils.c         # Utility functions
├── Makefile        # Build and test script
└── README.md
```

---

## 📚 External Resources

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [GNU documentation on poll()](https://www.gnu.org/software/libc/manual/html_node/Nonblocking-I_002fO.html)

---

## ⚠️ Requirements

- **POSIX system** (Linux, macOS, etc.)
- **GCC** and **pthread**
- Local network access for multi-client tests

---

## 📄 License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.

---

## 🤝 Contributing

Contributions for bug fixes, new features or documentation improvements are welcome!  
Open an _issue_ or submit a _pull request_.

---
