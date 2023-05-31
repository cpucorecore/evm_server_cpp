//
// Created by sky on 2023/5/31.
//

#ifndef EVM_SERVER_CPP_EVM_REQUEST_PROCESSOR_H
#define EVM_SERVER_CPP_EVM_REQUEST_PROCESSOR_H


#include "evm_tx_executor.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;

const auto DEFAULT_TX_KIND = evmone::state::Transaction::Kind::legacy;
const int64_t DEFAULT_TX_GAS_LIMIT = 2000000;
const int64_t DEFAULT_MAX_GAS_PRICE = 2;
const int64_t DEFAULT_MAX_PRIORITY_GAS_PRICE = 1;
const uint64_t DEFAULT_CHAIN_ID = 0;

enum ErrorCode : int {
    SUCCESS = 0,
    INVALID_JSON,
    UNSUPPORTED_URI,
};

struct Category : std::error_category {
    [[nodiscard]] const char* name() const noexcept final { return "evm_request_processor"; }

    [[nodiscard]] std::string message(int ev) const noexcept final {
        switch (ev) {
            case SUCCESS:
                return "success";
            case INVALID_JSON:
                return "invalid json";
            case UNSUPPORTED_URI:
                return "unsupported uri";
            default:
                return "Wrong error code";
        }
    }
};

const Category category;

inline std::error_code make_error_code(ErrorCode ec) noexcept {
    return {ec, category};
}

const std::string URI_CONTRACT_CREATE = "/contract/create";
const std::string URI_CONTRACT_CALL = "/contract/call";


class evm_request_processor {
public:
    std::variant<json, std::error_code> do_request(const std::string &uri, const char *req);

private:
    evm_tx_executor executor;
};


#endif //EVM_SERVER_CPP_EVM_REQUEST_PROCESSOR_H
