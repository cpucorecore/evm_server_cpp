#include "state/state.hpp"
#include "state/rlp.hpp"
#include "evmone/evmone.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace evmone;

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
    http::request<http::dynamic_body> request_;

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

            default:
                // We return responses indicating an error if
                // we do not recognize the request method.
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

void
http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
    acceptor.async_accept(socket,
                          [&](beast::error_code ec)
                          {
                              if(!ec)
                                  std::make_shared<http_connection>(std::move(socket))->start();
                              http_server(acceptor, socket);
                          });
}

int main(int argc, char* argv[]) {
    try
    {
        if(argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <address> <port>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    receiver 0.0.0.0 80\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    receiver 0::0 80\n";
            return EXIT_FAILURE;
        }

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        ioc.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    evmone::state::Account init_account{
        .nonce = 0,
        .balance = 100000000000000000
    };

    evmone::state::BlockInfo block{
        .gas_limit = 1000000000000
    };

    evmone::state::State state;
    auto from = state.get_or_insert(0x0000000000000000000000000000000000000001_address, init_account);

    evmc_revision rev = {EVMC_CANCUN};

    evmc::VM vm{evmc_create_evmone()};

    std::string_view code_hex = "6080604052348015600f57600080fd5b5060a88061001e6000396000f3fe6080604052348015600f57600080fd5b506004361060325760003560e01c80638c0f57bc146037578063b4598503146053575b600080fd5b603d605b565b6040518082815260200191505060405180910390f35b60596064565b005b60008054905090565b60016000540160008190555056fea2646970667358221220cf73d15cfce3fc3bf0374db9315fe52577d84e1a44b1ed5121f98d741095d27d64736f6c634300060c0033";
    auto code_bytes = evmc::from_hex(code_hex);
    evmone::state::Transaction tx_contract_create{
        .kind = evmone::state::Transaction::Kind::legacy,
        .data = code_bytes.value(),
        .gas_limit = 2000000,
        .max_gas_price = 20,
        .max_priority_gas_price = 10,
        .sender = 0x0000000000000000000000000000000000000001_address,
        .to = std::nullopt,
        .value = 0,
        .chain_id = 0,
        .nonce = from.nonce,
    };
    auto computed_tx_hash = keccak256(rlp::encode(tx_contract_create));
    std::cout << evmc::hex(computed_tx_hash.bytes) << std::endl;

    auto result = evmone::state::transition(state, block, tx_contract_create, rev, vm);
    if (holds_alternative<evmone::state::TransactionReceipt>(result)) {
        evmone::state::TransactionReceipt receipt = std::get<evmone::state::TransactionReceipt>(result);
        std::cout << "status:" << receipt.status << ", gas_used:" << receipt.gas_used << std::endl;
        if(!receipt.logs.empty()) {
            std::cout << "logs:" << std::endl;
            for (auto log: receipt.logs) {
                std::cout << "addr:" << log.addr << ", data:" << evmc::hex(log.data) << std::endl;
            }
        }
    } else {
        std::error_code error = std::get<std::error_code>(result);
        std::cout << "Transaction failed. Error message: " << error.message() << std::endl;
    }

    for(auto acc : state.get_accounts()) {
        std::cout << acc.first << std::endl;
    }

    ///
    std::string_view input_hex = "0xb4598503";
    auto input_bytes = evmc::from_hex(input_hex);
    evmone::state::Transaction tx_contract_call{
            .kind = evmone::state::Transaction::Kind::legacy,
            .data = input_bytes.value(),
            .gas_limit = 2000000,
            .max_gas_price = 20,
            .max_priority_gas_price = 10,
            .sender = 0x0000000000000000000000000000000000000001_address,
            .to = 0x522b3294e6d06aa25ad0f1b8891242e335d3b459_address,
            .value = 0,
            .chain_id = 0,
            .nonce = from.nonce,
    };
    computed_tx_hash = keccak256(rlp::encode(tx_contract_call));
    std::cout << evmc::hex(computed_tx_hash.bytes) << std::endl;

    result = evmone::state::transition(state, block, tx_contract_call, rev, vm);
    if (holds_alternative<evmone::state::TransactionReceipt>(result)) {
        evmone::state::TransactionReceipt receipt = std::get<evmone::state::TransactionReceipt>(result);
        std::cout << "status:" << receipt.status << ", gas_used:" << receipt.gas_used << std::endl;
        if(!receipt.logs.empty()) {
            std::cout << "logs:" << std::endl;
            for (auto log: receipt.logs) {
                std::cout << "addr:" << log.addr << ", data:" << evmc::hex(log.data) << std::endl;
            }
        }
    } else {
        std::error_code error = std::get<std::error_code>(result);
        std::cout << "Transaction failed. Error message: " << error.message() << std::endl;
    }


    return 0;
}
