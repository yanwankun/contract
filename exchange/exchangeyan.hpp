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
const uint8_t sell_order_type = 2; //  卖单类型

const uint8_t order_status_trading = 1; //  委托中
const uint8_t order_status_end = 2; //  委托交易成功
const uint8_t order_status_cancel = 3; //  委托取消

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

const uint64_t asset_recision = 100000;
const uint64_t exchange_fee = 3; // 交易得手续费 （分子）
const uint64_t exchange_fee_base_amount = 1000; // 交易手续非得基数（分母）
class exchangeyan : public contract
{
  public:
    exchangeyan(uint64_t account_id)
        : contract(account_id)
        , accounts(_self, _self)
        , orders(_self, _self)
        , buyorders(_self, _self)
        , sellorders(_self, _self)
        , dealorders(_self, _self)
    {
    }

    // 限价挂单交易
    //@abi action
    // @abi payable
    void sellorder(int64_t price);

    //@abi action
    // @abi payable
    void buyorder(contract_asset quantity, int64_t price);

    //@abi action
    void cancelorder(uint8_t type, uint64_t id);

    //市场价格购买和售卖
    // @abi action
    // @abi payable
    void mpbuy(contract_asset quantity);

    // @abi action
    // @abi payable
    void mpsell();


    // 账户记录
    //@abi table account i64
    struct account {
        uint64_t owner;
        // 可用余额
        std::vector<contract_asset> balances;

        uint64_t primary_key() const { return owner; }
        GRAPHENE_SERIALIZE(account, (owner)(balances))
    };

    // 订单表
    // @abi table order i64
    struct order {
        uint64_t id;
        contract_asset pay_amount;
        contract_asset amount; // 金额
        uint64_t user;
        uint8_t order_type; // 买单和卖单
        int64_t price;
        int64_t deal_price;
        uint64_t deal_amount; // 成交数量
        uint64_t un_deal_amount; // 未成交数量
        uint8_t status; //订单状态
        uint64_t fee_amount;
        uint64_t refund_amount;
        int64_t  order_time;

        uint64_t primary_key() const { return id; }
        uint64_t get_user() const { return user; }
        GRAPHENE_SERIALIZE(order, (id)(pay_amount)(amount)(user)(order_type)(price)(deal_price)(deal_amount)(un_deal_amount)(status)(fee_amount)(refund_amount)(order_time))
    };

    // 买单列表 
    //@abi table buyorder i64
    struct buyorder {
        uint64_t id;
        int64_t price;
        contract_asset quantity;
        uint64_t order_id; // 订单编号
        uint64_t buyer;
        int64_t  order_time;

        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return price; }
        uint64_t get_asset() const { return quantity.asset_id; }
        uint64_t get_orderid() const { return order_id; }
        GRAPHENE_SERIALIZE(buyorder, (id)(price)(quantity)(order_id)(buyer)(order_time))
    };

    // 卖单列表
    //@abi table sellorder i64
    struct sellorder {
        uint64_t id;
        int64_t price;
        contract_asset quantity;
        uint64_t order_id; // 订单编号
        uint64_t seller;
        int64_t  order_time;

        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return price; }
        uint64_t get_asset() const { return quantity.asset_id; }
        uint64_t get_orderid() const { return order_id; }
        GRAPHENE_SERIALIZE(sellorder, (id)(price)(quantity)(order_id)(seller)(order_time))
    };

    // 成交订单列表
    // @abi table dealorder i64
    struct dealorder {
        uint64_t id;
        uint64_t sell_order_id; // 订单编号
        uint64_t buy_order_id; // 订单编号
        int64_t price;
        contract_asset quantity;
        uint64_t buyer;
        uint64_t seller;
        contract_asset fee;
        int64_t  order_time;

        uint64_t primary_key() const { return id; }
        GRAPHENE_SERIALIZE(dealorder, (id)(sell_order_id)(buy_order_id)(price)(quantity)(buyer)(seller)(fee)(order_time))
    };

    void update_sell_order(uint64_t id, contract_asset quantity);
    void update_buy_order(uint64_t id, contract_asset quantity);
    uint64_t insert_sell_order(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t seller);
    uint64_t insert_buy_order(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t buyer);
    uint64_t insert_dealorder(int64_t price, contract_asset quantity);
    uint64_t inser_dealorder(uint64_t sellorder_id, uint64_t buyorder_id, uint64_t price, contract_asset quantity, uint64_t buyer, uint64_t seller, contract_asset fee);
    uint64_t insert_order(contract_asset pay_amount, contract_asset amount, uint64_t user, uint8_t order_type, int64_t price, uint8_t status);
    void update_order_sell(uint64_t id, contract_asset quantity);
    void update_order_buy(uint64_t id, contract_asset quantity, contract_asset fee, int64_t price);
    void sell_order_fun(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t seller);
    void buy_order_fun(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t buyer);
    void remove_sell_order(uint64_t order_id); 
    void remove_buy_order(uint64_t order_id);
    void cancel_sell_order_fun(uint64_t id, uint64_t seller);
    void cancel_buy_order_fun(uint64_t id, uint64_t buyer);
  private:
    
    typedef graphene::multi_index<N(account), account> account_index;
    typedef graphene::multi_index<N(buyorder), buyorder,
                        indexed_by<N(price), const_mem_fun<buyorder, uint64_t, &buyorder::get_price>>,
                        indexed_by<N(orderid), const_mem_fun<buyorder, uint64_t, &buyorder::get_orderid>>,
                        indexed_by<N(asset), const_mem_fun<buyorder, uint64_t, &buyorder::get_asset>>
                        > buyorder_index;
    typedef graphene::multi_index<N(sellorder), sellorder,
                        indexed_by<N(price), const_mem_fun<sellorder, uint64_t, &sellorder::get_price>>,
                        indexed_by<N(orderid), const_mem_fun<sellorder, uint64_t, &sellorder::get_orderid>>,
                        indexed_by<N(asset), const_mem_fun<sellorder, uint64_t, &sellorder::get_asset>>
    > sellorder_index;
    typedef graphene::multi_index<N(order), order,
                        indexed_by<N(user), const_mem_fun<order, uint64_t, &order::get_user>>
    > order_index;
    typedef graphene::multi_index<N(dealorder), dealorder> dealorder_index;


    account_index accounts;
    order_index orders;
    buyorder_index buyorders;
    sellorder_index sellorders;
    dealorder_index dealorders;
};
