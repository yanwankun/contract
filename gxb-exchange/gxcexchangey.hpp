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
const uint64_t platform_core_asset_id = 16; // 平台核心资产id WKYCOIN

class gxcexchangey : public contract
{
  public:
    gxcexchangey(uint64_t account_id)
        : contract(account_id)
        , accounts(_self, _self)
        , buyorders(_self, _self)
        , sellorders(_self, _self)
        , profits(_self, _self)
        , incomes(_self, _self)
        , dealorders(_self, _self)
        , depositlogs(_self, _self)
        , withdrawlogs(_self, _self)
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

    // 账户记录
    //@abi table account i64
    struct account {
        uint64_t owner;
        // 可用余额
        std::vector<contract_asset> balances;
        // 锁定余额 (挂单中得余额)
        std::vector<contract_asset> lock_balances;

        uint64_t primary_key() const { return owner; }

        GRAPHENE_SERIALIZE(account, (owner)(balances))
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

    // 体现记录表
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

    void add_balances(uint64_t user, contract_asset quantity);
    void sub_balances(uint64_t user, contract_asset quantity);
    void add_balances_lock(uint64_t user, contract_asset quantity);
    void sub_balances_lock(uint64_t user, contract_asset quantity);
    void exchange_transfer(uint64_t from, uint64_t to, contract_asset quantity); 
    void transfer_from_lock(uint64_t from, uint64_t to, contract_asset quantity);
    void balance_lock(uint64_t user, contract_asset quantity);
    void unlock_lock_balance(uint64_t user, contract_asset quantity);
    void update_sell_order(uint64_t id, contract_asset quantity);
    void update_buy_order(uint64_t id, contract_asset quantity);
    void insert_sell_order(uint64_t seller, contract_asset quantity, int64_t price);
    void insert_buy_order(uint64_t buyer, contract_asset quantity, int64_t price);
    void insert_profit(contract_asset profit);
    void insert_dealorder(int64_t price, contract_asset quantity);
    void insert_depositlog(uint64_t buyer, contract_asset amount);
    void insert_withdrawlog(uint64_t buyer, contract_asset amount);
    void add_income(contract_asset profit);
    void sell_order_fun(contract_asset quantity, int64_t price, uint64_t seller);
    void buy_order_fun(contract_asset quantity, int64_t price, uint64_t buyer);
    void cancel_sell_order_fun(uint64_t id, uint64_t seller);
    void cancel_buy_order_fun(uint64_t id, uint64_t buyer);

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

    account_index accounts;
    buyorder_index buyorders;
    sellorder_index sellorders;
    profit_index profits;
    income_index incomes;
    dealorder_index dealorders;
    depositlog_index depositlogs;
    withdrawlog_index withdrawlogs;
    
};
