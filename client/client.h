#ifndef CLIENT_H
#define CLIENT_H

#include "client_config.h"
#include <netinet/in.h>

constexpr size_t BUFFER_SIZE = 2048;

class Client
{
public:
    explicit Client(const ClientConfig &config);
    ~Client();

    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;

    void run();

private:
    ClientConfig cfg;
    int socket_fd;
    sockaddr_in server_addr;

    void setup_socket();
    void send_message();
    void receive_response();
    void print_response(const buffer_t &buf);
};

#endif /* CLIENT_H */
