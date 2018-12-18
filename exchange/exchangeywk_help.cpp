#include "exchangeywk.hpp"

using namespace graphene;


// 把出价高和创建时间早得放在前面
bool buyoredercomp(const exchangeywk::buyorder &a, const exchangeywk::buyorder &b){
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
bool selloredercomp(const exchangeywk::sellorder &a, const exchangeywk::sellorder &b){
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
void exchangeywk::add_balances(uint64_t user, contract_asset quantity){
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
void exchangeywk::sub_balances(uint64_t user, contract_asset quantity){
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

void exchangeywk::update_sell_order(uint64_t id, contract_asset quantity) {

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

void exchangeywk::update_buy_order(uint64_t id, contract_asset quantity) {

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

uint64_t exchangeywk::insert_sell_order(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t seller) {
    uint64_t pk = sellorders.available_primary_key();
    print("sell order pk = ", pk);
    sellorders.emplace(0, [&](auto &a_sell_order) {
        a_sell_order.id = pk;
        a_sell_order.price = price;
        a_sell_order.quantity = quantity;
        a_sell_order.order_id = order_id;
        a_sell_order.seller = seller;
        a_sell_order.order_time = get_head_block_time();
    });
    return pk;
}

uint64_t exchangeywk::insert_buy_order(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t buyer) {
    uint64_t pk = buyorders.available_primary_key();
    print("buyer order pk = ", pk);
    buyorders.emplace(0, [&](auto &a_buy_order) {
        a_buy_order.id = pk;
        a_buy_order.price = price;
        a_buy_order.quantity = quantity;
        a_buy_order.order_id = order_id;
        a_buy_order.buyer = buyer;
        a_buy_order.order_time = get_head_block_time();
    });
    return pk;
}

uint64_t exchangeywk::insert_dealorder(uint64_t sellorder_id, uint64_t buyorder_id, uint64_t price, contract_asset quantity, uint64_t buyer, uint64_t seller, contract_asset fee) {
    uint64_t pk = dealorders.available_primary_key();
    print("dealorders pk = ", pk);
    dealorders.emplace(0, [&](auto &a_deal_order) {
        a_deal_order.id = pk;
        a_deal_order.sell_order_id = sellorder_id;
        a_deal_order.buy_order_id = buyorder_id;
        a_deal_order.price = price;
        a_deal_order.quantity = quantity;
        a_deal_order.buyer = buyer;
        a_deal_order.seller = seller;
        a_deal_order.fee = fee;
        a_deal_order.order_time = get_head_block_time();
    });

    uint64_t once_delete_count = get_sysconfig(once_delete_count_ID);
    if (pk > get_sysconfig(table_save_count_ID) && pk % once_delete_count == 0) {
        delete_dealorder(once_delete_count);
    }

    return pk;
}

uint64_t exchangeywk::insert_order(contract_asset pay_amount, contract_asset amount, uint64_t user, uint8_t order_type, int64_t price, uint8_t status) {
    uint64_t pk = orders.available_primary_key();
    print("orders pk = ", pk);
    orders.emplace(0, [&](auto &a_order) {
        a_order.id = pk;
        a_order.pay_amount = pay_amount;
        a_order.amount = amount;
        a_order.user = user;
        a_order.order_type = order_type; // 买单和卖单
        a_order.price = price;
        a_order.deal_price = 0;
        if (order_type == sell_order_type) {
            a_order.deal_price = price;
        }
        a_order.deal_amount = 0; // 成交数量
        a_order.un_deal_amount = amount.amount; // 未成交数量
        a_order.status = status; //订单状态
        a_order.fee_amount = 0;
        a_order.order_time = get_head_block_time();
    });

    uint64_t once_delete_count = get_sysconfig(once_delete_count_ID);
    if (pk > get_sysconfig(table_save_count_ID) && pk % once_delete_count == 0) {
        delete_order(once_delete_count);
    }

    return pk;
}


void exchangeywk::update_order_sell(uint64_t id, contract_asset quantity, int64_t price) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "订单信息不存在");

    int64_t deal_price = it->price == price ? price : (it->deal_amount * it->price + quantity.amount * price) / (it->deal_amount + quantity.amount);
    orders.modify(it, 0, [&](auto &m_order) {
        m_order.amount -= quantity;
        m_order.deal_price = deal_price;
        m_order.deal_amount += quantity.amount;
        m_order.un_deal_amount -= quantity.amount;
        if (m_order.un_deal_amount == 0) {
            m_order.status = order_status_end;
        }
    });
}

void exchangeywk::update_order_buy(uint64_t id, contract_asset quantity, contract_asset fee, int64_t price) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "订单信息不存在");

    int64_t deal_price = it->price == price ? price : (it->deal_amount * it->deal_price + quantity.amount * price ) / (it->deal_amount + quantity.amount);
    int64_t undealamount = it->un_deal_amount - quantity.amount;
    int64_t refund_amount;
    if (undealamount == 0) {
        // 有退款 = 付款得钱 - 购买资产得钱 - 手续费
        refund_amount = it->pay_amount.amount - (it->deal_amount + quantity.amount) * deal_price - it->fee_amount - fee.amount;
        if (refund_amount > 0) {
            contract_asset refundasset{refund_amount, platform_core_asset_id_VALUE};
            withdraw_asset(_self, it->user, platform_core_asset_id_VALUE, refund_amount);
            sub_balances(it->user, refundasset);
        }
    }

    orders.modify(it, 0, [&](auto &m_order) {
        m_order.amount -= quantity;
        m_order.deal_price = deal_price;
        m_order.deal_amount += quantity.amount;
        m_order.un_deal_amount -= quantity.amount;
        m_order.fee_amount += fee.amount;
        if (m_order.un_deal_amount == 0) {
            m_order.status = order_status_end;
            contract_asset refundasset{refund_amount, platform_core_asset_id_VALUE};
            m_order.refund_amount = refundasset;
        }
    });
}

void exchangeywk::end_order(uint64_t id) {
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "订单信息不存在");
    graphene_assert(it->status == order_status_trading , "订单已结束");

    if (it->order_type == buy_order_type) {
        int64_t refund_amount = it->pay_amount.amount - it->deal_amount * it->deal_price - it->fee_amount;
        if (refund_amount > 0) {
            contract_asset refundasset{refund_amount, platform_core_asset_id_VALUE};
            withdraw_asset(_self, it->user, platform_core_asset_id_VALUE, refund_amount);
            sub_balances(it->user, refundasset);
            orders.modify(it, 0, [&](auto &m_order) {
                m_order.status = order_status_end;
                m_order.refund_amount = refundasset;
            });
        }
    } else {
        graphene_assert(it->un_deal_amount > 0 , "订单金额异常");
        withdraw_asset(_self, it->user, it->amount.asset_id, it->amount.amount);
        sub_balances(it->user, it->amount);
        orders.modify(it, 0, [&](auto &m_order) {
            m_order.status = order_status_end;
            m_order.refund_amount = m_order.amount;
        });
    }
}

// 当收到卖单请求时得处理方法
void exchangeywk::sell_order_fun(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t seller) {

    auto idx = buyorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<buyorder> match_buy_orders;
    uint64_t match_amount = 0; // 已经匹配得金额数目
    uint64_t count = 0; // 已经匹配得交易数
    uint64_t max_match_count = get_sysconfig(max_match_order_count_ID); // 最大匹配交易条数
    uint64_t max_match_amount = quantity.amount * get_sysconfig(match_amount_times_ID); // 最大匹配交易金额
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (itr->price >= price) {
            match_buy_orders.emplace_back(*itr);
            match_amount += itr->quantity.amount;
            count += 1;
            if (count >= max_match_count || match_amount >= max_match_amount) {
                break;
            }
        }
    }

    if (match_buy_orders.size() == 0) {
        insert_sell_order(order_id, quantity, price, seller);
        return;
    }

    sort(match_buy_orders.begin(), match_buy_orders.end(), buyoredercomp);

    for (buyorder _buyorder : match_buy_orders) { 
        if (quantity.amount > 0) {
            // 如果有匹配 添加成交记录 添加手续费 把卖家的资产转给买家 把买家得核心资产转给卖家
            contract_asset trade_quantity{0, quantity.asset_id};
            if (_buyorder.quantity.amount <= quantity.amount) {
                trade_quantity.amount = _buyorder.quantity.amount;
            } else {
                trade_quantity.amount = quantity.amount;
            }
            int64_t deal_amount = trade_quantity.amount * price / asset_recision;
            int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

            contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
            contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
            contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE};

            insert_dealorder(order_id, _buyorder.order_id, price, deal_asset, _buyorder.buyer, seller, trade_fee);
            // 增加收入记录和更新总收入
            insert_profit(fee_amount * 2);
            contract_asset profit_fee{ fee_amount * 2, platform_core_asset_id_VALUE};
            add_income(profit_fee);
            update_order_sell(order_id, trade_quantity, _buyorder.price);
            update_order_buy(_buyorder.order_id, trade_quantity, trade_fee, price);
            update_buy_order(_buyorder.id, _buyorder.quantity - trade_quantity);

            // 卖方收入
            withdraw_asset(_self, seller, platform_core_asset_id_VALUE, deal_amount - fee_amount); // 卖方手续费在这里扣除
            // 买方收入
            withdraw_asset(_self, _buyorder.buyer, _buyorder.quantity.asset_id, trade_quantity.amount);
            sub_balances(_buyorder.buyer, amount);
            sub_balances(seller, trade_quantity);
            
            quantity -= trade_quantity;
        } else {
            break;
        }
    }

    if (quantity.amount > 0) {
        insert_sell_order(order_id, quantity, price, seller);
    }
}

// 收到买单得处理
void exchangeywk::buy_order_fun(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t buyer) {
    auto idx = sellorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<sellorder> match_sell_orders;
    uint64_t match_amount = 0; // 已经匹配得金额数目
    uint64_t count = 0; // 已经匹配得交易数
    uint64_t max_match_count = get_sysconfig(max_match_order_count_ID); // 最大匹配交易条数
    uint64_t max_match_amount = quantity.amount * get_sysconfig(match_amount_times_ID); // 最大匹配交易金额
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (itr->price <= price) {
            match_sell_orders.emplace_back(*itr);
            match_amount += itr->quantity.amount;
            count += 1;
            if (count >= max_match_count || match_amount >= max_match_amount) {
                break;
            }
        }
    }

    if (match_sell_orders.size() == 0) {
        insert_buy_order(order_id, quantity, price, buyer);
        return;
    }

    sort(match_sell_orders.begin(), match_sell_orders.end(), selloredercomp);

    for (sellorder _sellorder : match_sell_orders) { 
        if (quantity.amount > 0) {
            contract_asset trade_quantity{0, quantity.asset_id};
            if (_sellorder.quantity.amount <= quantity.amount) {
                trade_quantity.amount = _sellorder.quantity.amount;
            } else {
                trade_quantity.amount = quantity.amount;
            }

            int64_t deal_amount = trade_quantity.amount * _sellorder.price / asset_recision;
            int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

            contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
            contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
            contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE}; // 买方减少得金额

            insert_dealorder(_sellorder.order_id, order_id, price, deal_asset, buyer, _sellorder.seller, trade_fee);
            // 增加收入记录和更新总收入
            insert_profit(fee_amount * 2);
            contract_asset profit_fee{ fee_amount * 2, platform_core_asset_id_VALUE};
            add_income(profit_fee);

            update_order_sell(_sellorder.order_id, trade_quantity, _sellorder.price);
            update_order_buy(order_id, trade_quantity, trade_fee, price);
            update_sell_order(_sellorder.order_id, _sellorder.quantity - trade_quantity);

            // 卖方收入
            withdraw_asset(_self, _sellorder.seller, platform_core_asset_id_VALUE, deal_amount - fee_amount); // 卖方手续费在这里扣除
            // 买方收入
            withdraw_asset(_self, buyer, quantity.asset_id, trade_quantity.amount);
            sub_balances(buyer, amount);
            sub_balances(_sellorder.seller, trade_quantity);
            
            quantity -= trade_quantity;
        } else {
            break;
        }
    }

    if (quantity.amount > 0) {
        insert_buy_order(order_id, quantity, price, buyer);
    }
}

void exchangeywk::remove_sell_order(uint64_t order_id) {
    for (auto it = sellorders.begin(); it != sellorders.end(); it++) {
        if(it->order_id == order_id) {
            sellorders.erase(it);
        }
    }
}

void exchangeywk::remove_buy_order(uint64_t order_id) {
    for (auto it = buyorders.begin(); it != buyorders.end(); it++) {
        if(it->order_id == order_id) {
            buyorders.erase(it);
        }
    }
}

void exchangeywk::cancel_sell_order_fun(uint64_t id, uint64_t seller) {
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "卖单信息不存在");
    graphene_assert(it->user == seller, "无权操作");
    graphene_assert(it->order_type == sell_order_type, "无权操作");
    graphene_assert(it->status == order_status_trading, "订单已完成或者已取消");

    contract_asset amount{it->un_deal_amount, it->amount.asset_id};
    sub_balances(seller, amount);
    withdraw_asset(_self, seller, it->amount.asset_id, it->un_deal_amount);
    orders.modify(it, 0, [&](auto &m_order) {
        m_order.status = order_status_cancel;
        m_order.refund_amount = amount;
    });
    
    remove_sell_order(it->id);
}

void exchangeywk::cancel_buy_order_fun(uint64_t id, uint64_t buyer) {
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "买单信息不存在");
    graphene_assert(it->user == buyer, "无权操作");
    graphene_assert(it->order_type == buy_order_type, "无权操作");
    graphene_assert(it->status == order_status_trading, "订单已完成或者已取消");

    int64_t refund_amount = it->pay_amount.amount - it->deal_amount * it->deal_price / asset_recision - it->fee_amount;
    contract_asset amount{refund_amount, platform_core_asset_id_VALUE};

    sub_balances(buyer, amount);
    withdraw_asset(_self, buyer, amount.asset_id, amount.amount);
    remove_buy_order(it->id);

    orders.modify(it, 0, [&](auto &m_order) {
        m_order.status = order_status_cancel;
        m_order.refund_amount = amount;
    });

}

void exchangeywk::insert_sysconfig(uint64_t id, uint64_t value) {
    auto it = sysconfigs.find(id);
    graphene_assert(it == sysconfigs.end(), "配置信息已存在");
    sysconfigs.emplace(0, [&](auto &a_config) {
        a_config.id = id;
        a_config.value = value;
    });
}

void exchangeywk::update_sysconfig(uint64_t id, uint64_t value) {
    auto it = sysconfigs.find(id);
    graphene_assert(it != sysconfigs.end(), "配置信息不存在");

    if (id == table_save_count_ID || id == once_delete_count_ID) {
        graphene_assert(it->value < value, "该配置只能增加不能减少");
    }
    if (id == platform_status_ID) {
        graphene_assert(value == platform_lock_status_value || value == platform_un_lock_status_value, "交易所状态值支持");
    }

    sysconfigs.modify(it, 0, [&](auto &o) {
        o.value = value;
    });
}

uint64_t exchangeywk::get_sysconfig(uint64_t id) {
    auto it = sysconfigs.find(id);
    graphene_assert(it != sysconfigs.end(), "配置信息不存在");
    return it->value;
}

void exchangeywk::authverify(uint64_t sender) {
    uint64_t profit_account_id = get_sysconfig(profit_account_id_ID);
    graphene_assert(sender == profit_account_id, "越权操作");
}

void exchangeywk::statusverify() {
    uint64_t platform_status = get_sysconfig(platform_status_ID);
    graphene_assert(platform_status == platform_un_lock_status_value, "平台已锁定，暂时不能交易");
}

void exchangeywk::add_income(contract_asset profit) {
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

void exchangeywk::sub_income(contract_asset profit) {
    graphene_assert(profit.amount > 0, "资金操作不能为负数");
    auto income_asset = incomes.find(profit.asset_id);
    graphene_assert(income_asset != incomes.end(), "当前收益没有对应得资产");
    graphene_assert(income_asset->amount >= profit.amount, "当前收益不足");
    
    incomes.modify(income_asset, 0, [&](auto &o) {
        o.amount -= profit.amount;
    });
    
} 

void exchangeywk::delete_dealorder(uint64_t deletecount) {
    uint64_t delete_count = 0;
    for(auto itr = dealorders.begin(); itr != dealorders.end();) {
        itr = dealorders.erase(itr);
        delete_count++;
        if (delete_count >= deletecount) {
            break;
        }
    }
}

uint64_t exchangeywk::insert_profit(int64_t profit_amount) {
    uint64_t pk = profits.available_primary_key();
    print("profits pk = ", pk);
    profits.emplace(0, [&](auto &a_profit) {
        a_profit.id = pk;
        a_profit.profit_amount = profit_amount;
        a_profit.profit_time = get_head_block_time();
    });
    uint64_t once_delete_count = get_sysconfig(once_delete_count_ID);
    if (pk > get_sysconfig(table_save_count_ID) && pk % once_delete_count == 0) {
        delete_profit(once_delete_count);
    }
    return pk;
}

// 数据删除
void exchangeywk::delete_profit(uint64_t deletecount) {
    uint64_t delete_count = 0;
    for(auto itr = profits.begin(); itr != profits.end();) {
        itr = profits.erase(itr);
        delete_count++;
        if (delete_count >= deletecount) {
            break;
        }
    }
}

// 数据删除
void exchangeywk::delete_order(uint64_t deletecount) {
    uint64_t delete_count = 0;
    for(auto itr = orders.begin(); itr != orders.end();) {
        itr = orders.erase(itr);
        delete_count++;
        if (delete_count >= deletecount) {
            break;
        }
    }
}

void exchangeywk::verifycoinstatus(uint64_t id, uint8_t ordertype) {
    uint8_t coinstatus = get_cointype(id);
    if (coinstatus == coin_exchange_on_status) {
        return;
    } else if (coinstatus == coin_exchange_buy_lock_status) {
        graphene_assert(ordertype == sell_order_type, "此币已经买锁定，不能购买");
    } else if (coinstatus == coin_exchange_sell_lock_status) {
        graphene_assert(ordertype == buy_order_type, "此币已经卖锁定，不能售卖");
    } else {
        graphene_assert(false, "此币已下架，暂停所有交易");
    }
}

uint8_t exchangeywk::get_cointype(uint64_t id) {
    auto it = cointypes.find(id);
    graphene_assert(it != cointypes.end(), "币种信息不存在");
    return it->value;
}
