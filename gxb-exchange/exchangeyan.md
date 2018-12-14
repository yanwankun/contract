#exchangeyan
##  这是一个基于gxc得去中心化得币币之间交易所，完全去中心化，用户可以基于代码相信交易得安全信。

## 提供得方法

### 系统层面
#### 初始化 :这个方法得主要目的是初始化一些可变参数，将这些参数放在sysconfig这个表中，代码中的可变参数就是从这个表中获取得
> call_contract yanwankun-test exchangeyan null init "{}" GXS true

#### 平台存储 ：这个方法主要是实现平台保证金和平台资金得预存，合约得创建者可以将足量得平台币放在合约对应得表里面，交易中心得使用者可以直接从平台上购买平台币，用户也可以把一定额度得可以用来购买平台币得资产放在合约里面，当用户购买平台币锁定得资产不足以支付用户卖出平台币得金额时，会将用户预存资产转入到锁定资产，以用于下次用户卖出平台资产得支付。

>call_contract yanwankun-test exchangeyan {"amount":20000000,"asset_id":1.3.18} ptdeposite "{}" GXS true
>call_contract yanwankun-test exchangeyan {"amount":20000000,"asset_id":1.3.16} ptdeposite "{}" GXS true

该方法只支持两种资产的操作。
1 存入平台资产，用于用户购买
2 存入保证金，用于支付用户卖出平台资产

#### 平台提现
> call_contract yanwankun-test-1 exchangeyan null ptwithdraw "{\"amount\": {\"asset_id\": 16, \"amount\":500000}}" GXC true
可以将所欲者未锁定得保证金取出

#### 提取收益
> call_contract yanwankun-test-1 exchangeyan null ptwithdraw "{\"amount\": {\"asset_id\": 16, \"amount\":500000}}" GXC true

#### 更改配置
> call_contract yanwankun-test exchangeyan null updateconfig "{\"id\":1, \"value\": 1}" GXS true
id就是sysconfig表得id，value就是配置得值

#### 删除所有
> call_contract yanwankun-test exchangeyan null deleteall "{}" GXS true
清空合约所有数据，将用户得资产体现，平台收益转移到收益者账户



### 用户层面
#### 购买平台资产
>call_contract yanwankun-test-1 exchangeyan {"amount":10000000,"asset_id":1.3.16} buyptcoin "{}" GXS true
用户可以通过购买平台资产来和别人进行交易，因为币与币之间的交易只能通过平台币，用户也可以存入其他资产卖出以获取平台币

#### 用户存入资产
> call_contract yanwankun-test-1 exchangeyan {"amount":2000000,"asset_id":1.3.1} deposit "{}" GXS true
用户可以吧资产存储在台上进行交易和体现

#### 用户提现
> call_contract yanwankun-test-1 exchangeyan null withdraw "{\"to_account\":"yanwankun-test-1", \"amount\": {\"asset_id\": 1, \"amount\":500000}}" GXC true
将用户资产提取到指定账户上面

#### 用户挂单
> call_contract yanwankun-test-1 exchangeyan null pendingorder "{\"type\":1, \"quantity\": {\"asset_id\": 1, \"amount\":500000}, \"price\": 10}" GXC true
用户可以挂一个买单和卖单

#### 用户取消挂单
> call_contract yanwankun-test-1 exchangeyan null cancelorder "{\"type\":1, \"id\":0}" GXC true

#### 卖出平台币
> call_contract yanwankun-test-1 exchangeyan null sellptcoin "{\"amount\": {\"asset_id\": 18, \"amount\":500000}}" GXC true


#表说明

#"account" ： 记录了用户得账户信息，包括余额和冻结金额，冻结金额是由于用户挂单产生的金额
#"profit", ： 记录了平台的收益记录，当交易有差价产生时，差价会记录到平台得收益里面
#"income", ： 记录了平台当前得总收益，此部分金额是可以提现的
#"buyorder",： 记录了现在还没有匹配成功得买单记录
#"sellorder", ：记录了还没有匹配成功得卖单记录
#"dealorder", : 交易成功得订单记录，记录了交易金额和成交价，以买方价格为准
#"depositlog",: 充值记录，
#"withdrawlog",： 体现记录
#"sysconfig", ： 配置表
#"pledge" ： 保证金记录表




