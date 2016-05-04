#pragma once

#include <string>
#include <vector>
#include <iostream>

#include <stdint.h>

#include "TCPSocket.hpp"
#include "Logger.hpp"

const uint32_t START_HEADER = 0x64616568;       /* "head" */
const uint32_t MESSAGE_HEADER = 0x31313131;     /* "1111" */
const uint32_t KEEP_ALIVE_SECONDS = 5;

#define MSG_TYPE_PACK           0x4B434150      /* "PACK" */
#define MSG_TYPE_SYS_INFO       0x00010001
#define MSG_TYPE_CH1_STATUS     0x09000001
#define MSG_TYPE_CH2_STATUS     0x09000002
#define MSG_TYPE_CH3_STATUS     0x09000003
#define MSG_TYPE_CH4_STATUS     0x09000004
#define MSG_TYPE_CH_SUMMARY     0x09000008
#define MSG_TYPE_VIDEO_FORMAT   0x0A000001
#define MSG_TYPE_UNKNOWN_A      0x04000001
#define MSG_TYPE_UNKNOWN_B      0x05000001

class QT428
{
public:
    QT428(const Logger& logger, TCPSocket& tcpSocket,
          const std::string& username,
          const std::string& password,
          uint8_t channel,
          std::ostream& outputStream)
        : _logger(logger), _tcpSocket(tcpSocket),
          _username(username), _password(password),
          _channel(channel),
          _outputStream(outputStream) {
        _lastPing = time(NULL);
    }

    bool process(void);

    void onMessage(const std::vector<uint8_t>& data);
    void onSysInfoMessage(const std::vector<uint8_t>& data);
    void onChannelStatusMessage(uint8_t ch, const std::vector<uint8_t>& data);
    void onChannelSummaryMessage(const std::vector<uint8_t>& data);
    void onVideoMessage(const std::vector<uint8_t>& data);
    void onPackMessage(const std::vector<uint8_t>& data);
    void onUnknownMessageA(const std::vector<uint8_t>& data);
    void onUnknownMessageB(const std::vector<uint8_t>& data);

    void sendEnableLiveVideo(uint32_t channel);
private:
    void onConnect(void);
    bool recvMessage(void);
    bool sendMessage(const std::vector<uint8_t>& buffer);

    void sendLogin(void);
    void sendPing(void);
    void saveVideo(const std::vector<uint8_t>& data, size_t offset);

    const Logger& _logger;
    TCPSocket& _tcpSocket;
    std::string _username;
    std::string _password;
    uint8_t _channel;
    std::ostream& _outputStream;
    time_t _lastPing;
    std::vector<uint8_t> _packBuffer;
    std::vector<uint8_t> _rxBuffer;
    size_t _packOffset;
};
