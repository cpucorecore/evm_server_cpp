//
// Created by sky on 2023/5/30.
//

#include "evm_tx_executor.h"
#include "state/rlp.hpp"
#include <iostream>

using namespace evmone;

evmone::state::Account& evm_tx_executor::create_account(evmc::address addr, uint64_t nonce, uint64_t balance) {
    evmone::state::Account acc{
        .nonce = nonce,
        .balance = balance
    };
    return state.get_or_insert(addr, acc);
}

void evm_tx_executor::execute_tx(const evmone::state::Transaction &tx, evmc_revision rev) {
    auto computed_tx_hash = keccak256(rlp::encode(tx));
    std::cout << evmc::hex(computed_tx_hash.bytes) << std::endl;

    auto result = evmone::state::transition(state, block, tx, rev, vm);
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
}