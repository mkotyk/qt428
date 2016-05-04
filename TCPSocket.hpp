#pragma once

#include <string>
#include "Logger.hpp"

class TCPSocket
{
public:
    TCPSocket(const Logger& logger) : _logger(logger) {}

    bool connect(const std::string& hostname, int port);
    size_t recv(void* data, size_t size);
    size_t send(const void* data, size_t size);
    void toss(size_t length);

    int getFd() const { return _fd; }
private:
    int _fd;
    const Logger& _logger;
};

