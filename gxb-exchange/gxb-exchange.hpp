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
const uint64_t platform_core_asset_id = 11; // 平台核心资产id

class gxbexchange : public contract
{
  public:
    gxbexchange(uint64_t account_id)
        : contract(account_id)
        , accounts(_self, _self)
    {
    }

    // @abi action
    // @abi payable
    void deposit();

    // @abi action
    void withdraw(std::string to_account, contract_asset amount);

    //@abi action
    void pendingorder(uint8_t type, contract_asset quantity, uint64_t price);

    //@abi action
    void cancelorder(std::string to_account, contract_asset amount);

  private:

    // bool is_in_order(uint8_t type, uint64_t user);

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

    //@abi table buyorder i64
    struct buyorder {
        uint64_t id;
        uint64_t price;
        contract_asset quantity;
        uint64_t buyer;
        int64_t  order_time;
        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return price; }
        uint64_t get_asset() const { return quantity.asset_id; }
        uint64_t get_sender() const { return buyer; }
        GRAPHENE_SERIALIZE(buyorder, (id)(price)(quantity)(buyer)(order_time))
    };

    //@abi table sellorder i64
    struct sellorder {
        uint64_t id;
        uint64_t price;
        contract_asset quantity;
        uint64_t seller;
        int64_t  order_time;
        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return price; }
        uint64_t get_asset() const { return quantity.asset_id; }
        uint64_t get_sender() const { return seller; }
        GRAPHENE_SERIALIZE(sellorder, (id)(price)(quantity)(seller)(order_time))
    };

    

    typedef graphene::multi_index<N(account), account> account_index;

    typedef graphene::multi_index<N(buyorder), buyorder,
                        indexed_by<N(price), const_mem_fun<buyorder, uint64_t, &buyorder::get_price>>,
                        indexed_by<N(sender), const_mem_fun<buyorder, uint64_t, &buyorder::buyer>>,
                        indexed_by<N(asset), const_mem_fun<buyorder, uint64_t, &buyorder::get_asset>>
                        > buyorder_index;

    typedef graphene::multi_index<N(sellorder), sellorder,
                        indexed_by<N(price), const_mem_fun<sellorder, uint64_t, &sellorder::get_price>>,
                        indexed_by<N(sender), const_mem_fun<sellorder, uint64_t, &sellorder::seller>>,
                        indexed_by<N(asset), const_mem_fun<sellorder, uint64_t, &sellorder::get_asset>>
    > sellorder_index;

    account_index accounts;
    buyorder_index buyorders;
    sellorder_index sellorders;
    
};

GRAPHENE_ABI(gxbexchange, (deposit)(withdraw)(pendingorder)(cancelorder))