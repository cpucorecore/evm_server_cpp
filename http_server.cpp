//
// Created by sky on 2023/5/30.
//

#include "http_server.h"
#include "evm_request_processor.h"
#include <nlohmann/json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <iostream>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using json = nlohmann::json;

namespace my_program_state
{
    std::size_t
    request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }

    std::time_t
    now()
    {
        return std::time(0);
    }
}

evm_request_processor processor;

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket)
            : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void
    start()
    {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
//    http::request<http::dynamic_body> request_;
    http::request<http::string_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
            socket_.get_executor(), std::chrono::seconds(60)};

    // Asynchronously receive a complete request message.
    void
    read_request()
    {
        auto self = shared_from_this();

        http::async_read(
                socket_,
                buffer_,
                request_,
                [self](beast::error_code ec,
                       std::size_t bytes_transferred)
                {
                    boost::ignore_unused(bytes_transferred);
                    if(!ec)
                        self->process_request();
                });
    }

    // Determine what needs to be done with the request message.
    void process_request() {
        std::cout << "uri:" << request_.target() << std::endl;

        response_.keep_alive(false);
        response_.result(http::status::ok);
        response_.set(http::field::server, "evm");
        response_.set(boost::beast::http::field::content_type, "application/json");

        json resp_json;
        resp_json["rc"] = HTTP_SUCCESS;

        switch(request_.method()) {
            case http::verb::post: {
                auto ret = processor.do_request(request_.target().to_string(), request_.body().c_str());
                if (holds_alternative<json>(ret)) {
                    auto ret_json = std::get<json>(ret);
                    resp_json["err_message"] = "";
                    resp_json["result"] = ret_json;
                } else {
                    response_.result(http::status::bad_request);
                    auto err = std::get<std::error_code>(ret);
                    resp_json["rc"] = EVM_ERROR_CODE_BASE + err.value();
                    resp_json["err_message"] = err.message();
                    beast::ostream(response_.body()) << err;
                }

                break;
            }

            default:
                response_.result(http::status::bad_request);
                resp_json["rc"] = UNSUPPORTED_HTTP_METHOD;
                resp_json["err_message"] = std::string("unsupported http request, must post request");
                break;
        }

        beast::ostream(response_.body()) << resp_json;
        write_response();
    }

    void
    write_response()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(
                socket_,
                response_,
                [self](beast::error_code ec, std::size_t)
                {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                    self->deadline_.cancel();
                });
    }

    // Check whether we have spent enough time on this connection.
    void
    check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
                [self](beast::error_code ec)
                {
                    if(!ec)
                    {
                        // Close socket to cancel any outstanding operation.
                        self->socket_.close(ec);
                    }
                });
    }
};

void http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
    acceptor.async_accept(socket,
                          [&](beast::error_code ec) {
                              if(!ec)
                                  std::make_shared<http_connection>(std::move(socket))->start();
                              http_server(acceptor, socket);
                          });
}