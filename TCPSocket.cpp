#include "TCPSocket.hpp"

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>


bool TCPSocket::connect(const std::string& hostname, int port)
{
    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    struct addrinfo hints, *server;
    struct sockaddr_in server_addr;
    struct linger lngr = { .l_onoff = 0, .l_linger = 0 };
    int flag = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(hostname.c_str(), NULL, &hints, &server) != 0) {
        _logger.error("Hostname lookup failed.");
        return false;
    }
    server_addr.sin_family = AF_INET,
    server_addr.sin_addr = ((struct sockaddr_in*)server->ai_addr)->sin_addr;
    server_addr.sin_port = htons(port);

    if ((_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        _logger.error("Unable to create socket.");
        return false;
    }

    if (setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv))) {
        _logger.error("Failed to set socket timeout");
        return false;
    }

    if (setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag))) {
        _logger.error("Failed to set TCP_NODELAY");
        return false;
    }

    if (setsockopt(_fd, SOL_SOCKET, SO_LINGER, (char*)&lngr, sizeof(lngr))) {
        _logger.error("Failed to set SO_LINGER");
        return false;
    }

    if (::connect(_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        _logger.error("Unable to connect to remote.");
        return false;
    }

    return true;
}

size_t TCPSocket::recv(void *data, size_t size)
{
    int result;
    size_t bytesProcessed = 0;
    while(bytesProcessed != size) {
        result = ::recv(_fd, (char *)data + bytesProcessed, size - bytesProcessed, 0);
        if(result < 0) {
            if(errno == EAGAIN) {
                continue;
            }
            return result;
        }
        bytesProcessed += result;
    }
    return bytesProcessed;
}

size_t TCPSocket::send(const void* data, size_t size)
{
    int result;
    size_t bytesProcessed = 0;
    while(bytesProcessed != size) {
        result = ::send(_fd, (char *)data + bytesProcessed, size - bytesProcessed, 0);
        if(result < 0) {
            if(errno == EAGAIN) {
                continue;
            }
            return result;
        }
        bytesProcessed += size;
    }
    return bytesProcessed;
}

void TCPSocket::toss(size_t size)
{
    char c;
    for(;size && ::recv(_fd, &c, sizeof(c), 0) == sizeof(c);size--);
}
