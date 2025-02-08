# DDOS ATTACK

## Overview
A DDoS (Distributed Denial-of-Service) attack is a cyberattack in which an attacker floods a target server, website, or network with excessive internet traffic to disrupt its normal operation. The goal is to overwhelm the system, making it slow or completely inaccessible to legitimate users.

## ⚠️ Warning

> ‼️‼️ ☠️ ‼️‼️ **This project is intended for educational and ethical use only. It must not be used for unauthorized or malicious activities. The developers are not responsible for any misuse of this software. Ensure compliance with legal and ethical standards before deployment.**

## Structure

This project consists of two components:
- **Master (Server)**: A server application that listens for client connections, processes commands, and broadcasts tasks to connected clients.
- **Zombie (Client)**: A client application that connects to the Master, processes the received commands, and executes the assigned tasks.

### Commands Supported
The Master server and Zombie client support the following commands:

- `ADD`: Add a new task entry.
- `EXEC`: Execute tasks sent from the Master.
- `TERMINATE`: Disconnect and terminate the connection with the server.
- `SHUTDOWN`: Shut down the server.

### Requirements

- C++ compiler with support for C++11 or later.
- Boost libraries (for networking).
- cURL library (for HTTP requests in the client).
- `hping3` for flooding requests in the `Zombie` client.

## Master (Server)

### Functionality
- **Server Socket Setup**: The server listens on a specific port (default: `5000`) for incoming connections from clients.
- **Command Handling**: Supports commands like `ADD`, `EXEC`, `TERMINATE`, and `SHUTDOWN` through a simple command-line interface.
- **Client Management**: The server can handle multiple connected clients and send broadcast messages.
  
### Dependencies
- Boost.Asio for asynchronous I/O handling.
- Standard C++ libraries for various functionalities like file handling and string manipulation.

### Key Methods
1. **socket_setup()**: Initializes the server and starts accepting client connections.
2. **accept_connections()**: Listens for incoming client connections.
3. **broadcast()**: Sends a message to all connected clients.
4. **handle_*_command()**: Each command is handled by separate methods such as `handle_add_command()`, `handle_exec_command()`, etc.

### Example Usage
1. Compile and run the server:
   ```bash
   g++ -std=c++11 -o master master.cpp -lboost_system -lpthread
   ./master
   ```
2. Enter the command prompt and execute commands like:
   - **`A`**: Add a new task entry.
   - **`E`**: Execute tasks.
   - **`T`**: Terminate the current session.
   - **`S`**: Shutdown the server.

# Zombie (Client)

## Functionality

- **Task Execution**: The client connects to the server, receives tasks, and executes them based on the provided instructions.
- **Task Scheduler**: The client uses a task scheduler to delay task execution and run them at a specified time.
- **HTTP Requests**: It can make HTTP requests using cURL or run system commands to flood a URL.

## Dependencies

- **cURL** library for HTTP requests.
- **hping3** for generating requests.
- **TaskScheduler** class for scheduling and running tasks.

## Key Methods

- **connect_to_server()**: Connects to the server using TCP.
- **run_task()**: Executes the task by scheduling it for execution at the right time.
- **get_tasks()**: Continuously receives commands from the server and processes them.
- **process_tasks()**: Determines which task (`ADD`, `EXEC`, `TERMINATE`) to process based on the received command.

## Example Usage

### Build Instructions

To build the server and client, follow these steps:

### 1. Install Boost Libraries

```bash
sudo apt-get install libboost-all-dev
```

### 2. Install cURL

```bash
sudo apt-get install libcurl4-openssl-dev
```

### 3. Build the Project

```bash
make clean && make
```
This command will clean the previous build and then compile the project.

### 4. Run the Master (Server)

```bash
./master
```

### 5. Run the Zombie (Client)

```bash
./zombie
```

## 6. Deploy in Kubernetes for Client and Server

### Deploy Server
```bash
make
docker build -t cpp_server .
docker run -p 5000:5000 cpp_server
```

### Deploy Client
```bash
docker build -t cpp_client .
docker run -p 5001:5001 cpp_client
```