#include "gxb-exchange-help.cpp"

using namespace graphene;

void gxbexchange::deposit()
{
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    contract_asset amount{asset_amount, asset_id};

    uint64_t owner = get_trx_sender();
    auto it = accounts.find(owner);
    if (it == accounts.end()) {
        accounts.emplace(0, [&](auto &o) {
            o.owner = owner;
            o.balances.emplace_back(amount);
        });
    } else {
        uint16_t asset_index = std::distance(it->balances.begin(),
                                                find_if(it->balances.begin(), it->balances.end(), [&](const auto &a) { return a.asset_id == asset_id; }));
        if (asset_index < it->balances.size()) {
            accounts.modify(it, 0, [&](auto &o) { o.balances[asset_index] += amount; });
        } else {
            accounts.modify(it, 0, [&](auto &o) { o.balances.emplace_back(amount); });
        }
    }
}

void gxbexchange::withdraw(std::string to_account, contract_asset amount)
{
    int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
    graphene_assert(account_id >= 0, "invalid account_name to_account");
    graphene_assert(amount.amount > 0, "invalid amount");

    uint64_t owner = get_trx_sender();
    auto it = accounts.find(owner);
    graphene_assert(it != accounts.end(), "owner has no asset");

    int asset_index = 0;
    for (auto asset_it = it->balances.begin(); asset_it != it->balances.end(); ++asset_it) {
        if ((amount.asset_id) == asset_it->asset_id) {
            graphene_assert(asset_it->amount >= amount.amount, "balance not enough");
            if (asset_it->amount == amount.amount) {
                accounts.modify(it, 0, [&](auto &o) {
                    o.balances.erase(asset_it);
                });
                if (it->balances.size() == 0) {
                    accounts.erase(it);
                }
            } else {
                accounts.modify(it, 0, [&](auto &o) {
                    o.balances[asset_index] -= amount;
                });
            }

            break;
        }
        asset_index++;
    }

    withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
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
void gxbexchange::pendingorder(uint8_t type, contract_asset quantity, uint64_t price) 
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

void gxbexchange::cancelorder(std::string to_account, contract_asset amount);

GRAPHENE_ABI(gxbexchange, (deposit)(withdraw)(pendingorder)(cancelorder))