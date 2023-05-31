//
// Created by sky on 2023/5/30.
//

#include "evm_tx_executor.h"
#include <iostream>

using namespace evmone;

evmone::state::Account& evm_tx_executor::create_account(evmc::address addr, uint64_t nonce, uint64_t balance) {
    evmone::state::Account acc{
        .nonce = nonce,
        .balance = balance
    };
    return state.get_or_insert(addr, acc);
}

std::variant<tx_result, std::error_code> evm_tx_executor::execute_tx(const evmone::state::Transaction &tx, evmc_revision rev) {
    auto result = evmone::state::transition(state, block, tx, rev, vm);
    if (holds_alternative<evmone::state::TransactionReceipt>(result)) {
        tx_result ret;

        auto receipt = std::get<evmone::state::TransactionReceipt>(result);
        if(!tx.to) {
            ret.addr_created = evmone::state::get_address_created();
        }

        ret.receipt = receipt;
        return ret;
    } else {
        std::error_code error = std::get<std::error_code>(result);
        std::cout << "Transaction failed. Error message: " << error.message() << std::endl;
        return error;
    }
}