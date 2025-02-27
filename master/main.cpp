#include <iostream>
#include <vector>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <mutex>
#include <fstream>
#include <map>
#include <set>
#include <boost/asio.hpp>
#include <thread>

#define PORT 5000

using boost::asio::ip::tcp;

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
    boost::asio::io_context io_context;
    tcp::acceptor acceptor;
    std::vector<std::shared_ptr<tcp::socket>> clients;
    std::mutex clients_mutex;
    bool running;
    std::vector<ChronoLink> links;

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
        std::getline(file, int_str, '|');

        try {
            return int_str.empty() ? default_value : std::stoi(int_str);
        } catch (...) {
            return default_value;
        }
    }

public:
    Master() : acceptor(io_context, tcp::endpoint(tcp::v4(), PORT)), running(true) {}

    void socket_setup() {
        std::cout << "Server listening on port " << PORT << std::endl;
        start_listening_for_input();
        accept_connections();
        io_context.run();
    }

    void accept_connections() {
        auto socket = std::make_shared<tcp::socket>(io_context);
        acceptor.async_accept(*socket, [this, socket](boost::system::error_code ec) {
            if (!ec) {
                std::cout << "New client connected\n";
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.push_back(socket);
                start_reading(socket);
            }
            accept_connections();
        });
    }

    void start_reading(std::shared_ptr<tcp::socket> socket) {
        auto buffer = std::make_shared<std::vector<char>>(1024);
        socket->async_read_some(boost::asio::buffer(*buffer),
            [this, socket, buffer](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string message(buffer->begin(), buffer->begin() + length);
                    std::cout << "Received from client: " << message << std::endl;
                    start_reading(socket);
                } else {
                    remove_client(socket);
                }
            });
    }

    void broadcast(const std::string &message) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (auto it = clients.begin(); it != clients.end();) {
            if ((*it)->is_open()) {
                boost::asio::async_write(**it, boost::asio::buffer(message),
                    [](boost::system::error_code ec, std::size_t) {});
                ++it;
            } else {
                it = clients.erase(it);
            }
        }
    }

    void remove_client(std::shared_ptr<tcp::socket> socket) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), socket), clients.end());
        std::cout << "Client disconnected.\n";
    }

    void start_listening_for_input() {
        std::thread([this]() {
            show_navigation_menu();
            std::string command;

            while (true) {
                std::cout << "Enter the command: " << std::endl;
                
                if (!std::getline(std::cin, command) || command.empty()) 
                    break;
                
                switch (std::toupper(command[0])) {
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
                        break;
                    case 'C':
                        handle_list_command();
                        break;
                    default:
                        send_invalid_command_response();
                        break;
                }
            }

        }).detach();
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

    void handle_add_command() {
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

        broadcast(COMMAND);
    }

    void handle_exec_command() {
        std::string msg = "EXEC|_|_|_|_";
        broadcast(msg);
    }

    void handle_shutdown_command() {
        std::cout << "Server shutting down." << std::endl;
        std::string msg = "SHUTDOWN|_|_|_|_";
        broadcast(msg);
        running = false;
        io_context.stop();
    }

    void handle_terminate_command() {
        std::string msg = "TERMINATE|_|_|_|_";
        broadcast(msg);
    
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (auto& client : clients) {
            if (client->is_open()) {
                client->close();
            }
        }
        clients.clear(); // Remove all disconnected clients from the list
        std::cout << "All clients disconnected.\n";
    }
    
    void handle_list_command() {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string msg = "Connected clients: " + std::to_string(clients.size());
        std::cout << msg << std::endl;
        broadcast(msg);
    }

    void send_invalid_command_response() {
        std::string msg = "Invalid command.";
        std::cout << msg << std::endl;
        broadcast(msg);
    }

    ~Master() {
        for (auto& client : clients) {
            if (client->is_open()) {
                client->close();
            }
        }
    }
};

int main() {
    Master server;
    server.socket_setup();

    return 0;
}