## 这是一个基于gxc得去中心化得交易所


## 合约提供了 存钱（deposite） 取现（withdraw） 挂单（pendingorder） 和取消挂单得功能（cancelorder）

## 编译合约
``````
gxx -g gxcexchangey.abi gxcexchangey.cpp
gxx -o gxcexchangey.wast gxcexchangey.cpp
``````

## 部署合约 合约名为 gxcexchangey
``````
deploy_contract gxcexchangey yanwankun-test 0 0 ./contract/gxcexchangey GXC true
``````

## 合约调用

### 存钱
``````
unlocked >>> call_contract yanwankun-test gxcexchangey {"amount":2000000,"asset_id":1.3.16} deposit "{}" GXS true
call_contract yanwankun-test gxcexchangey {"amount":200,"asset_id":1.3.16} deposit "{}" GXS true
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

call_contract yanwankun-test gxcexchangey {"amount":2000000,"asset_id":1.3.1} deposit "{}" GXS true
``````
## 取钱
``````
unlocked >>> call_contract yanwankun-test gxcexchangey null withdraw "{\"to_account\":\"yanwankun-test\", \"amount\":{\"asset_id\": 16, \"amount\":55}}" GXS true
call_contract yanwankun-test gxcexchangey null withdraw "{\"to_account\":\"yanwankun-test\", \"amount\":{\"asset_id\": 16, \"amount\":55}}" GXS true
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
``````

### 挂单 挂一个 以单价 2WKYCOIN/1GXC 购买5个GXC 的买单
``````
unlocked >>> call_contract yanwankun-test gxcexchangey null pendingorder "{\"type\":1, \"quantity\": {\"asset_id\": 1, \"amount\":2}, \"price\": 5}" GXC true
call_contract yanwankun-test gxcexchangey null pendingorder "{\"type\":1, \"quantity\": {\"asset_id\": 1, \"amount\":2}, \"price\": 5}" GXC true
{
  "ref_block_num": 43276,
  "ref_block_prefix": 197017848,
  "expiration": "2018-12-12T07:08:03",
  "operations": [[
      75,{
        "fee": {
          "amount": 500,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1060",
        "method_name": "pendingorder",
        "data": "01020000000000000001000000000000000500000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "207b4f0142777d7047648f998c13d31045ec62850a741682e7ea2e9b361e5af6006a4653188b895b42d4e95c0051283a4f710ae078fe93cf96b8f620f657872507"
  ]
}


call_contract yanwankun-test gxcexchangey null pendingorder "{\"type\":1, \"quantity\": {\"asset_id\": 1, \"amount\":5}, \"price\": 5}" GXC true
call_contract yanwankun-test gxcexchangey null pendingorder "{\"type\":2, \"quantity\": {\"asset_id\": 1, \"amount\":4}, \"price\": 4}" GXC true
call_contract yanwankun-test gxcexchangey null pendingorder "{\"type\":1, \"quantity\": {\"asset_id\": 1, \"amount\":2}, \"price\": 5}" GXC true
``````
### 撤销订单

``````
unlocked >>> call_contract yanwankun-test gxcexchangey null cancelorder "{\"type\":1, \"id\":1}" GXC true 
call_contract yanwankun-test gxcexchangey null cancelorder "{\"type\":1, \"id\":1}" GXC true
{
  "ref_block_num": 43852,
  "ref_block_prefix": 2703412265,
  "expiration": "2018-12-12T07:40:21",
  "operations": [[
      75,{
        "fee": {
          "amount": 500,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1060",
        "method_name": "cancelorder",
        "data": "010100000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1f314685c20de4d0825af2e763526c0ad7b4635c399b1b2254d9bf54f5c198125d6ab2821df08ee1c7f6011b43ddcb353ec7e95a59f346e3b18ce1c38c819be4db"
  ]
}

``````

### 列出合约得所有存储表
``````
get_contract_tables gxcexchangey
[
  "account",
  "profit",
  "income",
  "buyorder"
  "sellorder",
  "dealorder",
  "depositlog",
  "withdrawlog"
]
``````


## 查询还未匹配得买单
``````
unlocked >>> get_table_objects gxcexchangey buyorder 0 10 10
get_table_objects gxcexchangey buyorder 0 10 10
[{
    "id": 1,
    "price": 5,
    "quantity": {
      "amount": 2,
      "asset_id": 1
    },
    "buyer": 980,
    "order_time": 1544598453
  }
]
### 系统账户余额
get_table_objects gxcexchangey account 0 10 10
### 还未匹配得卖单
get_table_objects gxcexchangey sellorder 0 10 10
### 总收益
get_table_objects gxcexchangey income 15 19 10
### 收益表
get_table_objects gxcexchangey profit 0 10 10
### 匹配完成订单
get_table_objects gxcexchangey dealorder 0 10 10
### 充值记录
get_table_objects gxcexchangey depositlog 0 10 10
### 体现记录
get_table_objects gxcexchangey withdrawlog 15 19 10
``````

