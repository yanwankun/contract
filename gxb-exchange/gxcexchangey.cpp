#include "gxc_exchange_help.cpp"

using namespace graphene;

void gxcexchangey::deposit()
{
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    contract_asset amount{asset_amount, asset_id};

    uint64_t owner = get_trx_sender();
    
    add_balances(owner, amount);
    insert_depositlog(owner, amount);
}

void gxcexchangey::withdraw(std::string to_account, contract_asset amount)
{
    int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
    graphene_assert(account_id >= 0, "invalid account_name to_account");
    graphene_assert(amount.amount > 0, "invalid amount");

    uint64_t owner = get_trx_sender();
    auto it = accounts.find(owner);
    graphene_assert(it != accounts.end(), "owner has no asset");

    sub_balances(owner, amount);
    withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
    insert_withdrawlog(owner, amount);
}

/**
 * 挂单说明 
 * type 挂单类型 1 买单 2 卖单
 * quantity 挂单金额
 * price 挂单单价  
 * 
 * 处理过程说明 
 * 其次 如果是卖单， 就从买单列表中查询相同资产得买单，找出所有价格合适得订单，从中匹配交易，如果有剩余则存入卖单列表，如没有则结束， 买单跟卖单处理过程类似
 * 
 * */
void gxcexchangey::pendingorder(uint8_t type, contract_asset quantity, int64_t price) 
{
    graphene_assert(quantity.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");
    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        sell_order_fun(quantity, price, sender); 
    } else if (buy_order_type == type) {
        buy_order_fun(quantity, price, sender);
    }

}

void gxcexchangey::cancelorder(uint8_t type, uint64_t id) {

    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        cancel_sell_order_fun(id, sender); 
    } else if (buy_order_type == type) {
        cancel_buy_order_fun(id, sender);
    }
}

GRAPHENE_ABI(gxcexchangey, (deposit)(withdraw)(pendingorder)(cancelorder))