//
// Created by sky on 2023/5/30.
//

#ifndef EVM_SERVER_CPP_EVM_TX_EXECUTOR_H
#define EVM_SERVER_CPP_EVM_TX_EXECUTOR_H


#include "state/state.hpp"

#include "evmone/evmone.h"

const int64_t BLOCK_GAS_LIMIT = 1000000000000;
const int64_t DEFAULT_BALANCE = 100000000;

class evm_tx_executor {
public:
    evm_tx_executor() : vm(evmc_create_evmone()){
        block.gas_limit = BLOCK_GAS_LIMIT;
    };

    evmone::state::Account& create_account(evmc::address addr, uint64_t nonce = 0, uint64_t balance = DEFAULT_BALANCE);
    void execute_tx(const evmone::state::Transaction &tx, evmc_revision rev = EVMC_CANCUN);

private:
    evmone::state::BlockInfo block;
    evmone::state::State state;
    evmc::VM vm;
};


#endif //EVM_SERVER_CPP_EVM_TX_EXECUTOR_H
