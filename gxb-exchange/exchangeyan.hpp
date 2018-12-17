#include <graphenelib/asset.h>
#include <graphenelib/contract.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/global.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/system.h>
#include <vector>

using namespace graphene;

const uint8_t buy_order_type = 1; //  买单类型
const uint8_t sell_order_type = 2; //  买单类型
const uint64_t platform_lock_status_value = 1; // 平台交易锁定
const uint64_t platform_un_lock_status_value = 2; // 平台交易未锁定
const uint64_t platform_core_asset_id_VALUE = 18; // 平台核心资产id YTSYTS

const uint64_t profit_account_id_ID = 1;
const uint64_t platform_status_ID = 0;  
const uint64_t max_match_order_count_ID = 2;
const uint64_t match_amount_times_ID = 3;

const uint64_t table_save_count_ID = 4;
const uint64_t once_delete_count_ID = 5;

const uint64_t profit_account_id_VALUE = 980; //收益账户id 只有这个人才可以获取平台的收益
const uint64_t match_amount_times_VALUE = 2; //  匹配金额得最大倍数
const uint64_t max_match_order_count_VALUE = 20; // 一个订单中最多去和该数目个订单做撮合

const uint64_t table_save_count_VALUE = 10; //数据表保存得最大条数 当表得记录达到这个数目时会自动删除前
const uint64_t once_delete_count_VALUE = 2; //当表得记录达到最大条目时， 每增加 once_delete_count 就会自动删除最开始得 once_delete_count 条数目

// 保证金相关配置
const uint64_t ptcoin_trade_coin_id = 16; //能够购买和卖出平台得可交易币得id WKYCOIN
const int64_t  ptcoin_ratio = 20; //平台币和gxc得兑换比率 这个不可更改
const uint64_t ptcoin_trade_fee_ratio = 1; //购买和卖出平台币得手续费百分比
const uint64_t ptcoin_trade_fee_min = 40000; //购买和卖出平台币得最小手续费
const uint64_t ptcoin_trade_fee_max = 200000; //购买和卖出平台币得最大手续费
const uint64_t plateform_deposite_platecoin_ID = 0; //平台的平台币 这个可以提走
const uint64_t plateform_deposite_gxc_ID = 1;  // 平台的gxc资产id，这个也可以提走
const uint64_t plateform_deposite_lock_gxc_ID = 2; //平台锁定的gxc资产id 这个是为了保证智能合约内部得平台币和gxc得自由兑换

const uint64_t ptcoin_lock_not_all_min = 100000000; //当平台保证金小于这个数目(1000GXC)时,
const uint64_t ptcoin_lock_ratio = 20; //保证金锁定比率
const uint64_t asset_recision = 100000;

class exchangeyan : public contract
{
  public:
    exchangeyan(uint64_t account_id)
        : contract(account_id)
        , accounts(_self, _self)
        , buyorders(_self, _self)
        , sellorders(_self, _self)
        , profits(_self, _self)
        , incomes(_self, _self)
        , dealorders(_self, _self)
        , depositlogs(_self, _self)
        , withdrawlogs(_self, _self)
        , sysconfigs(_self, _self)
        , pledges(_self, _self)
    {
    }


    // @abi action
    // @abi payable
    void deposit();

    // @abi action
    void withdraw(std::string to_account, contract_asset amount);

    //@abi action
    void pendingorder(uint8_t type, contract_asset quantity, int64_t price);

    //@abi action
    void cancelorder(uint8_t type, uint64_t id);

    //@abi action
    void fetchprofit(std::string to_account, contract_asset amount);

    // @abi action
    void updateconfig(uint64_t id, uint64_t value);

    // @abi action
    void init();

    // @abi action
    void deleteall();

    // @abi action
    // @abi payable
    void ptdeposite(); //平台储值

    // @abi action
    void ptwithdraw(contract_asset amount); //平台体现

    // @abi action
    // @abi payable
    void buyptcoin(); // 购买平台币

    // @abi action
    void sellptcoin(contract_asset amount); //卖出平台币

    // 账户记录
    //@abi table account i64
    struct account {
        uint64_t owner;
        // 可用余额
        std::vector<contract_asset> balances;
        // 锁定余额 (挂单中得余额)
        std::vector<contract_asset> lock_balances;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(balances)(lock_balances))
    };

    // 收入记录
    //@abi table profit i64
    struct profit {
        uint64_t id;
        contract_asset profit_asset;
        uint64_t profit_time;

        uint64_t primary_key() const { return id; }
        GRAPHENE_SERIALIZE(profit, (id)(profit_asset)(profit_time))
    };

    // 总收入
    //@abi table income i64
    struct income {
        uint64_t asset_id;
        uint64_t amount;

        uint64_t primary_key() const { return asset_id; }
        GRAPHENE_SERIALIZE(income, (asset_id)(amount))
    };

    // 买单列表 
    //@abi table buyorder i64
    struct buyorder {
        uint64_t id;
        int64_t price;
        contract_asset quantity;
        uint64_t buyer;
        int64_t  order_time;
        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return price; }
        uint64_t get_asset() const { return quantity.asset_id; }
        uint64_t get_sender() const { return buyer; }
        GRAPHENE_SERIALIZE(buyorder, (id)(price)(quantity)(buyer)(order_time))
    };

    // 卖单列表
    //@abi table sellorder i64
    struct sellorder {
        uint64_t id;
        int64_t price;
        contract_asset quantity;
        uint64_t seller;
        int64_t  order_time;
        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return price; }
        uint64_t get_asset() const { return quantity.asset_id; }
        uint64_t get_sender() const { return seller; }
        GRAPHENE_SERIALIZE(sellorder, (id)(price)(quantity)(seller)(order_time))
    };

    // 成交订单列表
    // @abi table dealorder i64
    struct dealorder {
        uint64_t id;
        // 买入价格
        int64_t price;
        contract_asset quantity;
        int64_t  order_time;

        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return price; }
        uint64_t get_asset() const { return quantity.asset_id; }
        GRAPHENE_SERIALIZE(dealorder, (id)(price)(quantity)(order_time))
    };

    // 充值记录表
    // @abi table depositlog i64
    struct depositlog {
        uint64_t id;
        uint64_t user;
        contract_asset quantity;
        int64_t  order_time;

        uint64_t primary_key() const { return id; }
        uint64_t get_user() const { return user; }
        uint64_t get_asset() const { return quantity.asset_id; }
        GRAPHENE_SERIALIZE(depositlog, (id)(user)(quantity)(order_time))
    };

    // 提现记录表
    // @abi table withdrawlog i64
    struct withdrawlog {
        uint64_t id;
        uint64_t user;
        contract_asset quantity;
        int64_t  order_time;

        uint64_t primary_key() const { return id; }
        uint64_t get_user() const { return user; }
        uint64_t get_asset() const { return quantity.asset_id; }
        GRAPHENE_SERIALIZE(withdrawlog, (id)(user)(quantity)(order_time))
    };

    // 系统配置表
    // @abi table sysconfig i64
    struct sysconfig {
        uint64_t id;
        uint64_t value;

        uint64_t primary_key() const { return id; }
        GRAPHENE_SERIALIZE(sysconfig, (id)(value))
    };

    // 保证表
    // @abi table pledge i64
    struct pledge {
        uint64_t id;
        contract_asset amount;

        uint64_t primary_key() const { return id; }
        GRAPHENE_SERIALIZE(pledge, (id)(amount))
    };

    void add_balances(uint64_t user, contract_asset quantity);
    void sub_balances(uint64_t user, contract_asset quantity);
    void add_balances_lock(uint64_t user, contract_asset quantity);
    void sub_balances_lock(uint64_t user, contract_asset quantity);
    void add_income(contract_asset profit);
    void sub_income(contract_asset profit);
    void exchange_transfer(uint64_t from, uint64_t to, contract_asset quantity); 
    void transfer_from_lock(uint64_t from, uint64_t to, contract_asset quantity);
    void balance_lock(uint64_t user, contract_asset quantity);
    void unlock_lock_balance(uint64_t user, contract_asset quantity);
    void update_sell_order(uint64_t id, contract_asset quantity);
    void update_buy_order(uint64_t id, contract_asset quantity);
    void sell_order_fun(contract_asset quantity, int64_t price, uint64_t seller);
    void buy_order_fun(contract_asset quantity, int64_t price, uint64_t buyer);
    void cancel_sell_order_fun(uint64_t id, uint64_t seller);
    void cancel_buy_order_fun(uint64_t id, uint64_t buyer);

    // 数据插入
    uint64_t insert_sell_order(uint64_t seller, contract_asset quantity, int64_t price);
    uint64_t insert_buy_order(uint64_t buyer, contract_asset quantity, int64_t price);
    uint64_t insert_profit(contract_asset profit);
    uint64_t insert_dealorder(int64_t price, contract_asset quantity);
    uint64_t insert_depositlog(uint64_t buyer, contract_asset amount);
    uint64_t insert_withdrawlog(uint64_t buyer, contract_asset amount);
    // 配置相关
    void insert_sysconfig(uint64_t id, uint64_t value);
    void update_sysconfig(uint64_t id, uint64_t value);
    uint64_t get_sysconfig(uint64_t id);
    void delete_sysconfig();
    // 保证金相关
    void add_pledge(uint64_t id, contract_asset amount);
    void sub_pledge(uint64_t id, contract_asset amount);
    void delete_pledge();

    // 数据删除相关
    void delete_profit(uint64_t deletecount);
    void delete_depositlog(uint64_t deletecount);
    void delete_withdrawlog(uint64_t deletecount);
    void delete_dealorder(uint64_t deletecount);

    void authverify(uint64_t sender); // 判断是否是收益所有者在操作
    void statusverify(); // 判断交易所状态是否

    void ptcoin_lock(); // 平台资产锁定 当用户卖出平台资产的时候，如果锁定资产不足以支付用户资产得时候，就会把用户得保证金转入到锁定账户

  private:
    
    typedef graphene::multi_index<N(account), account> account_index;
    typedef graphene::multi_index<N(buyorder), buyorder,
                        indexed_by<N(price), const_mem_fun<buyorder, uint64_t, &buyorder::get_price>>,
                        indexed_by<N(sender), const_mem_fun<buyorder, uint64_t, &buyorder::get_sender>>,
                        indexed_by<N(asset), const_mem_fun<buyorder, uint64_t, &buyorder::get_asset>>
                        > buyorder_index;
    typedef graphene::multi_index<N(sellorder), sellorder,
                        indexed_by<N(price), const_mem_fun<sellorder, uint64_t, &sellorder::get_price>>,
                        indexed_by<N(sender), const_mem_fun<sellorder, uint64_t, &sellorder::get_sender>>,
                        indexed_by<N(asset), const_mem_fun<sellorder, uint64_t, &sellorder::get_asset>>
    > sellorder_index;
    typedef graphene::multi_index<N(dealorder), dealorder,
                        indexed_by<N(price), const_mem_fun<dealorder, uint64_t, &dealorder::get_price>>,
                        indexed_by<N(asset), const_mem_fun<dealorder, uint64_t, &dealorder::get_asset>>
    > dealorder_index;
    typedef graphene::multi_index<N(profit), profit> profit_index;
    typedef graphene::multi_index<N(income), income> income_index;
    typedef graphene::multi_index<N(depositlog), depositlog,
                        indexed_by<N(user), const_mem_fun<depositlog, uint64_t, &depositlog::get_user>>,
                        indexed_by<N(asset), const_mem_fun<depositlog, uint64_t, &depositlog::get_asset>>
    > depositlog_index;
    typedef graphene::multi_index<N(withdrawlog), withdrawlog,
                        indexed_by<N(user), const_mem_fun<withdrawlog, uint64_t, &withdrawlog::get_user>>,
                        indexed_by<N(asset), const_mem_fun<withdrawlog, uint64_t, &withdrawlog::get_asset>>
    > withdrawlog_index;
    typedef graphene::multi_index<N(sysconfig), sysconfig> sysconfig_index;
    typedef graphene::multi_index<N(pledge), pledge> pledge_index;

    account_index accounts;
    buyorder_index buyorders;
    sellorder_index sellorders;
    profit_index profits;
    income_index incomes;
    dealorder_index dealorders;
    depositlog_index depositlogs;
    withdrawlog_index withdrawlogs;
    sysconfig_index sysconfigs;
    pledge_index pledges;

};
