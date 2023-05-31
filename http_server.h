//
// Created by sky on 2023/5/30.
//

#ifndef EVM_SERVER_CPP_HTTP_SERVER_H
#define EVM_SERVER_CPP_HTTP_SERVER_H


#include <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;

void http_server(tcp::acceptor& acceptor, tcp::socket& socket);

enum REQUEST_RETURN_CODE : int {
    HTTP_SUCCESS = 0,
    UNSUPPORTED_HTTP_METHOD,
    EVM_ERROR_CODE_BASE = 10000,
};

#endif //EVM_SERVER_CPP_HTTP_SERVER_H
