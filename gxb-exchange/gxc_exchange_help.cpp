#include "gxcexchangey.hpp"

using namespace graphene;


// 把出价高和创建时间早得放在前面
bool buyoredercomp(const gxcexchangey::buyorder &a, const gxcexchangey::buyorder &b){
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
bool selloredercomp(const gxcexchangey::sellorder &a, const gxcexchangey::sellorder &b){
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
void gxcexchangey::add_balances(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);
    if (it_user == accounts.end()) {
        accounts.emplace(0, [&](auto &o) {
            o.owner = user;
            o.balances.emplace_back(quantity);
        });
    } else {
        int asset_index = 0;
        bool add = false;
        for (auto asset_it = it_user->balances.begin(); asset_it != it_user->balances.end(); ++asset_it) {
            if ((quantity.asset_id) == asset_it->asset_id) {
                accounts.modify(it_user, 0, [&](auto &o) {
                    o.balances[asset_index] += quantity;
                });
                add = true;
                break;
            }
            asset_index++;
        }
        if (!add) {
            accounts.modify(it_user, 0, [&](auto &o) {
                o.balances.emplace_back(quantity);
            });
        }
    }
}

// 减少余额
void gxcexchangey::sub_balances(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);
    graphene_assert(it_user != accounts.end(), "账户记录不存在");

    int asset_index = 0;
    for (auto asset_it = it_user->balances.begin(); asset_it != it_user->balances.end(); ++asset_it) {
        if ((quantity.asset_id) == asset_it->asset_id) {
            graphene_assert(asset_it->amount >= quantity.amount, "账户相关资产余额不足");
            accounts.modify(it_user, 0, [&](auto &o) {
                o.balances[asset_index] -= quantity;
            });
            break;
        }
        asset_index++;
    }
    
}

// 增加余额
void gxcexchangey::add_balances_lock(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);

    int asset_index = 0;
    bool add = false;
    for (auto asset_it = it_user->lock_balances.begin(); asset_it != it_user->lock_balances.end(); ++asset_it) {
        if ((quantity.asset_id) == asset_it->asset_id) {
            accounts.modify(it_user, 0, [&](auto &o) {
                o.lock_balances[asset_index] += quantity;
            });
            add = true;
            break;
        }
        asset_index++;
    }
    if (!add) {
        accounts.modify(it_user, 0, [&](auto &o) {
            o.lock_balances.emplace_back(quantity);
        });
    }
    
}

// 减少余额
void gxcexchangey::sub_balances_lock(uint64_t user, contract_asset quantity){
    graphene_assert(quantity.amount > 0, "资金操作不能为负数");
    auto it_user = accounts.find(user);
    graphene_assert(it_user != accounts.end(), "账户记录不存在");

    int asset_index = 0;
    for (auto asset_it = it_user->lock_balances.begin(); asset_it != it_user->lock_balances.end(); ++asset_it) {
        if ((quantity.asset_id) == asset_it->asset_id) {
            graphene_assert(asset_it->amount >= quantity.amount, "账户相关资产余额不足");
            accounts.modify(it_user, 0, [&](auto &o) {
                o.lock_balances[asset_index] -= quantity;
            });

            break;
        }
        asset_index++;
    }
    
}

/**
 * 内部转账
 */
 void gxcexchangey::exchange_transfer(uint64_t from, uint64_t to, contract_asset quantity) {
    auto it_from = accounts.find(from);
    graphene_assert(it_from != accounts.end(), "转账发送者账户记录不存在");

    // 发送者扣除相应金额
    sub_balances(from, quantity);
    // 接受者增加相应金额
    add_balances(to, quantity);

 }

/**
 * 内部lock转转实现(从lock账户到balance) 
 */ 
void gxcexchangey::transfer_from_lock(uint64_t from, uint64_t to, contract_asset quantity) {
    auto it_from = accounts.find(from);
    graphene_assert(it_from != accounts.end(), "转账发送者账户记录不存在");

    // 发送者扣除相应金额
    sub_balances_lock(from, quantity);

    // 接受者增加相应金额
    add_balances(to, quantity);

}

// balance转账到lock账户
void gxcexchangey::balance_lock(uint64_t user, contract_asset quantity) {
    auto it_user = accounts.find(user);
    graphene_assert(it_user != accounts.end(), "锁定资产发送者账户记录不存在");

    sub_balances(user, quantity);
    add_balances_lock(user, quantity);

}

void gxcexchangey::unlock_lock_balance(uint64_t user, contract_asset quantity) {
    auto it_user = accounts.find(user);
    graphene_assert(it_user != accounts.end(), "锁定资产发送者账户记录不存在");

    add_balances(user, quantity);
    sub_balances_lock(user, quantity);
}


void gxcexchangey::add_income(contract_asset profit) {
    graphene_assert(profit.amount > 0, "资金操作不能为负数");
    auto income_asset = incomes.find(profit.asset_id);
    if (income_asset == incomes.end()) {
        incomes.emplace(0, [&](auto &o) {
            o.asset_id = profit.asset_id;
            o.amount = profit.amount;
        });
    } else {
        incomes.modify(income_asset, 0, [&](auto &o) {
            // todo 判断溢出问题
            o.amount += profit.amount;
        });
    }
}

void gxcexchangey::sub_income(contract_asset profit) {
    graphene_assert(profit.amount > 0, "资金操作不能为负数");
    auto income_asset = incomes.find(profit.asset_id);
    graphene_assert(income_asset != incomes.end(), "当前收益没有对应得资产");
    graphene_assert(income_asset->amount >= profit.amount, "当前收益不足");
    
    incomes.modify(income_asset, 0, [&](auto &o) {
        o.amount -= profit.amount;
    });
    
} 

void gxcexchangey::update_sell_order(uint64_t id, contract_asset quantity) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = sellorders.find(id);
    graphene_assert(it != sellorders.end(), "卖单信息不存在");

    if (quantity.amount > 0) {
        sellorders.modify(it, 0, [&](auto &o) {
            o.quantity = quantity;
        });
    } else {
        sellorders.erase(it); 
    }
}

void gxcexchangey::update_buy_order(uint64_t id, contract_asset quantity) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = buyorders.find(id);
    graphene_assert(it != buyorders.end(), "买单信息不存在");

    if (quantity.amount > 0) {
        buyorders.modify(it, 0, [&](auto &o) {
            o.quantity = quantity;
        });
    } else {
        buyorders.erase(it); 
    }
}

void gxcexchangey::insert_sell_order(uint64_t seller, contract_asset quantity, int64_t price) {
    uint64_t pk = sellorders.available_primary_key();
    print("sell order pk = ", pk);
    sellorders.emplace(0, [&](auto &a_sell_order) {
        a_sell_order.id = pk;
        a_sell_order.price = price;
        a_sell_order.quantity = quantity;
        a_sell_order.seller = seller;
        a_sell_order.order_time = get_head_block_time();
    });
}

void gxcexchangey::insert_buy_order(uint64_t buyer, contract_asset quantity, int64_t price) {
    uint64_t pk = buyorders.available_primary_key();
    print("buyer order pk = ", pk);
    buyorders.emplace(0, [&](auto &a_buy_order) {
        a_buy_order.id = pk;
        a_buy_order.price = price;
        a_buy_order.quantity = quantity;
        a_buy_order.buyer = buyer;
        a_buy_order.order_time = get_head_block_time();
    });
}

void gxcexchangey::insert_profit(contract_asset profit) {
    uint64_t pk = profits.available_primary_key();
    print("profits pk = ", pk);
    profits.emplace(0, [&](auto &a_profit) {
        a_profit.id = pk;
        a_profit.profit_asset = profit;
        a_profit.profit_time = get_head_block_time();
    });
}

void gxcexchangey::insert_dealorder(int64_t price, contract_asset quantity) {
    uint64_t pk = dealorders.available_primary_key();
    print("dealorders pk = ", pk);
    dealorders.emplace(0, [&](auto &a_dealorder) {
        a_dealorder.id = pk;
        a_dealorder.price = price;
        a_dealorder.quantity = quantity;
        a_dealorder.order_time = get_head_block_time();
    });
}

void gxcexchangey::insert_depositlog(uint64_t user, contract_asset amount) {
    uint64_t pk = depositlogs.available_primary_key();
    print("depositlogs pk = ", pk);
    depositlogs.emplace(0, [&](auto &a_depositlog) {
        a_depositlog.id = pk;
        a_depositlog.user = user;
        a_depositlog.quantity = amount;
        a_depositlog.order_time = get_head_block_time();
    });
}
void gxcexchangey::insert_withdrawlog(uint64_t user, contract_asset amount) {
    uint64_t pk = withdrawlogs.available_primary_key();
    print("withdrawlogs pk = ", pk);
    withdrawlogs.emplace(0, [&](auto &a_withdrawlog) {
        a_withdrawlog.id = pk;
        a_withdrawlog.user = user;
        a_withdrawlog.quantity = amount;
        a_withdrawlog.order_time = get_head_block_time();
    });
}


// 当收到卖单请求时得处理方法
void gxcexchangey::sell_order_fun(contract_asset quantity, int64_t price, uint64_t seller) {
    auto idx = buyorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<buyorder> match_buy_orders;
    uint64_t match_amount = 0; // 已经匹配得金额数目
    uint64_t count = 0; // 已经匹配得交易数
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (itr->price >= price) {
            match_buy_orders.emplace_back(*itr);

            match_amount += itr->quantity.amount;
            count += 1;
            if (count > max_match_order_count || match_amount > quantity.amount * match_amount_times) {
                break;
            }
        }
    }

    if (match_buy_orders.size() == 0) {
        insert_sell_order(seller, quantity, price);
        balance_lock(seller, quantity);
        return;
    }

    sort(match_buy_orders.begin(), match_buy_orders.end(), buyoredercomp);

    for (buyorder _buyorder : match_buy_orders) { 
        if (quantity.amount > 0) {
            if (_buyorder.quantity.amount <= quantity.amount) {
                contract_asset trade_plateform_asset{ _buyorder.quantity.amount * price, platform_core_asset_id};
                transfer_from_lock(_buyorder.buyer, seller, trade_plateform_asset);
                exchange_transfer(seller, _buyorder.buyer, _buyorder.quantity);

                if (_buyorder.price > price) {
                    contract_asset profit{ _buyorder.quantity.amount * (_buyorder.price - price), platform_core_asset_id};
                    insert_profit(profit);
                    add_income(profit);
                }

                update_buy_order(_buyorder.id, _buyorder.quantity - _buyorder.quantity);
                insert_dealorder(_buyorder.price, _buyorder.quantity);
                quantity -= _buyorder.quantity;
            } else {
                contract_asset trade_plateform_asset{ quantity.amount * price, platform_core_asset_id};
                transfer_from_lock(_buyorder.buyer, seller, trade_plateform_asset);
                exchange_transfer(seller, _buyorder.buyer, quantity);

                if (_buyorder.price > price) {
                    contract_asset profit{ quantity.amount * (_buyorder.price - price), platform_core_asset_id};
                    insert_profit(profit);
                    add_income(profit);
                }

                update_buy_order(_buyorder.id, _buyorder.quantity - quantity);
                insert_dealorder(_buyorder.price, quantity);
                quantity -= quantity;
            }
        } else {
            break;
        }
    }

    if (quantity.amount > 0) {
        insert_sell_order(seller, quantity, price);
        balance_lock(seller, quantity);
    }
}

// 收到买单得处理
void gxcexchangey::buy_order_fun(contract_asset quantity, int64_t price, uint64_t buyer) {
    auto idx = sellorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    uint64_t match_amount = 0; // 已经匹配得金额数目
    uint64_t count = 0; // 已经匹配得交易数
    vector<sellorder> match_sell_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (itr->price <= price) {
            match_sell_orders.emplace_back(*itr);

            match_amount += itr->quantity.amount;
            count += 1;
            if (count > max_match_order_count || match_amount > quantity.amount * match_amount_times) {
                break;
            }
        }
    }

    if (match_sell_orders.size() == 0) {
        insert_buy_order(buyer, quantity, price);
        contract_asset lock_asset{ quantity.amount * price, platform_core_asset_id};
        balance_lock(buyer, lock_asset);
        return;
    }

    sort(match_sell_orders.begin(), match_sell_orders.end(), selloredercomp);

    for (sellorder _sellorder : match_sell_orders) { 
        if (quantity.amount > 0) {
            if (_sellorder.quantity.amount <= quantity.amount) {
                contract_asset trade_plateform_asset{ _sellorder.quantity.amount * _sellorder.price, platform_core_asset_id};
                exchange_transfer(buyer, _sellorder.seller, trade_plateform_asset);
                transfer_from_lock(_sellorder.seller, buyer, _sellorder.quantity);

                if (_sellorder.price < price) {
                    contract_asset profit{ _sellorder.quantity.amount * (price - _sellorder.price), platform_core_asset_id};
                    insert_profit(profit);
                    add_income(profit);
                }

                update_sell_order(_sellorder.id, _sellorder.quantity - _sellorder.quantity);
                insert_dealorder(price, _sellorder.quantity);
                quantity -= _sellorder.quantity;
            } else {
                contract_asset trade_plateform_asset{ quantity.amount * _sellorder.price, platform_core_asset_id};
                exchange_transfer(buyer, _sellorder.seller, trade_plateform_asset);
                transfer_from_lock(_sellorder.seller, buyer, quantity);

                if (_sellorder.price < price) {
                    contract_asset profit{ quantity.amount * (price - _sellorder.price), platform_core_asset_id};
                    insert_profit(profit);
                    add_income(profit);
                }

                update_sell_order(_sellorder.id, _sellorder.quantity - quantity);
                insert_dealorder(price, quantity);
                quantity -= quantity;
            }
        } else {
            break;
        }
    }

    if (quantity.amount > 0) {
        insert_buy_order(buyer, quantity, price);
        balance_lock(buyer, quantity);
    }
}

void gxcexchangey::cancel_sell_order_fun(uint64_t id, uint64_t seller) {
    auto it = sellorders.find(id);
    graphene_assert(it != sellorders.end(), "卖单信息不存在");
    graphene_assert(it->seller == seller, "无权操作");

    unlock_lock_balance(seller, it->quantity);
    sellorders.erase(it);
}

void gxcexchangey::cancel_buy_order_fun(uint64_t id, uint64_t buyer) {
    auto it = buyorders.find(id);
    graphene_assert(it != buyorders.end(), "买单信息不存在");
    graphene_assert(it->buyer == buyer, "无权操作");

    contract_asset buy_asset{ it->quantity.amount * it->price, platform_core_asset_id};
    unlock_lock_balance(buyer, buy_asset);
    buyorders.erase(it);
}