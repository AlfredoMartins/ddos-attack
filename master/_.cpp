#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <mutex>
#include <fstream>
#include <map>

#define PORT 5000

std::map<std::string, int> COMMAND_MAP = {
    {"ADD", 0},
    {"EXEC", 1},
    {"TERMINATE", 2},
    {"SHUTDOWN", 3}
};

std::map<int, std::string> REVERSE_COMMAND_MAP = {
    {0, "ADD"},
    {1, "EXEC"},
    {2, "TERMINATE"},
    {3, "SHUTDOWN"}
};

struct ChronoLink {
    std::string url;
    time_t time;
    int code;
    int duration;

    std::string get_command() {
        std::stringstream ss;
        ss << REVERSE_COMMAND_MAP[code] << " | " << url << " | " << std::to_string(time) << " | " << code << " | " << duration;
        return ss.str();
    }

    std::string trim(const std::string& s) {
        std::string trimmed = s;
        trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
        return trimmed;
    }

    ChronoLink read_command(const std::string& command) {
        std::stringstream ss(command);
        std::string token;
        std::vector<std::string> parts;

        while (std::getline(ss, token, '|')) {
            parts.push_back(trim(token));
        }

        if (parts.empty()) {
            std::cerr << "Command is empty." << std::endl;
            return {};
        }

        if (parts[0] == "EXEC") {
            ChronoLink link;
            link.code = 1;
            return link;
        }

        if (parts.size() == 5) {
            try {
                std::string url = parts[1];
                time_t time = std::stoi(parts[2]);
                int code = std::stoi(parts[3]);
                int duration = std::stoi(parts[4]);
                ChronoLink link = {url, time, code, duration};
                std::cout << "COMMAND EXECUTED: " << link.get_command() << std:: endl;
                return link;
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid argument: " << e.what() << std::endl;
            } catch (const std::out_of_range& e) {
                std::cerr << "Out of range: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Failed to parse the values correctly." << std::endl;
        }

        return {};
    }
};

class Master {
private:
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    std::vector<std::thread> threads;
    std::vector<int> clients;
    std::vector<ChronoLink> links;
    std::mutex clients_mutex;
    bool running;

    void trim(std::string &s) {
        s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
    }

    int read_int_from_user(int default_value = 0) {
        std::string input = read_string_from_user();
        try {
            return input.empty() ? default_value : std::stoi(input);
        } catch (...) {
            return default_value;
        }
    }

    time_t read_time_from_user() {
        std::cout << "Enter time (YYYY-MM-DD HH:MM:SS, empty for now): ";
        std::string time_input = read_string_from_user();
        if (time_input.empty()) return time(0);

        struct tm tm = {};
        std::stringstream ss(time_input);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (ss.fail()) return time(0);
        return mktime(&tm);
    }

    std::string read_string_from_user() {
        std::string input;
        std::getline(std::cin, input);
        trim(input);
        return input;
    }

    char upper(char c) {
        return std::toupper(static_cast<unsigned char>(c));
    }

    std::string read_string_from_file(std::ifstream& file) {
        std::string result;
        std::getline(file, result, '|');
        return result;
    }

    time_t read_time_from_file(std::ifstream& file) {
        std::string time_str;
        std::getline(file, time_str, '|');

        struct tm tm = {};
        std::stringstream ss(time_str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (ss.fail()) return time(0);
        return mktime(&tm);
    }

    int read_int_from_file(std::ifstream& file) {
        std::string int_str;
        std::getline(file, int_str, '|');

        try {
            return std::stoi(int_str);
        } catch (...) {
            return 0;
        }
    }

    int read_int_from_file(std::ifstream& file, int default_value) {
        std::string int_str;
        std::getline(file, int_str, '|');  // Read until '|'

        try {
            return int_str.empty() ? default_value : std::stoi(int_str);
        } catch (...) {
            return default_value;
        }
    }

public:
    Master() : running(true) {}

    void socket_setup() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 10) < 0) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << PORT << std::endl;
        show_navigation_menu();

        while (running) {
            int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket < 0) {
                perror("accept failed");
                continue;
            }
            std::cout << "New client connected\n";

            // Locking the mutex for thread safety
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(new_socket);

            threads.emplace_back([this, new_socket]() { handle_client(new_socket); });
        }
    }

    void handle_client() {
        while (true) {
            std::cout << "Enter the command: " << std::endl;
            std::string command = read_string_from_user();
            if (command.empty()) break;

            switch (upper(command[0])) {
                case 'A':
                    handle_add_command();
                    break;
                case 'E':
                    handle_exec_command();
                    break;
                case 'S':
                    handle_shutdown_command();
                    return;
                case 'T':
                    handle_terminate_command();
                    return;
                case 'C':
                    handle_list_command();
                    break;
                default:
                    send_invalid_command_response();
                    break;
            }
        }

    }

    void handle_add_command(int client_socket) {
        std::ifstream file("file.txt");

        if (!file) {
            std::cerr << "File does not exist. Creating file with sample data.\n";

            std::ofstream new_file("file.txt");

            if (new_file) {
                new_file << "http://example.com|1970-01-01 00:00:00|0|1\n";
                std::cerr << "Sample data written to file.txt.\n";
            } else {
                std::cerr << "Unable to create the file.\n";
                return;
            }

            new_file.close();

            file.open("file.txt");
        }

        std::string url = read_string_from_file(file);
        time_t time = read_time_from_file(file);
        int code = read_int_from_file(file);
        int duration = read_int_from_file(file, 600);

        file.close();

        ChronoLink link = {url, time, code, duration};
        links.push_back(link);
        std::string COMMAND = link.get_command();
        std::string message = "Added new entry: " + COMMAND;
        send(client_socket, COMMAND.c_str(), COMMAND.size(), 0);
    }

    void handle_exec_command(int client_socket) {
        std::string message = "EXEC|_|_|_|_";broadcast_message
    }

    void handle_shutdown_command(int client_socket) {
        std::cout << "Server shutting down." << std::endl;
        std::string msg = "SHUTDOWN|_|_|_|_";
        broadcast_message(msg);
        running = false;
        close(server_fd);
        exit(0);
    }

    void handle_terminate_command(int client_socket) {
        std::string msg = "TERMINATE|_|_|_|_";
        broadcast_message(msg);
        close(client_socket);
    }

    void handle_list_command() {
        std::lock_guard<std::mutex> lock(clients_mutex);
        int alive_clients = 0;
        clients.erase(std::remove_if(clients.begin(), clients.end(),
            [&](int client_socket) { return !is_client_alive(client_socket); }), clients.end());
        alive_clients = clients.size();
        std::cout << "Total clients: " << alive_clients << std::endl;
    }

    void show_navigation_menu() {
        std::string msg = "COMMANDS:\n"
                          "\tA - Add entry (ADD)\n"
                          "\tE - Execute (EXEC)\n"
                          "\tS - Stop server (STOP)\n"
                          "\tT - Terminate session (TERMINATE)\n"
                          "\tC - Connected clients (CONNECT)\n"
                          "\tX - Exit (EXIT)";
        std::cout << msg << std::endl << std::flush;
    }

    void send_invalid_command_response() {
        std::string msg = "Invalid command. Please try again.";
        broadcast_message(msg);
    }

    bool is_client_alive(int clientSocket) {
        return send(clientSocket, "", 1, MSG_NOSIGNAL) != -1;
    }

    void broadcast_message(const std::string &message) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (int i = 0; i < clients.size(); i++) {
            int client_socket = clients.at(i);
            if (is_client_alive(client_socket)) {
                ssize_t sent = send(client_socket, message.c_str(), message.size(), 0);
                if (sent == -1) {
                    std::cerr << "Failed to send message to client. Closing socket." << std::endl;
                    close(client_socket);
                    clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
                }
            }
        }
    }

    ~Master() {
        for (auto &t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        close(server_fd);
    }
};

int main() {
    Master server;
    server.socket_setup();
    return 0;
}