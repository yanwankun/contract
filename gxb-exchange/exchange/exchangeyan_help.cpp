#include "exchangeyan.hpp"

using namespace graphene;


// 把出价高和创建时间早得放在前面
bool buyoredercomp(const exchangeyan::buyorder &a, const exchangeyan::buyorder &b){
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
bool selloredercomp(const exchangeyan::sellorder &a, const exchangeyan::sellorder &b){
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

void exchangeyan::authverify(uint64_t sender) {
    uint64_t profit_account_id = get_sysconfig(profit_account_id_ID);
    graphene_assert(sender == profit_account_id, "越权操作");
}

void exchangeyan::statusverify() {
    uint64_t platform_status = get_sysconfig(platform_status_ID);
    graphene_assert(platform_status == platform_un_lock_status_value, "平台已锁定，暂时不能交易");
}

// 增加余额
void exchangeyan::add_balances(uint64_t user, contract_asset quantity){
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
void exchangeyan::sub_balances(uint64_t user, contract_asset quantity){
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


// /**
//  * 内部转账
//  */
//  void exchangeyan::exchange_transfer(uint64_t from, uint64_t to, contract_asset quantity) {
//     auto it_from = accounts.find(from);
//     graphene_assert(it_from != accounts.end(), "转账发送者账户记录不存在");

//     // 发送者扣除相应金额
//     sub_balances(from, quantity);
//     // 接受者增加相应金额
//     add_balances(to, quantity);

//  }

// /**
//  * 内部lock转转实现(从lock账户到balance) 
//  */ 
// void exchangeyan::transfer_from_lock(uint64_t from, uint64_t to, contract_asset quantity) {
//     auto it_from = accounts.find(from);
//     graphene_assert(it_from != accounts.end(), "转账发送者账户记录不存在");

//     // 发送者扣除相应金额
//     sub_balances_lock(from, quantity);

//     // 接受者增加相应金额
//     add_balances(to, quantity);

// }

// // balance转账到lock账户
// void exchangeyan::balance_lock(uint64_t user, contract_asset quantity) {
//     auto it_user = accounts.find(user);
//     graphene_assert(it_user != accounts.end(), "锁定资产发送者账户记录不存在");

//     sub_balances(user, quantity);
//     add_balances_lock(user, quantity);

// }

// void exchangeyan::unlock_lock_balance(uint64_t user, contract_asset quantity) {
//     auto it_user = accounts.find(user);
//     graphene_assert(it_user != accounts.end(), "锁定资产发送者账户记录不存在");

//     add_balances(user, quantity);
//     sub_balances_lock(user, quantity);
// }


// void exchangeyan::add_income(contract_asset profit) {
//     graphene_assert(profit.amount > 0, "资金操作不能为负数");
//     auto income_asset = incomes.find(profit.asset_id);
//     if (income_asset == incomes.end()) {
//         incomes.emplace(0, [&](auto &o) {
//             o.asset_id = profit.asset_id;
//             o.amount = profit.amount;
//         });
//     } else {
//         incomes.modify(income_asset, 0, [&](auto &o) {
//             // todo 判断溢出问题
//             o.amount += profit.amount;
//         });
//     }
// }

// void exchangeyan::sub_income(contract_asset profit) {
//     graphene_assert(profit.amount > 0, "资金操作不能为负数");
//     auto income_asset = incomes.find(profit.asset_id);
//     graphene_assert(income_asset != incomes.end(), "当前收益没有对应得资产");
//     graphene_assert(income_asset->amount >= profit.amount, "当前收益不足");
    
//     incomes.modify(income_asset, 0, [&](auto &o) {
//         o.amount -= profit.amount;
//     });
    
// } 

void exchangeyan::update_sell_order(uint64_t id, contract_asset quantity) {

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

void exchangeyan::update_buy_order(uint64_t id, contract_asset quantity) {

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

uint64_t exchangeyan::insert_sell_order(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t seller) {
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

uint64_t exchangeyan::insert_buy_order(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t buyer) {
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

// uint64_t exchangeyan::insert_profit(contract_asset profit) {
//     uint64_t pk = profits.available_primary_key();
//     print("profits pk = ", pk);
//     profits.emplace(0, [&](auto &a_profit) {
//         a_profit.id = pk;
//         a_profit.profit_asset = profit;
//         a_profit.profit_time = get_head_block_time();
//     });
//     uint64_t once_delete_count = get_sysconfig(once_delete_count_ID);
//     if (pk > get_sysconfig(table_save_count_ID) && pk % once_delete_count == 0) {
//         delete_profit(once_delete_count);
//     }
//     return pk;
// }


uint64_t exchangeyan::insert_dealorder(int64_t price, contract_asset quantity) {
    uint64_t pk = dealorders.available_primary_key();
    print("dealorders pk = ", pk);
    dealorders.emplace(0, [&](auto &a_dealorder) {
        a_dealorder.id = pk;
        a_dealorder.price = price;
        a_dealorder.quantity = quantity;
        a_dealorder.order_time = get_head_block_time();
    });
    uint64_t once_delete_count = get_sysconfig(once_delete_count_ID);
    if (pk > get_sysconfig(table_save_count_ID) && pk % once_delete_count == 0) {
        delete_dealorder(once_delete_count);
    }
    return pk;
}

// uint64_t exchangeyan::insert_depositlog(uint64_t user, contract_asset amount) {
//     uint64_t pk = depositlogs.available_primary_key();
//     print("depositlogs pk = ", pk);
//     depositlogs.emplace(0, [&](auto &a_depositlog) {
//         a_depositlog.id = pk;
//         a_depositlog.user = user;
//         a_depositlog.quantity = amount;
//         a_depositlog.order_time = get_head_block_time();
//     });
//     uint64_t once_delete_count = get_sysconfig(once_delete_count_ID);
//     if (pk > get_sysconfig(table_save_count_ID) && pk % once_delete_count == 0) {
//         delete_depositlog(once_delete_count);
//     }
//     return pk;
// }
// uint64_t exchangeyan::insert_withdrawlog(uint64_t user, contract_asset amount) {
//     uint64_t pk = withdrawlogs.available_primary_key();
//     print("withdrawlogs pk = ", pk);
//     withdrawlogs.emplace(0, [&](auto &a_withdrawlog) {
//         a_withdrawlog.id = pk;
//         a_withdrawlog.user = user;
//         a_withdrawlog.quantity = amount;
//         a_withdrawlog.order_time = get_head_block_time();
//     });
//     uint64_t once_delete_count = get_sysconfig(once_delete_count_ID);
//     if (pk > get_sysconfig(table_save_count_ID) && pk % once_delete_count == 0) {
//         delete_withdrawlog(once_delete_count);
//     }
//     return pk;
// }

// void exchangeyan::insert_sysconfig(uint64_t id, uint64_t value) {
//     auto it = sysconfigs.find(id);
//     graphene_assert(it == sysconfigs.end(), "配置信息已存在");
//     sysconfigs.emplace(0, [&](auto &a_config) {
//         a_config.id = id;
//         a_config.value = value;
//     });
// }

// void exchangeyan::update_sysconfig(uint64_t id, uint64_t value) {
//     auto it = sysconfigs.find(id);
//     graphene_assert(it != sysconfigs.end(), "配置信息不存在");

//     if (id == table_save_count_ID || id == once_delete_count_ID) {
//         graphene_assert(it->value < value, "该配置只能增加不能减少");
//     }
//     if (id == platform_status_ID) {
//         graphene_assert(value == platform_lock_status_value || value == platform_un_lock_status_value, "交易所状态值不存在");
//     }

//     sysconfigs.modify(it, 0, [&](auto &o) {
//         o.value = value;
//     });
// }

// uint64_t exchangeyan::get_sysconfig(uint64_t id) {
//     auto it = sysconfigs.find(id);
//     graphene_assert(it != sysconfigs.end(), "配置信息不存在");
//     return it->value;
// }

// void exchangeyan::delete_sysconfig() {
//     for(auto itr = sysconfigs.begin(); itr != sysconfigs.end();) {
//         itr = sysconfigs.erase(itr);
//     }
// }

uint64_t exchangeyan::inser_dealorder(uint64_t sellorder_id, uint64_t buyorder_id, uint64_t price, contract_asset quantity, uint64_t buyer, uint64_t seller, contract_asset fee) {
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
    return pk;
}

uint64_t exchangeyan::insert_order(contract_asset pay_amount, contract_asset amount, uint64_t user, uint8_t order_type, int64_t price, uint8_t status) {
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
    return pk;
}

// insert_order_mp(contract_asset pay_amount, contract_asset amount, uint64_t user, uint8_t order_type, uint8_t status) {
//     uint64_t pk = orders.available_primary_key();
//     print("orders pk = ", pk);
//     orders.emplace(0, [&](auto &a_order) {
//         a_order.id = pk;
//         a_order.pay_amount = pay_amount;
//         a_order.amount = amount;
//         a_order.user = user;
//         a_order.order_type = order_type; // 买单和卖单
//         a_order.price = 0;
//         a_order.deal_price = 0;
//         if (order_type == sell_order_type) {
//             a_order.deal_price = price;
//         }
//         a_order.deal_amount = 0; // 成交数量
//         a_order.un_deal_amount = amount.amount; // 未成交数量
//         a_order.status = status; //订单状态
//         a_order.fee_amount = 0;
//         a_order.order_time = get_head_block_time();
//     });
//     return pk;
// }

void exchangeyan::update_order_sell(uint64_t id, contract_asset quantity) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "订单信息不存在");

    // int64_t deal_price = (it->deal_amount * it->deal_price + quantity.amount * price ) / (it->deal_amount + quantity.amount);

    orders.modify(it, 0, [&](auto &m_order) {
        m_order.amount -= quantity;
        // m_order.deal_price = deal_price;
        m_order.deal_amount += quantity.amount;
        m_order.un_deal_amount -= quantity.amount;
        if (m_order.un_deal_amount <= 0) {
            m_order.status = order_status_end;
        }
    });
}

void exchangeyan::update_order_buy(uint64_t id, contract_asset quantity, contract_asset fee, int64_t price) {

    graphene_assert(quantity.amount >= 0, "更新订单金额不能为负数");
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "订单信息不存在");

    int64_t deal_price = (it->deal_amount * it->deal_price + quantity.amount * price ) / (it->deal_amount + quantity.amount);

    orders.modify(it, 0, [&](auto &m_order) {
        m_order.amount -= quantity;
        m_order.deal_price = deal_price;
        m_order.deal_amount += quantity.amount;
        m_order.un_deal_amount -= quantity.amount;
        m_order.fee_amount += fee.amount;
        if (m_order.un_deal_amount <= 0) {
            m_order.status = order_status_end;
            // 有退款
            int64_t refund_amount = m_order.amount.amount - m_order.deal_amount * m_order.deal_price;
            if (refund_amount > 0) {
                withdraw_asset(_self, m_order.user, platform_core_asset_id_VALUE, refund_amount);
                m_order.refund_amount = refund_amount;
            }
        }
    });
}

// 当收到卖单请求时得处理方法
void exchangeyan::sell_order_fun(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t seller) {

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
        insert_sell_order(order_id, quantity, price, seller);
        return;
    }

    sort(match_buy_orders.begin(), match_buy_orders.end(), buyoredercomp);

    for (buyorder _buyorder : match_buy_orders) { 
        if (quantity.amount > 0) {
            // 如果有匹配 添加成交记录 添加手续费 把卖家的资产转给买家 把买家得核心资产转给卖家
            if (_buyorder.quantity.amount <= quantity.amount) {
                int64_t deal_amount = _buyorder.quantity.amount * price / asset_recision;
                int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

                contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
                contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
                contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE};

                inser_dealorder(order_id, _buyorder->order_id, price, deal_asset, _buyorder->buyer, seller, trade_fee);
                update_order_sell(order_id, _buyorder.quantity);
                update_order_buy(_buyorder->order_id, _buyorder.quantity, trade_fee, price);
                update_buy_order(_buyorder.id, _buyorder.quantity - _buyorder.quantity);

                // 卖方收入
                withdraw_asset(_self, seller, platform_core_asset_id_VALUE, deal_amount - trade_fee); // 卖方手续费在这里扣除
                // 买方收入
                withdraw_asset(_self, _buyorder->buyer, _buyorder.quantity.asset_id, _buyorder.quantity.amount);
                sub_balances(_buyorder->buyer, amount);
                sub_balances(seller, _buyorder.quantity);
                
                quantity -= _buyorder.quantity;
            } else {
                int64_t deal_amount = quantity.amount * price / asset_recision;
                int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

                contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
                contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
                contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE};

                inser_dealorder(order_id, _buyorder->order_id, price, deal_asset, _buyorder->buyer, seller, trade_fee);
                update_order_sell(order_id, quantity);
                update_order_buy(_buyorder->order_id, quantity, trade_fee, price);
                update_buy_order(_buyorder.id, _buyorder.quantity - quantity);

                // 卖方收入
                withdraw_asset(_self, seller, platform_core_asset_id_VALUE, deal_amount - trade_fee); // 卖方手续费在这里扣除
                // 买方收入
                withdraw_asset(_self, _buyorder->buyer, quantity.asset_id, quantity.amount);
                sub_balances(_buyorder->buyer, amount);
                sub_balances(seller, quantity);
                
                quantity -= quantity;
            }
        } else {
            break;
        }
    }

    if (quantity.amount > 0) {
        insert_sell_order(order_id, quantity, price, seller);
    }
}

// 收到买单得处理
void exchangeyan::buy_order_fun(uint64_t order_id, contract_asset quantity, int64_t price, uint64_t buyer) {
    auto idx = sellorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<sellorder> match_sell_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (itr->price <= price) {
            match_sell_orders.emplace_back(*itr);
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
                trade_quantity.amount = _sellorder.quantity.amoun;

                // int64_t deal_amount = _sellorder.quantity.amount * _sellorder.price / asset_recision;
                // int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

                // contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
                // contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
                // contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE}; // 买方减少得金额

                // inser_dealorder(_sellorder->order_id, order_id, price, deal_asset, buyer, _sellorder->seller, trade_fee);
                // update_order_sell(_sellorder->order_id, _sellorder->quantity);
                // update_order_buy(order_id, _sellorder->quantity, trade_fee, price);
                // update_sell_order(_sellorder->order_id, _sellorder->quantity - _sellorder->quantity);

                // // 卖方收入
                // withdraw_asset(_self, _sellorder.seller, platform_core_asset_id_VALUE, deal_amount - trade_fee); // 卖方手续费在这里扣除
                // // 买方收入
                // withdraw_asset(_self, buyer, quantity.asset_id, _sellorder.quantity.amount);
                // sub_balances(buyer, amount);
                // sub_balances(_sellorder.seller, _sellorder.quantity);
                
                // quantity -= _sellorder.quantity;
            } else {
                trade_quantity.amount = quantity.amoun;
                // int64_t deal_amount = quantity.amount * _sellorder.price / asset_recision;
                // int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

                // contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
                // contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
                // contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE}; // 买方减少得金额

                // inser_dealorder(_sellorder->order_id, order_id, price, deal_asset, buyer, _sellorder->seller, trade_fee);
                // update_order_sell(_sellorder->order_id, quantity);
                // update_order_buy(order_id, quantity, trade_fee, price);
                // update_sell_order(_sellorder->order_id, _sellorder->quantity - quantity);

                // // 卖方收入
                // withdraw_asset(_self, _sellorder.seller, platform_core_asset_id_VALUE, deal_amount - trade_fee); // 卖方手续费在这里扣除
                // // 买方收入
                // withdraw_asset(_self, buyer, quantity.asset_id, quantity.amount);
                // sub_balances(buyer, amount);
                // sub_balances(_sellorder.seller, quantity);

                // quantity -= quantity;
            }

            int64_t deal_amount = trade_quantity.amount * _sellorder.price / asset_recision;
            int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

            contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
            contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
            contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE}; // 买方减少得金额

            inser_dealorder(_sellorder->order_id, order_id, price, deal_asset, buyer, _sellorder->seller, trade_fee);
            update_order_sell(_sellorder->order_id, trade_quantity);
            update_order_buy(order_id, trade_quantity, trade_fee, price);
            update_sell_order(_sellorder->order_id, _sellorder->quantity - trade_quantity);

            // 卖方收入
            withdraw_asset(_self, _sellorder.seller, platform_core_asset_id_VALUE, deal_amount - trade_fee); // 卖方手续费在这里扣除
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

void exchangeyan::remove_sell_order(uint64_t order_id) {
    auto idx = sellorders.template get_index<N(orderid)>();
    auto match_itr_lower = idx.lower_bound(order_id);

    graphene_assert(it != sellorders.end() && it->order_id == order_id, "卖单信息不存在");
    sellorders.erase(it);
}

void exchangeyan::remove_buy_order(uint64_t order_id) {
    auto idx = buyorders.template get_index<N(orderid)>();
    auto match_itr_lower = idx.lower_bound(order_id);

    graphene_assert(it != buyorders.end() && it->order_id == order_id, "卖单信息不存在");
    buyorders.erase(it);
}

void exchangeyan::cancel_sell_order_fun(uint64_t id, uint64_t seller) {
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "卖单信息不存在");
    graphene_assert(it->user == seller, "无权操作");
    graphene_assert(it->status == order_status_trading, "订单已完成或者已取消");

    contract_asset amount{it->un_deal_amount, it->amount.asset_id};
    sub_balances(seller, amount);
    withdraw_asset(_self, seller, it->amount.asset_id, it->un_deal_amount);
    orders.modify(it, 0, [&](auto &m_order) {
        m_order.status = order_status_cancel;
    });
    
    remove_sell_order(it->order_id);
}

void exchangeyan::cancel_buy_order_fun(uint64_t id, uint64_t buyer) {
    auto it = orders.find(id);
    graphene_assert(it != orders.end(), "买单信息不存在");
    graphene_assert(it->user == buyer, "无权操作");
    graphene_assert(it->status == order_status_trading, "订单已完成或者已取消");

    int64_t refund_amount = it->pay_amount.amount - it->deal_amount * it->deal_price / asset_recision - it->fee_amount;
    contract_asset amount{refund_amount, platform_core_asset_id_VALUE};

    sub_balances(buyer, amount);
    withdraw_asset(_self, buyer, amount.asset_id, amount.amount);
    remove_buy_order(it->order_id);

    orders.modify(it, 0, [&](auto &m_order) {
        m_order.status = order_status_cancel;
    });

}


// // 数据删除
// void exchangeyan::delete_profit(uint64_t deletecount) {
//     uint64_t delete_count = 0;
//     for(auto itr = profits.begin(); itr != profits.end();) {
//         itr = profits.erase(itr);
//         delete_count++;
//         if (delete_count >= deletecount) {
//             break;
//         }
//     }
// }

// void exchangeyan::delete_depositlog(uint64_t deletecount) {
//     uint64_t delete_count = 0;
//     for(auto itr = depositlogs.begin(); itr != depositlogs.end();) {
//         itr = depositlogs.erase(itr);
//         delete_count++;
//         if (delete_count >= deletecount) {
//             break;
//         }
//     }
// }

// void exchangeyan::delete_withdrawlog(uint64_t deletecount) {
//     uint64_t delete_count = 0;
//     for(auto itr = withdrawlogs.begin(); itr != withdrawlogs.end();) {
//         itr = withdrawlogs.erase(itr);
//         delete_count++;
//         if (delete_count >= deletecount) {
//             break;
//         }
//     }
// }

// void exchangeyan::delete_dealorder(uint64_t deletecount) {
//     uint64_t delete_count = 0;
//     for(auto itr = dealorders.begin(); itr != dealorders.end();) {
//         itr = dealorders.erase(itr);
//         delete_count++;
//         if (delete_count >= deletecount) {
//             break;
//         }
//     }
// }

// void exchangeyan::ptcoin_lock() {
//     auto itr = pledges.find(plateform_deposite_gxc_ID);
//     graphene_assert(itr != pledges.end(), "保证金相关资金不存在");
//     graphene_assert(itr->amount.asset_id == ptcoin_trade_coin_id, "保证金资金出错");
//     int64_t transfer_ammount = 0;
//     if (itr->amount.amount < ptcoin_lock_not_all_min) {
//         transfer_ammount = itr->amount.amount;
//     } else {
//         transfer_ammount = (itr->amount.amount / 100) * ptcoin_lock_ratio;
//     }

//     contract_asset lock_amount{transfer_ammount, ptcoin_trade_coin_id};
//     sub_pledge(plateform_deposite_gxc_ID, lock_amount);
//     add_pledge(plateform_deposite_lock_gxc_ID, lock_amount);
// }

// void exchangeyan::delete_pledge(){
//     // 删除所有的账户
//     uint64_t profit_account_id = get_sysconfig(profit_account_id_ID);
//     for(auto itr = pledges.begin(); itr != pledges.end();) {
//         withdraw_asset(_self, profit_account_id, itr->amount.asset_id, itr->amount.amount);
//         itr = pledges.erase(itr);
//     }
// }