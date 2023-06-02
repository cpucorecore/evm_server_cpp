//
// Created by sky on 2023/6/2.
//

#ifndef EVM_SERVER_CPP_HELP_MESSAGE_H
#define EVM_SERVER_CPP_HELP_MESSAGE_H

#include <string>

static constexpr auto default_help_message = R"delimiter(
<html>
<head><title>evm server help</title></head>

<body>
<h1>readme</h1>
<h2>how to call the service:</h2>
<h3>1. create contract</h3>
<p>
curl -d "@create_contract.json" localhost:9001/contract/create<br/><br/>

create_contract.json:<br/><br/>
{<br/>
  "from": "0x00000000000000000000000000000000000003e8",<br/>
  "value": 0,<br/>
  "input": "",<br/>
  "code": "6080604052348015600f57600080fd5b5060a88061001e6000396000f3fe6080604052348015600f57600080fd5b506004361060325760003560e01c80638c0f57bc146037578063b4598503146053575b600080fd5b603d605b565b6040518082815260200191505060405180910390f35b60596064565b005b60008054905090565b60016000540160008190555056fea2646970667358221220cf73d15cfce3fc3bf0374db9315fe52577d84e1a44b1ed5121f98d741095d27d64736f6c634300060c0033
"<br/>
}<br/>
</p>

<h3>2. call contract</h3>
<p>
curl -d "@call_contract.json" localhost:9001/contract/call<br/><br/>

call_contract.json:<br/><br/>
{<br/>
  "from": "0x00000000000000000000000000000000000003e8",<br/>
  "to": "0x771207d02d9b8Cef2DD4F935c7E298A54fbf73bc",<br/>
  "value": 0,<br/>
  "input": "0xb4598503"<br/>
}<br/>
</p>

</body>
</html>

)delimiter";

const std::string& load_help_html_file();

#endif //EVM_SERVER_CPP_HELP_MESSAGE_H
