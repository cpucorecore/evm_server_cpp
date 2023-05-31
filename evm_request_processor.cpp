//
// Created by sky on 2023/5/31.
//

#include "evm_request_processor.h"
#include "state/rlp.hpp"

#include <set>
#include <iostream>

using std::cout;
using std::endl;

using namespace evmone;

const std::set<const boost::string_view> uris_supported = {URI_CONTRACT_CREATE, URI_CONTRACT_CALL};

std::variant<json, std::error_code> evm_request_processor::do_request(const boost::string_view &uri, const char *req) {
    cout << "uri:[" << uri << "], request_json body:[" << req << "]" << endl;

    if(!uris_supported.contains(uri)) {
        return make_error_code(UNSUPPORTED_URI);
    }

    nlohmann::json request_json;
    nlohmann::json ret;

    try {
        request_json = nlohmann::json::parse(req);
    } catch (json::parse_error& e) {
        cout << "parse request_json body failed, err:" << e.what() << endl;
        return make_error_code(INVALID_JSON);
    }
    // TODO check request_json

    evmone::state::Transaction tx {
        .kind = DEFAULT_TX_KIND,
        .gas_limit = DEFAULT_TX_GAS_LIMIT,
        .max_gas_price = DEFAULT_MAX_GAS_PRICE,
        .max_priority_gas_price = DEFAULT_MAX_PRIORITY_GAS_PRICE,
        .value = request_json["value"],
        .chain_id = DEFAULT_CHAIN_ID,
    };

    auto from_addr = operator""_address(request_json["from"].get<std::string>().c_str());
    auto from_acc = executor.create_account(from_addr);
    tx.sender = from_addr;
    tx.nonce = from_acc.nonce;

    if(uri == URI_CONTRACT_CREATE) {
        tx.data = evmc::from_hex(request_json["code"].get<std::string_view>()).value();
    } else if(uri == URI_CONTRACT_CALL) {
        tx.to = operator""_address(request_json["to"].get<std::string>().c_str());
        tx.data = evmc::from_hex(request_json["input"].get<std::string_view>()).value();
        executor.execute_tx(tx);
    }

    auto tx_hash = evmc::hex(keccak256(rlp::encode(tx)));
    auto tx_ret = executor.execute_tx(tx);
    if (holds_alternative<tx_result>(tx_ret)) {
        auto result = std::get<tx_result>(tx_ret);

        ret["status"] = "success";
        ret["tx_hash"] = tx_hash;
        ret["evm_status"] = result.receipt.status;
        ret["gas_used"] = result.receipt.gas_used;
        if(!tx.to) {
            ret["contract_address"] = evmc::hex(result.addr_created);
        }
    } else {
        auto err = std::get<std::error_code>(tx_ret);

        ret["status"] = "fail";
        ret["error"] = err.message();
    }
    cout << "tx executed:" << ret.dump() << endl;
    return ret;
}