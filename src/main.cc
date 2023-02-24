#include "web_server.h"

int main() {
    socket_options options(9000, true, true);
    web_server http_server(web_server::EPOLL_MODE::LISTEN_CONNECTION_LT, 30000, options);
    http_server.start();
}