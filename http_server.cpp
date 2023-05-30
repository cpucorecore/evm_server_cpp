//
// Created by sky on 2023/5/30.
//

#include "http_server.h"
#include "evm_tx_executor.h"
#include <nlohmann/json.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <iostream>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace evmone;
using json = nlohmann::json;

namespace beast = boost::beast;

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

    evm_tx_executor executor;
}

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
    void
    process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch(request_.method())
        {
            case http::verb::get:
                response_.result(http::status::ok);
                response_.set(http::field::server, "Beast");
                create_response();
                break;

            case http::verb::post:
                response_.result(http::status::ok);
                response_.set(http::field::server, "Beast");
                response_.set(boost::beast::http::field::content_type, "application/json");
                try
                {
                    nlohmann::json payload = nlohmann::json::parse(request_.body());
                    std::cout << "Received JSON payload: " << payload.dump() << std::endl;

                    std::cout << "uri:" << request_.target() << std::endl;
                    if(request_.target() == "/contract/create") {
                        auto from = payload["from"].get<std::string>();
                        auto code = payload["code"].get<std::string_view>();
                        auto addr = operator""_address(from.c_str());
                        auto acc = my_program_state::executor.create_account(addr);
                        auto code_bytes = evmc::from_hex(code);
                        evmone::state::Transaction tx{
                                .kind = evmone::state::Transaction::Kind::legacy,
                                .data = code_bytes.value(),
                                .gas_limit = 2000000,
                                .max_gas_price = 20,
                                .max_priority_gas_price = 10,
                                .sender = addr,
                                .to = std::nullopt,
                                .value = 0,
                                .chain_id = 0,
                                .nonce = acc.nonce,
                        };
                        my_program_state::executor.execute_tx(tx);
                    } else if(request_.target() == "/contract/call") {
                        auto from = payload["from"].get<std::string>();
                        auto to = payload["to"].get<std::string>();
                        auto input = payload["input"].get<std::string_view>();
                        auto from_addr = operator""_address(from.c_str());
                        auto to_addr = operator""_address(from.c_str());
                        auto acc = my_program_state::executor.create_account(from_addr);
                        auto input_bytes = evmc::from_hex(input);
                        evmone::state::Transaction tx{
                                .kind = evmone::state::Transaction::Kind::legacy,
                                .data = input_bytes.value(),
                                .gas_limit = 2000000,
                                .max_gas_price = 20,
                                .max_priority_gas_price = 10,
                                .sender = from_addr,
                                .to = to_addr,
                                .value = 0,
                                .chain_id = 0,
                                .nonce = acc.nonce,
                        };
                        my_program_state::executor.execute_tx(tx);
                    } else {
                        // TODO
                    }
                    json response_payload;
                    response_payload["status"] = "success";
                    response_payload["message"] = "Received JSON payload successfully";

                    beast::ostream(response_.body()) << response_payload;
                }
                catch (json::parse_error& e)
                {
                    response_.result(http::status::bad_request);
                    beast::ostream(response_.body()) << "Invalid JSON payload";
                }
                break;

            default:
                response_.result(http::status::bad_request);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body())
                        << "Invalid request-method '"
                        << std::string(request_.method_string())
                        << "'";
                break;
        }

        write_response();
    }

    // Construct a response message based on the program state.
    void
    create_response()
    {
        if(request_.target() == "/count")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                    << "<html>\n"
                    <<  "<head><title>Request count</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Request count</h1>\n"
                    <<  "<p>There have been "
                    <<  my_program_state::request_count()
                    <<  " requests so far.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        }
        else if(request_.target() == "/time")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                    <<  "<html>\n"
                    <<  "<head><title>Current time</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Current time</h1>\n"
                    <<  "<p>The current time is "
                    <<  my_program_state::now()
                    <<  " seconds since the epoch.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
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
                          [&](beast::error_code ec)
                          {
                              if(!ec)
                                  std::make_shared<http_connection>(std::move(socket))->start();
                              http_server(acceptor, socket);
                          });
}