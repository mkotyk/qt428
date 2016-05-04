#include "QT428.hpp"
#include "Utils.hpp"
#include "TCPSocket.hpp"
#include "Logger.hpp"

#include <unistd.h>
#include <stdlib.h>

static void usage()
{
    std::cerr << "Usage:" << std::endl;
    std::cerr << "    qt428 -v -u <username> -p <password> -c <channel> host[:port]" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    -v            Verbose output." << std::endl;
    std::cerr << "    -u <username> User name for logging into DVB server.  Default admin." << std::endl;
    std::cerr << "    -p <password> Password for logging into DVB server.  Default 123456." << std::endl;
    std::cerr << "    -c <channel>  Which channel to stream live video." << std::endl;
    std::cerr << "    host          Host to connect to." << std::endl;
    std::cerr << "    port          Port to connect to.  Default 6036." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Example:" << std::endl;
    std::cerr << " qt428 -u admin -p 123456 -c 1 192.168.155.6 | mplayer -" << std::endl;
    std::cerr << std::endl;
}

int main(int argc, char *argv[])
{
    std::string user = "admin";
    std::string password = "123456";
    std::string hostname;
    unsigned int port = 6036;
    unsigned int channel = 1;
    int logLevel = 0;
    int opt;

    while((opt = getopt(argc, argv, "u:p:c:")) != -1) {
        switch(opt) {
            case 'u':
                user.assign(optarg);
                break;
            case 'p':
                password.assign(optarg);
                break;
            case 'c':
                channel = atoi(optarg);
                break;
            case 'v':
                logLevel++;
                break;
            case 'h':
            case '?':
            default:
                usage();
                return -1;
        }
    }

    if(optind >= argc) {
        usage();
        return -1;
    }

    hostname.assign(argv[optind]);
    int sepIndex = hostname.find(':');
    if(sepIndex != -1) {
        port = atoi(hostname.substr(sepIndex + 1).c_str());
        hostname.resize(sepIndex);
    }

    Logger logger;
    logger.setLevel(logLevel);
    TCPSocket tcpSocket(logger);
    QT428 qt428(logger, tcpSocket, user, password, channel, std::cout);

    if(!tcpSocket.connect(hostname, port)) {
        logger.error(format("Unable to connect to [%s:%d]\n", hostname, port));
        return -1;
    }

    while(qt428.process());

    return 0;
}
