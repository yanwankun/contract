## 这是一个基于gxc得去中心化得交易所


## 合约提供了 存钱（deposite） 取现（withdraw） 挂单（pendingorder） 和取消挂单得功能（cancelorder）

## 编译合约
gxx -g gxcexchange.abi gxcexchange.cpp
gxx -o gxcexchange.wast gxcexchange.cpp


## 部署合约 合约名为 gxcexchange
deploy_contract gxcexchange yanwankun-test 0 0 ./contract/gxcexchange GXC true

## 合约调用

### 存钱
unlocked >>> call_contract yanwankun-test gxcexchange {"amount":200,"asset_id":1.3.16} deposit "{}" GXS true
call_contract yanwankun-test gxcexchange {"amount":200,"asset_id":1.3.16} deposit "{}" GXS true
{
  "ref_block_num": 39850,
  "ref_block_prefix": 865838005,
  "expiration": "2018-12-12T03:54:42",
  "operations": [[
      75,{
        "fee": {
          "amount": 500,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1043",
        "amount": {
          "amount": 200,
          "asset_id": "1.3.16"
        },
        "method_name": "deposit",
        "data": "",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "206280cb6939428ca49c03322937f5b56fc4f839819334bc13a690dd6a33eea9b55e45617111a0acd2cf8cdbc33f457267ea6d5875ae3030b561e1eaee7bf1db67"
  ]
}

call_contract yanwankun-test gxcexchange {"amount":200,"asset_id":1.3.1} deposit "{}" GXS true

## 取钱
unlocked >>> call_contract yanwankun-test gxcexchange null withdraw "{\"to_account\":\"yanwankun-test\", \"amount\":{\"asset_id\": 16, \"amount\":55}}" GXS true
call_contract yanwankun-test gxcexchange null withdraw "{\"to_account\":\"yanwankun-test\", \"amount\":{\"asset_id\": 16, \"amount\":55}}" GXS true
{
  "ref_block_num": 39919,
  "ref_block_prefix": 2878906590,
  "expiration": "2018-12-12T03:58:42",
  "operations": [[
      75,{
        "fee": {
          "amount": 500,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1043",
        "method_name": "withdraw",
        "data": "0e79616e77616e6b756e2d7465737437000000000000001000000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1f0ef1ad0c236e5deff967b5e42e5d64264aab654c13b562f78324b67c793653442faf608b5085d6accdf100b2a23c8f356f2ba2ce2c65416ced987eb7d1503e54"
  ]
}


### 挂单 挂一个 以单价 2WKYCOIN/1GXC 购买5个GXC 的买单
call_contract yanwankun-test gxcexchange null pendingorder "{\"type\":1, \"quantity\": {\"asset_id\": 1, \"amount\":5}, \"price\": 2}" GXC true

call_contract yanwankun-test gxcexchange null pendingorder "{\"type\":2, \"quantity\": {\"asset_id\": 1, \"amount\":5}, \"price\": 5}" GXC true



### 列出合约得所有存储表
get_contract_tables gxcexchange
[
  "account",
  "profit",
  "income",
  "buyorder",
  "sellorder",
  "dealorder"
]

