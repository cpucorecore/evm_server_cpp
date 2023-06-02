# evm server
- based on [evmc](https://github.com/ethereum/evmc) and [evmone](https://github.com/ethereum/evmone)
- add http server to handle evm request

# todos
- [ ] persistent store
- [ ] state query
- [ ] state root hash
- [ ] more parameters for evm call, like max_gas, evm_version...

# compile
test on mac(m1) osx and ubuntu 20.04LTS

## mac
```bash
mkdir build
cd build
cmake ..
make
```

## ubuntu 20.04
must install gcc-10,g++-10
```
sudo apt install gcc-10 g++-10
```
after install follow the compile step by mac

# run
```bash
./evm_server_cpp 127.0.0.1 9001
```

# test
open the link "localhost:9001" in your browser to get help

or:
```bash
cd test
bash create_contract.sh 
bash call_contract.sh 
```