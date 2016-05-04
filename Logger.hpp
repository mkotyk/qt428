#pragma once
#include <stdio.h>

class Logger
{
public:
    const static int LEVEL_INFO = 1;

    void setLevel(int logLevel) {
        _logLevel = logLevel;
    }

    void error(const std::string msg) const {
        perror(msg.c_str());
    }

    void info(const std::string msg) const {
        if(_logLevel >= LEVEL_INFO) {
            fprintf(stderr, "INFO: %s\n", msg.c_str());
        }
    }
private:
    int _logLevel;
};
