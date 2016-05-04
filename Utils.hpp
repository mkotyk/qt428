#pragma once

#include <string>

std::string format(const std::string& fmt, ...);
void hexdump(const unsigned char *data, size_t length);
