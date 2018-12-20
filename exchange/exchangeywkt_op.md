### 合约编译
gxx -g exchangeywkt.abi exchangeywkt.cpp
gxx -o exchangeywkt.wast exchangeywkt.cpp

#### 合约部署
deploy_contract exchangeywkt yanwankun-test 0 0 ./contract/exchangeywkt GXC true

#### 合约更新
update_contract exchangeywkt yanwankun-test ./contract/exchangeywkt GXC true

#### 合约初始化
call_contract yanwankun-test exchangeywkt null init "{}" GXS true
get_table_rows exchangeywkt sysconfig 0 100 100
``````
nlocked >>> call_contract yanwankun-test exchangeywkt null init "{}" GXS true
call_contract yanwankun-test exchangeywkt null init "{}" GXS true
{
  "ref_block_num": 2056,
  "ref_block_prefix": 2004098952,
  "expiration": "2018-12-18T11:28:51",
  "operations": [[
      75,{
        "fee": {
          "amount": 15100,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1203",
        "method_name": "init",
        "data": "",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1f5fad35369be1eb49c168fbd0e275ab86958f99dc2bece01ea58e58563e7caca124eb30bd1dcabd1891d5aaeba3da89def0c7eddc8edb49fb5cba4cb2a42df264"
  ]
}
unlocked >>> get_table_rows exchangeywkt sysconfig 0 100 100
get_table_rows exchangeywkt sysconfig 0 100 100
{
  "rows": [{  // 平台交易状态 1 锁定， 2 正常
      "id": 0, 
      "value": 2  
    },{ //收益账户id 只有这个人才可以获取平台的收益,系统级别得操作只能由这个账户发起
      "id": 1,
      "value": 980
    },{ // 一个订单中最多去和该数目个订单做撮合
      "id": 2,
      "value": 20
    },{ //  匹配金额得最大倍数
      "id": 3,
      "value": 2
    },{ //数据表保存得最大条数 当表得记录达到这个数目时会自动删除前
      "id": 4,
      "value": 10
    },{ //当表得记录达到最大条目时， 每增加 once_delete_count 就会自动删除最开始得 once_delete_count 条数目
      "id": 5,
      "value": 2
    }
  ],
  "more": false
}
unlocked >>>
``````
#### 添加交易支持得币种
call_contract yanwankun-test exchangeywkt null setcoin "{\"id\":16, \"value\":1}" GXS true
get_table_rows exchangeywkt cointype 0 100 100
``````
unlocked >>> call_contract yanwankun-test exchangeywkt null setcoin "{\"id\":16, \"value\":1}" GXS true
call_contract yanwankun-test exchangeywkt null setcoin "{\"id\":16, \"value\":1}" GXS true
{
  "ref_block_num": 2350,
  "ref_block_prefix": 4154432078,
  "expiration": "2018-12-18T11:43:51",
  "operations": [[
      75,{
        "fee": {
          "amount": 2463,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1203",
        "method_name": "addcointype",
        "data": "1000000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1f50653f50398bc716f47ffbd06d0ae2b2ac09e68c708d9f186f34a82e9b78bd8c44785ac50803f76ab566c02ddbe49e6f82ce82032696b2550e9766baa1f4a3d2"
  ]
}
unlocked >>> get_table_rows exchangeywkt cointype 0 100 100
get_table_rows exchangeywkt cointype 0 100 100
{
  "rows": [{
      "id": 16,
      "value": 1
    }
  ],
  "more": false
}
``````
#### 添加平台币种
call_contract yanwankun-test exchangeywkt null setptcoin "{\"id\":18, \"value\":1}" GXS true
``````
unlocked >>> call_contract yanwankun-test exchangeywkt null setptcoin "{\"id\":18, \"value\":1}" GXS true
call_contract yanwankun-test exchangeywkt null setptcoin "{\"id\":18, \"value\":1}" GXS true
{
  "ref_block_num": 54726,
  "ref_block_prefix": 3879375734,
  "expiration": "2018-12-20T11:04:18",
  "operations": [[
      75,{
        "fee": {
          "amount": 2463,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1203",
        "method_name": "setptcoin",
        "data": "120000000000000001",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "2057d210f33bdbc557d02c8b9dfcfc555a1ccb4836796e49ce0f3c5209ecb20d484a286527a0c5630633b1c40b274adb89ba203fa15e792b3720c37ba3ec9eeafc"
  ]
}
unlocked >>> get_table_rows exchangeywkt ptcointype 0 100 100
get_table_rows exchangeywkt ptcointype 0 100 100
{
  "rows": [{
      "id": 18,
      "value": 1
    }
  ],
  "more": false
}
``````
#### 合约调用

###### 限价售卖
call_contract yanwankun-test exchangeywkt {"amount":2000000,"asset_id":1.3.16} pdsellorder "{\"price\": 1000000, \"core_asset_id\":18}" GXS true
get_table_rows exchangeywkt order 0 100 100

````````
以1000000 ywkcoin/1ytsyts 出售20 个ywkcoin
unlocked >>> call_contract yanwankun-test exchangeywkt {"amount":2000000,"asset_id":1.3.16} pdsellorder "{\"price\": 1000000, \"core_asset_id\":18}" GXS true
call_contract yanwankun-test exchangeywkt {"amount":2000000,"asset_id":1.3.16} pdsellorder "{\"price\": 1000000, \"core_asset_id\":18}" GXS true
{
  "ref_block_num": 54777,
  "ref_block_prefix": 2853840896,
  "expiration": "2018-12-20T11:07:06",
  "operations": [[
      75,{
        "fee": {
          "amount": 20783,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1203",
        "amount": {
          "amount": 2000000,
          "asset_id": "1.3.16"
        },
        "method_name": "pdsellorder",
        "data": "40420f00000000001200000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1f273e2c5c369098eaee7b5eb0b4640cd072f299a66689ff08c4055f7b09f7a7ae2eb1068eaa9ff168d73b57d520d1d93e0605ea7a12ea7bbd8a859b19fd29fd96"
  ]
}
出售着账户会有余额记录
nlocked >>> get_table_rows exchangeywkt account 980 20 20
get_table_rows exchangeywkt account 980 20 20
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
生成得挂单
unlocked >>> get_table_rows exchangeywkt order 0 100 100
get_table_rows exchangeywkt order 0 100 100
{
  "rows": [{
      "id": 0,
      "pay_amount": {
        "amount": 2000000,
        "asset_id": 16
      },
      "amount": {
        "amount": 2000000,
        "asset_id": 16
      },
      "user": 980,
      "order_type": 2,
      "core_asset_id": 18,
      "price": 1000000,
      "deal_price": 1000000,
      "deal_amount": 0,
      "un_deal_amount": 2000000,
      "status": 1,
      "fee_amount": 0,
      "refund_amount": {
        "amount": 0,
        "asset_id": 0
      },
      "order_time": 1545303996
    }
  ],
  "more": false
}
会生成一个限价卖单
unlocked >>> get_table_rows exchangeywkt sellorder 0 100 100
get_table_rows exchangeywkt sellorder 0 100 100
{
  "rows": [{
      "id": 0,
      "core_asset_id": 18,
      "price": 1000000,
      "quantity": {
        "amount": 2000000,
        "asset_id": 16
      },
      "order_id": 0,
      "seller": 980,
      "order_time": 1545303996
    }
  ],
  "more": false
} 
合约账户会有一笔收入
unlocked >>> list_account_balances exchangeywkt
list_account_balances exchangeywkt
20 WKYCOIN
0 YTSYTS
````````

###### 限价购买

call_contract yanwankun-test exchangeywkt {"amount":22050000,"asset_id":1.3.18} pdbuyorder "{ \"quantity\": {\"asset_id\": 16, \"amount\":2500000},\"price\": 800000}" GXS true
```````
#### 说明以 0.0009 ywkcoin/1ytsyts 购买 25.00000 个ywkcoin
unlocked >>> call_contract yanwankun-test exchangeywkt {"amount":2000000,"asset_id":1.3.18} pdbuyorder "{ \"quantity\": {\"asset_id\": 16, \"amount\":2500000},\"price\": 9}" GXS true
call_contract yanwankun-test exchangeywkt {"amount":2000000,"asset_id":1.3.18} pdbuyorder "{ \"quantity\": {\"asset_id\": 16, \"amount\":2500000},\"price\": 9}" GXS true
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
unlocked >>> get_table_rows exchangeywkt buyorder 0 100 100  
get_table_rows exchangeywkt buyorder 0 100 100
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
unlocked >>> list_account_balances exchangeywkt 
list_account_balances exchangeywkt 
20 WKYCOIN
20 YTSYTS

#### 用户合约余额增加
unlocked >>> get_table_rows exchangeywkt account 980 20 20
get_table_rows exchangeywkt account 980 20 20
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
call_contract yanwankun-test exchangeywkt null cancelorder "{\"type\":1, \"id\":0}" GXC true
``````
unlocked >>> call_contract yanwankun-test exchangeywkt null cancelorder "{\"type\":1, \"id\":1}" GXC true
call_contract yanwankun-test exchangeywkt null cancelorder "{\"type\":1, \"id\":1}" GXC true
{
  "ref_block_num": 2823,
  "ref_block_prefix": 161897682,
  "expiration": "2018-12-18T12:07:51",
  "operations": [[
      75,{
        "fee": {
          "amount": 100,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1203",
        "method_name": "cancelorder",
        "data": "010100000000000000",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1f1b54ac052bd422601beed5f534b65fad968c74b94cccbb820284a9972b39c2e000869b7cf862abc00b1d2029e06283f398cd05ab74fc336beb041c5270d762d4"
  ]
}
``````

//市场价格购买和售卖
// @abi action
// @abi payable
void mpbuy(contract_asset quantity);

// @abi action
// @abi payable
void mpsell();

#### deleteall 删除由所有数据
call_contract yanwankun-test exchangeywkt null deleteall "{}" GXS true
``````
unlocked >>> call_contract yanwankun-test exchangeywkt null deleteall "{}" GXS true
call_contract yanwankun-test exchangeywkt null deleteall "{}" GXS true
{
  "ref_block_num": 2208,
  "ref_block_prefix": 688162573,
  "expiration": "2018-12-18T11:36:36",
  "operations": [[
      75,{
        "fee": {
          "amount": 100,
          "asset_id": "1.3.1"
        },
        "account": "1.2.980",
        "contract_id": "1.2.1203",
        "method_name": "deleteall",
        "data": "",
        "extensions": []
      }
    ]
  ],
  "extensions": [],
  "signatures": [
    "1f1398404096ed8483eb489b3c0368854d5383da611db6bbd1333575a14b79880b0afa309cb679f4d1542417cbf80430b2adaf2fd86648276613dca4ba582c3cb1"
  ]
}
unlocked >>> get_table_rows exchangeywkt sysconfig 0 100 100
get_table_rows exchangeywkt sysconfig 0 100 100
{
  "rows": [],
  "more": false
}
``````
#### 表查询
unlocked >>> get_contract_tables exchangeywkt
get_contract_tables exchangeywkt
[
  "account",
  "order",
  "buyorder",
  "sellorder",
  "dealorder"
]
get_table_rows exchangeywkt order 0 20 20