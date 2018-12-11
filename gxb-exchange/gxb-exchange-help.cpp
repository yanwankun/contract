#include "gxb-exchange.hpp"

using namespace graphene;


// 把出价高和创建时间早得放在前面
bool buyoredercomp(const buyorder &a, const buyorder &b){
    if (a.price > b.price) {
        return true;
    } else if (a.price < b.price) {
        return false;
    } else if (a.order_time <= b.order_time) {
        return true;
    } else {
        return false;
    }
}

// 把售价低和创建时间早得放在前面
bool selloredercomp(const sellorder &a, const sellorder &b){
    if (a.price < b.price) {
        return true;
    } else if (a.price > b.price) {
        return false;
    } else if (a.order_time <= b.order_time) {
        return true;
    } else {
        return false;
    }
}

// 增加余额
void add_balances(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);
    if (it_user == accounts.end()) {
        accounts.emplace(0, [&](auto &o) {
            o.owner = owner;
            o.balances.emplace_back(quantity);
        });
    } else {
        uint16_t asset_index = std::distance(it_user->balances.begin(),
                                                find_if(it_user->balances.begin(), it_user->balances.end(), [&](const auto &a) { return a.asset_id == quantity.asset_id; }));
        if (asset_index < it->balances.size()) {
            accounts.modify(it_user, 0, [&](auto &o) { o.balances[asset_index] += quantity; });
        } else {
            accounts.modify(it_user, 0, [&](auto &o) { o.balances.emplace_back(quantity); });
        }
    }
}

// 减少余额
void sub_balances(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);
    graphene_assert(it_user != accounts.end(), "账户记录不存在");
    
    uint16_t asset_index = std::distance(it_user->balances.begin(),
                                                find_if(it_user->balances.begin(), it_user->balances.end(), [&](const auto &a) { return a.asset_id == quantity.asset_id; }));
    
    graphene_assert(asset_index < it_user->balances.size(), "账户没有相关资产，不能进行操作");
    graphene_assert(it_user->balances[asset_index].amount >= quantity.amount, "账户相关资产余额不足");

    accounts.modify(it_user, 0, [&](auto &o) { o.balances[asset_index] -= quantity; });
    
}

// 增加余额
void add_balances_lock(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);
    
    uint16_t asset_index = std::distance(it_user->lock_balances.begin(),
                                            find_if(it_user->lock_balances.begin(), it_user->lock_balances.end(), [&](const auto &a) { return a.asset_id == quantity.asset_id; }));
    if (asset_index < it->balances.size()) {
        accounts.modify(it_user, 0, [&](auto &o) { o.lock_balances[asset_index] += quantity; });
    } else {
        accounts.modify(it_user, 0, [&](auto &o) { o.lock_balances.emplace_back(quantity); });
    }
    
}

// 减少余额
void sub_balances_lock(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);
    graphene_assert(it_user != accounts.end(), "账户记录不存在");
    
    uint16_t asset_index = std::distance(it_user->lock_balances.begin(),
                                                find_if(it_user->lock_balances.begin(), it_user->lock_balances.end(), [&](const auto &a) { return a.asset_id == quantity.asset_id; }));
    
    graphene_assert(asset_index < it_user->lock_balances.size(), "账户没有相关资产，不能进行操作");
    graphene_assert(it_user->lock_balances[asset_index].amount >= quantity.amount, "账户相关资产余额不足");

    accounts.modify(it_user, 0, [&](auto &o) { o.lock_balances[asset_index] -= quantity; });
    
}

/**
 * 内部转账
 */
 void exchange_transfer(uint64_t from, uint64_t to, contract_asset quantity) {
    auto it_from = accounts.find(from);
    graphene_assert(it_from != accounts.end(), "转账发送者账户记录不存在");

    // 发送者扣除相应金额
    int asset_index = 0;
    for (auto asset_it = it_from->lock_balances.begin(); asset_it != it_from->lock_balances.end(); ++asset_it) {
        if ((quantity.asset_id) == asset_it->asset_id) {
            graphene_assert(asset_it->amount >= quantity.amount, "锁定账户余额资产不足");
            if (asset_it->amount == quantity.amount) {
                accounts.modify(it_from, 0, [&](auto &o) {
                    o.lock_balances.erase(asset_it);
                });
            } else {
                accounts.modify(it_from, 0, [&](auto &o) {
                    o.balances[asset_index] -= quantity;
                });
            }

            break;
        }
        asset_index++;
    }

    // 接受者增加相应金额
    auto it_to = accounts.find(to);
    if (it_to == accounts.end()) {
        accounts.emplace(0, [&](auto &o) {
            o.owner = owner;
            o.balances.emplace_back(quantity);
        });
    } else {
        uint16_t asset_index = std::distance(it_to->balances.begin(),
                                                find_if(it_to->balances.begin(), it_to->balances.end(), [&](const auto &a) { return a.asset_id == quantity.asset_id; }));
        if (asset_index < it_to->balances.size()) {
            accounts.modify(it_to, 0, [&](auto &o) { o.balances[asset_index] += quantity; });
        } else {
            accounts.modify(it_to, 0, [&](auto &o) { o.balances.emplace_back(quantity); });
        }
    }
 }

/**
 * 内部转转实现(从lock账户)
 */ 
void transfer_from_lock(uint64_t from, uint64_t to, contract_asset quantity) {
    auto it_from = accounts.find(from);
    graphene_assert(it_from != accounts.end(), "转账发送者账户记录不存在");

    // 发送者扣除相应金额
    int asset_index = 0;
    for (auto asset_it = it_from->lock_balances.begin(); asset_it != it_from->lock_balances.end(); ++asset_it) {
        if ((quantity.asset_id) == asset_it->asset_id) {
            graphene_assert(asset_it->amount >= quantity.amount, "锁定账户余额资产不足");
            if (asset_it->amount == quantity.amount) {
                accounts.modify(it_from, 0, [&](auto &o) {
                    o.lock_balances.erase(asset_it);
                });
            } else {
                accounts.modify(it_from, 0, [&](auto &o) {
                    o.balances[asset_index] -= quantity;
                });
            }

            break;
        }
        asset_index++;
    }

    // 接受者增加相应金额
    auto it_to = accounts.find(to);
    if (it_to == accounts.end()) {
        accounts.emplace(0, [&](auto &o) {
            o.owner = owner;
            o.balances.emplace_back(quantity);
        });
    } else {
        uint16_t asset_index = std::distance(it_to->balances.begin(),
                                                find_if(it_to->balances.begin(), it_to->balances.end(), [&](const auto &a) { return a.asset_id == quantity.asset_id; }));
        if (asset_index < it_to->balances.size()) {
            accounts.modify(it_to, 0, [&](auto &o) { o.balances[asset_index] += quantity; });
        } else {
            accounts.modify(it_to, 0, [&](auto &o) { o.balances.emplace_back(quantity); });
        }
    }
}

// 转账到lock账户
void transfer_to_lock(uint64_t user, contract_asset quantity) {
    auto it_user = accounts.find(user);
    graphene_assert(it_user != accounts.end(), "锁定资产发送者账户记录不存在");

    int asset_index = 0;
    for (auto asset_it = it_user->balances.begin(); asset_it != it_user->balances.end(); ++asset_it) {
        if ((quantity.asset_id) == asset_it->asset_id) {
            graphene_assert(asset_it->amount >= quantity.amount, "账户余额资产不足");
            if (asset_it->amount == quantity.amount) {
                accounts.modify(it_user, 0, [&](auto &o) {
                    o.balances.erase(asset_it);
                });
            } else {
                accounts.modify(it_user, 0, [&](auto &o) {
                    o.balances[asset_index] -= quantity;
                });
            }

            break;
        }
        asset_index++;
    }

    // 接受者增加相应金额
    uint16_t asset_index = std::distance(it_user->lock_balances.begin(),
                                            find_if(it_user->lock_balances.begin(), it_user->lock_balances.end(), [&](const auto &a) { return a.asset_id == quantity.asset_id; }));
    if (asset_index < it_user->lock_balances.size()) {
        accounts.modify(it_user, 0, [&](auto &o) { o.lock_balances[asset_index] += quantity; });
    } else {
        accounts.modify(it_user, 0, [&](auto &o) { o.lock_balances.emplace_back(quantity); });
    }
}

void update_sell_order(uint64_t id, contract_asset quantity) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = sellorders.find(id);
    graphene_assert(it != sellorders.end(), "卖单信息不存在");

    if (quantity.amount > 0) {
        sellorders.modify(it, 0, [&](auto &o) {
            o.quantity = quantity;
        });
    } else {
        sellorders.delete(it); 
    }
}

void update_buy_order(uint64_t id, contract_asset quantity) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = buyorders.find(id);
    graphene_assert(it != buyorders.end(), "买单信息不存在");

    if (quantity.amount > 0) {
        buyorders.modify(it, 0, [&](auto &o) {
            o.quantity = quantity;
        });
    } else {
        buyorders.delete(it); 
    }
}

void add_sell_order(uint64_t seller, contract_asset quantity, uint64_t price) {
    uint64_t pk = sellorders.available_primary_key();
    print("sell order pk = ", pk);
    sellorders.emplace(0, [&](auto &a_sell_order) {
        a_sell_order.id = pk;
        a_sell_order.price = price;
        a_sell_order.quantity = quantity;
        a_sell_order.seller = seller;
        a_sell_order.order_time = get_head_block_time;
    });
}

void add_buyer_order(uint64_t buyer, contract_asset quantity, uint64_t price) {
    uint64_t pk = buyorders.available_primary_key();
    print("buyer order pk = ", pk);
    buyorders.emplace(0, [&](auto &a_sell_order) {
        a_sell_order.id = pk;
        a_sell_order.price = price;
        a_sell_order.quantity = quantity;
        a_sell_order.buyer = buyer;
        a_sell_order.order_time = get_head_block_time;
    });
}

// 当收到卖单请求时得处理方法
void sell_order_fun(contract_asset quantity, uint64_t price, uint64_t seller) {
    auto idx = buyorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<buyorder> match_buy_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (itr->price >= price) {
            match_buy_orders.emplace_back(*itr);
        }
    }

    if (match_buy_orders.size() == 0) {
        add_sell_order(seller, quantity, price);
        transfer_to_lock(seller, quantity);
        return;
    }

    sort(match_buy_orders.begin(), match_buy_orders.end(), buyoredercomp);

    for (buyorder _buyorder : match_buy_orders) { 
        if (quantity.amount > 0) {
            if (_buyorder.quantity.amount <= quantity.amount) {
                transfer_from_lock(_buyorder.buyer, seller, _buyorder.quantity);
                // 把卖家的钱发送给买家
                update_buy_order(_buyorder.id, _buyorder.quantity - _buyorder.quantity);
                quantity -= _buyorder.quantity;
            } else {
                transfer_from_lock(seller, _buyorder.buyer, quantity);
                update_buy_order(_buyorder.id, _buyorder.quantity - quantity);
                quantity -= quantity;
            }
        } else {
            break;
        }
    }

    if (quantity.amount > 0) {
        add_sell_order(seller, quantity, price);
        transfer_to_lock(seller, quantity);
    }
}
