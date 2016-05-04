#include "QT428.hpp"

#include <string.h>
#include <stdint.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <fstream>
#include <algorithm>

#include "Utils.hpp"

bool QT428::process(void)
{
    struct timeval timeout;
    fd_set readFDS;
    FD_ZERO(&readFDS);
    FD_SET(_tcpSocket.getFd(), &readFDS);
    int maxFd = _tcpSocket.getFd();
    bool rv = true;

    timeout.tv_sec = KEEP_ALIVE_SECONDS;
    timeout.tv_usec = 0;

    int rc = select(maxFd + 1, &readFDS, NULL, NULL, &timeout);
    if (rc == -1) {
        return false;
    } else if (rc == 0) {
        sendPing();
    }
    if(FD_ISSET(_tcpSocket.getFd(), &readFDS)) {
        rv = recvMessage();
    }
    time_t now = time(NULL);
    if((now - _lastPing) > KEEP_ALIVE_SECONDS) {
        sendPing();
    }
    return rv;
}

void QT428::sendPing(void)
{
    std::vector<uint8_t> empty;
    _lastPing = time(NULL);
    _logger.info("* Sending ping");
    sendMessage(empty);
}

bool QT428::recvMessage(void)
{
    uint32_t id;
    uint32_t length;
    if (_tcpSocket.recv(&id, sizeof(id)) == sizeof(id)) {
        if (id == START_HEADER) {
            /* Ignore the next 0x3B bytes */
            _tcpSocket.toss(0x40 - sizeof(id));
            onConnect();
        } else if (id == MESSAGE_HEADER) {
            if (_tcpSocket.recv(&length, sizeof(length)) == sizeof(length)) {
                if (length == 0) {
                    _logger.info("Incoming ping.");
                    return true;
                }
                _rxBuffer.reserve(length);
                _rxBuffer.resize(length);
                _tcpSocket.recv(_rxBuffer.data(), _rxBuffer.size());
                onMessage(_rxBuffer);
            } else {
                _logger.error("Unable to read length of message.");
                return false;
            }
        } else {
            _logger.error(format("Unknown message ID 0x%04X\n", id));
            return false;
        }
    } else {
        _logger.error("Unable to read message id.");
        return false;
    }
    return true;
}

bool QT428::sendMessage(const std::vector<uint8_t>& buffer)
{
    uint32_t msgLength = buffer.size();
    if (_tcpSocket.send(&MESSAGE_HEADER, sizeof(MESSAGE_HEADER)) != sizeof(MESSAGE_HEADER))
        return false;
    if (_tcpSocket.send(&msgLength, sizeof(msgLength)) != sizeof(msgLength))
        return false;
    if (msgLength == 0) {
        return true;
    }

    hexdump(buffer.data(), buffer.size());
    return (_tcpSocket.send(buffer.data(), msgLength) == msgLength);
}

void QT428::sendLogin(void)
{
//00000000  31 31 31 31 88 00 00 00  01 01 00 00 02 01 00 00   1111.... ........
//00000010  01 00 00 00 78 00 00 00  05 00 00 00 00 00 00 00   ....x... ........
//00000020  61 64 6d 69 6e 00 00 00  00 00 00 00 00 00 00 00   admin... ........
//00000030  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000040  00 00 00 00 31 32 33 34  35 36 00 00 00 00 00 00   ....1234 56......
//00000050  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000060  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000070  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000080  00 00 00 00 00 00 00 00  00 00 00 00 04 00 00 00   ........ ........
    typedef struct {
        uint8_t channel;    // Always 1
        uint8_t command;    // Always 1
        uint16_t unknown1;  // Always 0
        uint32_t unknown2;  // Alwaus 0x102
        uint32_t unknown3;  // Always 1
        uint32_t unknown4;  // Always 0x78
        uint32_t unknown5;  // 0x5
        uint32_t unknown6;
        uint8_t username[8];
        uint8_t unknown7[28];
        uint8_t password[6];
        uint8_t unknown8[64];
        uint32_t unknown9;  // Always 0x4
    } LoginMessage;

    LoginMessage loginMessage;

    memset(&loginMessage, 0, sizeof(loginMessage));
    loginMessage.channel = 0x1;
    loginMessage.command = 0x1;
    loginMessage.unknown2 = 0x102;
    loginMessage.unknown3 = 0x1;
    loginMessage.unknown4 = 0x78;
    loginMessage.unknown5 = 0x5;
    strncpy((char *) loginMessage.username,
            _username.c_str(),
            sizeof(loginMessage.username));
    strncpy((char *) loginMessage.password,
            _password.c_str(),
            sizeof(loginMessage.password));
    loginMessage.unknown9 = 0x4;

    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(LoginMessage));
    std::copy((uint8_t*) &loginMessage, (uint8_t*)  &loginMessage + sizeof(loginMessage), std::back_inserter(buffer));
    sendMessage(buffer);
}

void QT428::sendEnableLiveVideo(uint32_t channel)
{
//000000D0  31 31 31 31 34 00 00 00  01 02 00 00 00 00 00 00   11114... ........
//000000E0  01 00 00 00 24 00 00 00  00 00 00 00 01 00 00 00   ....$... ........
//000000F0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000100  00 00 00 00 00 00 00 00  00 00 00 00               ........ ....

    typedef struct {
        uint32_t msgType;   // 0x201
        uint32_t unknown1;  // 0x00
        uint32_t unknown2;  // 0x01
        uint32_t unknown3;  // 0x24
        uint32_t unknown4;  // 0x0
        uint32_t channel_mask;
        uint32_t unknown5;  // 0x0
        uint32_t unknown6;  // 0x0
        uint32_t unknown7;  // 0x0
        uint32_t unknown8;  // 0x0
        uint32_t unknown9;  // 0x0
        uint32_t unknown10;  // 0x0
        uint32_t unknown11;  // 0x0
    } LiveVideo;

    LiveVideo liveVideo;
    _channel = channel;

    memset(&liveVideo, 0, sizeof(liveVideo));
    liveVideo.msgType = (channel > 0)?0x201:0x203;
    liveVideo.unknown2 = 0x01;
    liveVideo.unknown3 = 0x24;
    if(channel > 0) {
        liveVideo.channel_mask = 1<<(channel - 1);
    }

    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(LiveVideo));
    std::copy((uint8_t*) &liveVideo, (uint8_t*) &liveVideo + sizeof(liveVideo), std::back_inserter(buffer));
    sendMessage(buffer);

    if(channel == 0) {
        return;
    }
    return;

// There seems to always be a second command that is constant
//0000010C  31 31 31 31 10 00 00 00  0c 05 00 00 d2 04 00 00   1111.... ........
//0000011C  01 00 00 00 00 00 00 00                            ........ 
    uint8_t unknownCmd[16] = {
        0x0C, 0x05, 0x00, 0x00 ,0xD2, 0x04, 0x00, 0x00,
        0x01, 0x00, 0x00 ,0x00, 0x00, 0x00, 0x00, 0x00
    };
    buffer.resize(sizeof(unknownCmd));
    buffer.assign(unknownCmd, unknownCmd + sizeof(unknownCmd));
    sendMessage(buffer);
}

void QT428::onConnect(void)
{
    sendLogin();
}

void QT428::onMessage(const std::vector<uint8_t>& data)
{
    // Cast the first uint32_t to identify the message type
    uint32_t msgType = *((uint32_t *)data.data());
    switch(msgType) {
        case MSG_TYPE_PACK:
            onPackMessage(data);
            break;
        case MSG_TYPE_SYS_INFO:
            onSysInfoMessage(data);
            break;
        case MSG_TYPE_CH1_STATUS:
        case MSG_TYPE_CH2_STATUS:
        case MSG_TYPE_CH3_STATUS:
        case MSG_TYPE_CH4_STATUS:
            onChannelStatusMessage(msgType & 0xFF, data);
            break;
        case MSG_TYPE_CH_SUMMARY:
            onChannelSummaryMessage(data);
            break;
        case MSG_TYPE_VIDEO_FORMAT:
            onVideoMessage(data);
            break;
        case MSG_TYPE_UNKNOWN_A:
            onUnknownMessageA(data);
            break;
        case MSG_TYPE_UNKNOWN_B:
            onUnknownMessageB(data);
            break;
        default:
            _logger.info(format("Unknown message type [%08X]", msgType));
            hexdump(data.data(), data.size());
            break;
    }
}

void QT428::onSysInfoMessage(const std::vector<uint8_t>& data)
{
//00000040  31 31 31 31 6c 01 00 00  01 00 01 00 00 2c 39 40   1111l... .....,9@
//00000050  04 00 00 00 5c 01 00 00  ff ff ff ff ff 00 00 00   ....\... ........
//00000060  00 00 00 00 ff 00 00 00  00 00 00 00 ff 00 00 00   ........ ........
//00000070  00 00 00 00 ff 00 00 00  00 00 00 00 ff 00 00 00   ........ ........
//00000080  00 00 00 00 ff 00 00 00  00 00 00 00 08 04 08 01   ........ ........
//00000090  59 00 00 00 08 04 00 00  80 08 10 04 40 0a 01 c9   Y....... ....@...
//000000A0  01 01 01 01 66 82 84 80  08 00 00 00 00 00 00 00   ....f... ........
//000000B0  fc ff c9 03 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//000000C0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//000000D0  00 00 00 00 00 18 ae 2b  65 b6 00 00 0b 0c dc 07   .......+ e.......
//000000E0  2d 3b 0c 00 45 44 56 52  00 00 00 00 00 00 00 00   -;..EDVR ........
//000000F0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000100  00 00 00 00 00 00 00 00  33 2e 32 2e 30 2e 50 2d   ........ 3.2.0.P-
//00000110  31 2e 30 2e 32 2e 31 2d  30 33 00 00 00 00 00 00   1.0.2.1- 03......
//00000120  00 00 00 00 00 00 00 00  00 00 00 00 31 31 31 31   ........ ....1111
//00000130  32 32 30 33 35 35 2d 31  30 31 30 32 33 31 34 33   220355-1 01023143
//00000140  35 2d 00 00 00 00 00 00  00 00 00 00 00 00 00 00   5-...... ........
//00000150  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000160  00 00 00 00 00 00 00 00  00 00 00 00 32 30 31 2e   ........ ....201.
//00000170  33 2e 34 2e 51 39 2d 31  2e 30 33 00 00 00 00 00   3.4.Q9-1 .03.....
//00000180  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000190  2d 2d 2d 00 00 00 00 00  00 00 00 00 00 00 00 00   ---..... ........
//000001A0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//000001B0  00 00 00 00                                        ....

    sendEnableLiveVideo(_channel);
}

void QT428::onChannelStatusMessage(uint8_t ch, const std::vector<uint8_t>& data)
{
//000001B4  31 31 31 31 34 00 00 00  04 00 00 09 ff ff ff ff   11114... ........
//000001C4  14 53 4c 00 24 00 00 00  08 00 00 00 00 00 00 00   .SL.$... ........
//000001D4  00 00 00 00 00 00 00 00  00 00 00 00 01 00 00 00   ........ ........
//000001E4  00 00 00 00 00 00 00 00  00 00 00 00

//00000214  31 31 31 31 1c 00 00 00  03 00 00 09 ff ff ff ff   1111.... ........
//00000224  73 6b 20 69 0c 00 00 00  08 00 00 00 00 00 00 00   sk i.... ........
//00000234  00 00 00 00                                        ....
}

void QT428::onChannelSummaryMessage(const std::vector<uint8_t>& data)
{
//00000238  31 31 31 31 1c 00 00 00  01 00 00 09 ff ff ff ff   1111.... ........
//00000268  ff ff ff ff 00 00 00 00  40 01 00 00 00 00 24 00   ........ @.....$.
//00000278  44 45 43 4b 00 00 00 00  00 00 00 00 00 00 00 00   DECK.... ........
//00000288  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000298  00 00 00 00 01 00 24 00  46 52 4f 4e 54 00 00 00   ......$. FRONT...
//000002A8  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//000002B8  00 00 00 00 00 00 00 00  00 00 00 00 02 00 24 00   ........ ......$.
//000002C8  47 41 52 41 47 45 00 00  00 00 00 00 00 00 00 00   GARAGE.. ........
//000002D8  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//000002E8  00 00 00 00 03 00 24 00  43 41 4d 45 52 41 30 34   ......$. CAMERA04
//000002F8  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000308  00 00 00 00 00 00 00 00  00 00 00 00 04 00 24 00   ........ ......$.
//00000318  43 41 4d 45 52 41 30 35  00 00 00 00 00 00 00 00   CAMERA05 ........
//00000328  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000338  00 00 00 00 05 00 24 00  43 41 4d 45 52 41 30 36   ......$. CAMERA06
//00000348  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000358  00 00 00 00 00 00 00 00  00 00 00 00 06 00 24 00   ........ ......$.
//00000368  43 41 4d 45 52 41 30 37  00 00 00 00 00 00 00 00   CAMERA07 ........
//00000378  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//00000388  00 00 00 00 07 00 24 00  43 41 4d 45 52 41 30 38   ......$. CAMERA08
//00000398  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
//000003A8  00 00 00 00 00 00 00 00  00 00 00 00               ........ ....
}

void QT428::onVideoMessage(const std::vector<uint8_t>& data)
{
//000004B8  31 31 31 31 74 00 00 00  01 00 00 0a ff ff ff ff   1111t... ........
//000004C8  04 00 00 00 64 00 00 00  00 00 00 00 03 00 00 00   ....d... ........
//000004D8  28 00 00 00 60 01 00 00  f0 00 00 00 e0 a4 e4 43   (...`... .......C
// 10
//000004E8  00 00 00 00 00 01 f0 3a  00 00 00 00 20 00 00 00   .......: .... ...
//000004F8  00 00 00 00 a8 42 d6 27  eb 30 05 00 6e 25 82 e4   .....B.' .0..n%..
//00000508  a6 01 00 00 28 00 00 00  60 01 00 00 f0 00 00 00   ....(... `.......
//00000518  01 00 10 00 48 32 36 34  00 00 00 00 00 00 00 00   ....H264 ........
//00000528  00 00 00 00 00 00 00 00  00 00 00 00               ........ ....
//// 29

    typedef struct {
        uint32_t command;  // always 0x0A000001
        uint32_t unknown1; // always 0xFFFFFFFF
        uint32_t unknown2; // always 0x4
        uint32_t payloadLength; // Always seems to be 16 bytes less than packet
        uint32_t unknown3; // always 0
        uint32_t unknown4; // always 1
        uint32_t unknown5;
        uint32_t xSize;     // always 0x160 = 352 = x res
        uint32_t ySize;     // always 0xf0 = 240 = y res
        uint32_t unknown6;
        uint32_t unknown7;
    } VideoMessage;

    VideoMessage *videoMessage = (VideoMessage*) data.data();
    size_t offset = (data.size() - videoMessage->payloadLength) + sizeof(VideoMessage);
    saveVideo(data, offset);
}

void QT428::onUnknownMessageA(const std::vector<uint8_t>& data)
{
    // Contains 16 repeating fields.. one per channel?  Motion trigger?
//000003A8                                       31 31 31 31                1111
//000003B8  ec 00 00 00 02 00 04 00  00 2c 39 40 04 00 00 00   ........ .,9@....
//000003C8  dc 00 00 00 0c 00 00 00  02 00 00 00 03 00 00 00   ........ ........
//000003D8  01 04 08 00 60 00 0c 00  02 04 08 00 60 00 0c 00   ....`... ....`...
//000003E8  02 1e 02 05 40 00 00 00  00 03 00 00 02 1e 02 05   ....@... ........
//000003F8  40 00 00 00 00 03 00 00  02 1e 02 05 40 00 00 00   @....... ....@...
//00000408  00 03 00 00 02 1e 02 05  40 00 00 00 00 03 00 00   ........ @.......
//00000418  02 1e 02 05 40 00 00 00  00 03 00 00 02 1e 02 05   ....@... ........
//00000428  40 00 00 00 00 03 00 00  02 1e 02 05 40 00 00 00   @....... ....@...
//00000438  00 03 00 00 02 1e 02 05  40 00 00 00 00 03 00 00   ........ @.......
//00000448  02 03 02 05 40 00 00 00  00 02 00 00 02 03 02 05   ....@... ........
//00000458  40 00 00 00 00 02 00 00  02 03 02 05 40 00 00 00   @....... ....@...
//00000468  00 02 00 00 02 03 02 05  40 00 00 00 00 02 00 00   ........ @.......
//00000478  02 03 02 05 40 00 00 00  00 02 00 00 02 03 02 05   ....@... ........
//00000488  40 00 00 00 00 02 00 00  02 03 02 05 40 00 00 00   @....... ....@...
//00000498  00 02 00 00 02 03 02 05  40 00 00 00 00 02 00 00   ........ @.......
}

void QT428::onUnknownMessageB(const std::vector<uint8_t>& data)
{
//00000534  31 31 31 31 14 00 00 00  03 00 05 00 00 2c 39 40   1111.... .....,9@
//00000544  04 00 00 00 04 00 00 00  e8 03 00 00               ........ ....
}

void QT428::onPackMessage(const std::vector<uint8_t>& data)
{
//00000544                                       31 31 31 31                1111
//00000554  1c 28 00 00 50 41 43 4b  01 00 00 00 05 00 00 00   .(..PACK ........
//00000564  dc a6 00 00 01 00 00 00  00 28 00 00 00 00 00 00   ........ .(......
//00000574  01 00 00 0a ff ff ff ff  04 00 00 00 cc a6 00 00   ........ ........
//00000584  01 00 00 00 01 00 00 00  8e a6 00 00 60 01 00 00   ........ ....`...
//00000594  f0 00 00 00 48 a0 bd 40  00 00 00 00 00 0b d0 3b   ....H..@ .......;
//000005A4  00 00 00 00 20 00 00 00  00 00 00 00 a9 c9 d7 27   .... ... .......'
//000005B4  eb 30 05 00 6f ac 83 e4  a6 01 00 00 00 00 00 01   .0..o... ........
//000005C4  67 42 e0 14 db 05 87 c4  00 00 00 01 68 ce 30 a4   gB...... ....h.0.
//000005D4  80 00 00 00 01 06 e5 01  1f 80 00 00 00 01 65 b8   ........ ......e.
//000005E4  00 00 c3 e4 02 b1 42 82  98 00 30 32 e1 89 e2 86   ......B. ..02....
// no repeating 0 octets beyond this line indicates compression

    // PACK represents a large packet broken up into parts
    //  - it is unknown if PACK messages can be interleaved
    //  - the actual content of a PACK message needs to be re-assembled
    //    and run through the onMessage method
    // sequence = sequential identifier for each PACK transaction
    // totalParts = total number of parts to expect
    // totalSize = total size of assembled pack
    // partNum = the number of this part
    // payloadSize = the number of bytes in this part
    typedef struct {
        uint32_t msgType; // Always "PACK"
        uint32_t sequence;
        uint32_t totalParts;
        uint32_t totalSize;
        uint32_t partNum;
        uint32_t payloadSize;
        uint32_t unknown6; // 0
    } PackMessage;

    PackMessage *packMessage = (PackMessage *) data.data();
    if (packMessage->partNum == 1) {
        _packBuffer.reserve(packMessage->totalSize);
        _packBuffer.resize(packMessage->totalSize);
        _packOffset = 0;
    }
    _logger.info(format("+ Pack %d/%d %d", packMessage->partNum,
                packMessage->totalParts,
                _packOffset));

    std::copy(data.begin() + sizeof(PackMessage),
              data.end(),
              _packBuffer.begin() + _packOffset);
    _packOffset += packMessage->payloadSize;

    if(packMessage->partNum == packMessage->totalParts) {
        onMessage(_packBuffer);
    }
}

void QT428::saveVideo(const std::vector<uint8_t>& data, size_t offset)
{
    _outputStream.write(((const char *)data.data()) + offset, data.size() - offset);
}
