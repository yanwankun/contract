### 合约编译
gxx -g exchangeywk.abi exchangeywk.cpp
gxx -o exchangeywk.wast exchangeywk.cpp

#### 合约部署
deploy_contract exchangeywk yanwankun-test 0 0 ./contract/exchangeywk GXC true

#### 合约更新
update_contract exchangeywk yanwankun-test ./contract/exchangeywk GXC true

contract: contract (type: string)
    new_owner: new_owner (type: string)
    contract_dir: contract_dir (type: string)
    fee_asset_symbol: fee_asset_symbol (type: string)
    broadcast: broadcast (type: bool)



#### 合约调用

call_contract yanwankun-test exchangeyan {"amount":20000000,"asset_id":1.3.18} ptdeposite "{}" GXS true

###### 限价售卖
call_contract yanwankun-test exchangeywk {"amount":2000000,"asset_id":1.3.16} pdsellorder "{\"price\": 10}" GXS true

````````
####说明 ：以0.00010 ywkcoin/1ytsyts 出售20 个ywkcoin
unlocked >>> call_contract yanwankun-test exchangeywk {"amount":2000000,"asset_id":1.3.16} pdsellorder "{\"price\": 10}" GXS true
call_contract yanwankun-test exchangeywk {"amount":2000000,"asset_id":1.3.16} pdsellorder "{\"price\": 10}" GXS true
{
  "ref_block_num": 61864,
  "ref_block_prefix": 2249774861,
  "expiration": "2018-12-18T06:25:27",
  "operations": [[
      75,{
        "fee": {
          "amount": 20314,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1200",
        "amount": {
          "amount": 2000000,
          "asset_id": "1.3.16"
        },
        "method_name": "pdsellorder",
        "data": "0a00000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "201397180f2090c4de1e13c3f8d2510c1d16337919b259a060ee4eea7090c39e4d378dd8b39a1d3efb74eb87343c67aa7718918479e7181fb330a3442f0665accb"
  ]
}
####　会生成一个限价卖单
unlocked >>> get_table_rows exchangeywk sellorder 0 100 100
get_table_rows exchangeywk sellorder 0 100 100
{
  "rows": [{
      "id": 0,
      "price": 10,
      "quantity": {
        "amount": 2000000,
        "asset_id": 16
      },
      "order_id": 0,
      "seller": 980,
      "order_time": 1545114297
    }
  ],
  "more": false
}
###### 合约账户会有一笔收入
unlocked >>> list_account_balances exchangeywk 
list_account_balances exchangeywk 
20 WKYCOIN

###### 出售着账户会有余额记录
unlocked >>> get_table_rows exchangeywk account 980 20 20
get_table_rows exchangeywk account 980 20 20
{
  "rows": [{
      "owner": 980,
      "balances": [{
          "amount": 2000000,
          "asset_id": 16
        }
      ]
    }
  ],
  "more": false
}
````````

###### 限价购买

call_contract yanwankun-test exchangeywk {"amount":2000000,"asset_id":1.3.18} pdbuyorder "{ \"quantity\": {\"asset_id\": 16, \"amount\":2500000},\"price\": 9}" GXS true
```````
#### 说明以 0.0009 ywkcoin/1ytsyts 购买 25.00000 个ywkcoin
unlocked >>> call_contract yanwankun-test exchangeywk {"amount":2000000,"asset_id":1.3.18} pdbuyorder "{ \"quantity\": {\"asset_id\": 16, \"amount\":2500000},\"price\": 9}" GXS true
call_contract yanwankun-test exchangeywk {"amount":2000000,"asset_id":1.3.18} pdbuyorder "{ \"quantity\": {\"asset_id\": 16, \"amount\":2500000},\"price\": 9}" GXS true
{
  "ref_block_num": 62053,
  "ref_block_prefix": 629156048,
  "expiration": "2018-12-18T06:37:39",
  "operations": [[
      75,{
        "fee": {
          "amount": 17951,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1200",
        "amount": {
          "amount": 2000000,
          "asset_id": "1.3.18"
        },
        "method_name": "pdbuyorder",
        "data": "a02526000000000010000000000000000900000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "205cb6b73b54c348373604c974b4e7e92d599b85b0b66d4d81ea8a5e96992f013405f71f95df0f517285fde980285f4c98ea52e0de1d8b6affad3fa1c6cb2e86f4"
  ]
}
### 生成一个买单
unlocked >>> get_table_rows exchangeywk buyorder 0 100 100  
get_table_rows exchangeywk buyorder 0 100 100
{
  "rows": [{
      "id": 0,
      "price": 9,
      "quantity": {
        "amount": 2500000,
        "asset_id": 16
      },
      "order_id": 1,
      "buyer": 980,
      "order_time": 1545115029
    }
  ],
  "more": false
}
#### 合约账户余额增加
unlocked >>> list_account_balances exchangeywk 
list_account_balances exchangeywk 
20 WKYCOIN
20 YTSYTS

#### 用户合约余额增加
unlocked >>> get_table_rows exchangeywk account 980 20 20
get_table_rows exchangeywk account 980 20 20
{
  "rows": [{
      "owner": 980,
      "balances": [{
          "amount": 2000000,
          "asset_id": 16
        },{
          "amount": 2000000,
          "asset_id": 18
        }
      ]
    }
  ],
  "more": false
}
```````
//@abi action
void cancelorder(uint8_t type, uint64_t id);

##### 订单撤销
###### 买单撤销
call_contract yanwankun-test exchangeywk null cancelorder "{\"type\":1, \"id\":0}" GXC true
``````

``````

//市场价格购买和售卖
// @abi action
// @abi payable
void mpbuy(contract_asset quantity);

// @abi action
// @abi payable
void mpsell();

#### 表查询
unlocked >>> get_contract_tables exchangeywk
get_contract_tables exchangeywk
[
  "account",
  "order",
  "buyorder",
  "sellorder",
  "dealorder"
]
get_table_rows exchangeywk order 0 20 20