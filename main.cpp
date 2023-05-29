#include "state/state.hpp"
#include "state/rlp.hpp"
#include "evmone/evmone.h"

#include <iostream>

using namespace evmone;

int main() {
    state::Account init_account{
        .nonce = 0,
        .balance = 100000000000000000
    };

    state::BlockInfo block{
        .gas_limit = 1000000000000
    };

    state::State state;
    auto from = state.get_or_insert(0x0000000000000000000000000000000000000001_address, init_account);

    evmc_revision rev = {EVMC_CANCUN};

    evmc::VM vm{evmc_create_evmone()};

    std::string_view code_hex = "6080604052348015600f57600080fd5b5060a88061001e6000396000f3fe6080604052348015600f57600080fd5b506004361060325760003560e01c80638c0f57bc146037578063b4598503146053575b600080fd5b603d605b565b6040518082815260200191505060405180910390f35b60596064565b005b60008054905090565b60016000540160008190555056fea2646970667358221220cf73d15cfce3fc3bf0374db9315fe52577d84e1a44b1ed5121f98d741095d27d64736f6c634300060c0033";
    auto code_bytes = evmc::from_hex(code_hex);
    state::Transaction tx_contract_create{
        .kind = state::Transaction::Kind::legacy,
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

    auto result = state::transition(state, block, tx_contract_create, rev, vm);
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
    state::Transaction tx_contract_call{
            .kind = state::Transaction::Kind::legacy,
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

    result = state::transition(state, block, tx_contract_call, rev, vm);
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
