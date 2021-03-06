#include "exchangeywkt_help.cpp"

using namespace graphene;

void exchangeywkt::init(){
    uint64_t sender = get_trx_sender();
    uint64_t owner_id;
    auto it = sysconfigs.find(profit_account_id_ID);
    if (it != sysconfigs.end()) {
        owner_id = it->value;
    } else {
        owner_id = profit_account_id_VALUE;
    }
    graphene_assert(owner_id == sender, "只有受益人才可以初始化");
    
    insert_sysconfig(profit_account_id_ID, profit_account_id_VALUE);
    insert_sysconfig(max_match_order_count_ID, max_match_order_count_VALUE);
    insert_sysconfig(match_amount_times_ID, match_amount_times_VALUE);
    insert_sysconfig(table_save_count_ID, table_save_count_VALUE);
    insert_sysconfig(once_delete_count_ID, once_delete_count_VALUE);
    insert_sysconfig(platform_status_ID, platform_un_lock_status_value);
}

void exchangeywkt::updateconfig(uint64_t id, uint64_t value) {
    uint64_t sender = get_trx_sender();
    authverify(sender);
    update_sysconfig(id, value);
}

// 配置更新
void exchangeywkt::setcoin(uint64_t id, uint8_t value) {
    graphene_assert(value >= 1 && value <= 4, "币种状态不支持");

    uint64_t sender = get_trx_sender();
    authverify(sender);

    auto it = cointypes.find(id);
    if (it == cointypes.end()) {
        cointypes.emplace(0, [&](auto &a_coin) {
            a_coin.id = id;
            a_coin.value = coin_exchange_on_status;
        });
    } else {
        cointypes.modify(it, 0, [&](auto &m_coin) {
            m_coin.value = value;
        });
    }
}

// 配置更新
void exchangeywkt::setptcoin(uint64_t id, uint8_t value) {
    graphene_assert(value == 1 || value == 2, "币种状态不支持");
    uint64_t sender = get_trx_sender();
    authverify(sender);

    auto it = ptcointypes.find(id);
    if (it == ptcointypes.end()) {
        ptcointypes.emplace(0, [&](auto &a_coin) {
            a_coin.id = id;
            a_coin.value = coin_exchange_on_status;
        });
    } else {
        ptcointypes.modify(it, 0, [&](auto &m_coin) {
            m_coin.value = value;
        });
    }
}

void exchangeywkt::fetchprofit(std::string to_account, contract_asset amount) {
    uint64_t sender = get_trx_sender();
    authverify(sender);

    int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
    graphene_assert(account_id >= 0, "目的账户不存在");

    sub_income(amount);
    withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
}

void exchangeywkt::pdsellorder(int64_t price, uint64_t core_asset_id) {

    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    graphene_assert(asset_id != core_asset_id, "买卖资产id不能一样");
    verifycoinstatus(asset_id, sell_order_type);
    verifyptcoinstatus(core_asset_id);
    contract_asset sell_amount{asset_amount, asset_id};

    graphene_assert(sell_amount.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");

    uint64_t user = get_trx_sender();
    add_balances(user, sell_amount);
    uint64_t order_id = insert_order(sell_amount, sell_amount, user, sell_order_type, core_asset_id, price, order_status_trading); 
    sell_order_fun(order_id, sell_amount, price, user, core_asset_id);

}

void exchangeywkt::pdbuyorder(contract_asset quantity, int64_t price) {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    graphene_assert(asset_id != quantity.asset_id, "买卖资产id不能一样");
    verifycoinstatus(quantity.asset_id, buy_order_type);
    verifyptcoinstatus(asset_id);

    graphene_assert(asset_amount * exchange_fee_base_amount/(exchange_fee_base_amount + exchange_fee) > quantity.amount * price / asset_recision , "支付金额不足以支付交易金额和手续费");
    contract_asset pay_amount{asset_amount, asset_id};

    graphene_assert(quantity.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");

    uint64_t user = get_trx_sender();
    add_balances(user, pay_amount);

    uint64_t order_id = insert_order(pay_amount, quantity, user, buy_order_type, asset_id, price, order_status_trading);
    buy_order_fun(order_id, quantity, price, user, asset_id);
}

void exchangeywkt::cancelorder(uint8_t type, uint64_t id) {
    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        cancel_sell_order_fun(id, sender); 
    } else if (buy_order_type == type) {
        cancel_buy_order_fun(id, sender);
    }
}

void exchangeywkt::mpbuy(contract_asset quantity) {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    graphene_assert(asset_id != quantity.asset_id, "买卖资产id不能一样");
    verifycoinstatus(quantity.asset_id, buy_order_type);
    verifyptcoinstatus(asset_id);

    contract_asset pay_amount{asset_amount, asset_id};
    uint64_t buyer = get_trx_sender();

    auto idx = sellorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<sellorder> match_sell_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (itr->core_asset_id == asset_id) {
            match_sell_orders.emplace_back(*itr);
        }
    }

    if (match_sell_orders.size() == 0) {
        withdraw_asset(_self, buyer, asset_id, asset_amount);
        return;
    }

    uint64_t order_id = insert_order(pay_amount, quantity, buyer, buy_order_type, asset_id, 0, order_status_trading);
    sort(match_sell_orders.begin(), match_sell_orders.end(), selloredercomp);
    int64_t pay_amount_amount = asset_amount * exchange_fee_base_amount / (exchange_fee_base_amount + exchange_fee);
    for (sellorder _sellorder : match_sell_orders) { 
        if (pay_amount_amount > 0 && quantity.amount > 0) {
            int64_t deal_quantity_amount;
            int64_t deal_amount;
            if (_sellorder.quantity.amount <= quantity.amount) {
                deal_amount = _sellorder.quantity.amount * _sellorder.price / asset_recision;
                if (pay_amount_amount >= deal_amount) {
                    deal_quantity_amount = _sellorder.quantity.amount;
                } else {
                    deal_quantity_amount = pay_amount_amount * asset_recision /  _sellorder.price;
                    deal_amount = pay_amount_amount;
                }
            } else {
                deal_amount = quantity.amount * _sellorder.price / asset_recision;
                if (pay_amount_amount >= deal_amount) {
                    deal_quantity_amount = quantity.amount;
                } else {
                    deal_quantity_amount = pay_amount_amount * asset_recision / _sellorder.price;
                    deal_amount = pay_amount_amount;
                }
            }
            // 成交资产
            contract_asset trade_quantity{deal_quantity_amount, quantity.asset_id};

            int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

            contract_asset deal_asset{ deal_amount, _sellorder.core_asset_id};
            contract_asset trade_fee{ fee_amount, _sellorder.core_asset_id};
            // contract_asset amount{deal_amount + fee_amount, _sellorder.core_asset_id}; // 买方减少得金额

            insert_dealorder(_sellorder.order_id, order_id, _sellorder.core_asset_id, _sellorder.price, deal_asset, buyer, _sellorder.seller, trade_fee);
            // 增加收入记录和更新总收入
            contract_asset profit_fee{ fee_amount * 2, _sellorder.core_asset_id};
            add_income(profit_fee);
            insert_profit(profit_fee);

            update_order_sell(_sellorder.order_id, trade_quantity, _sellorder.price);
            update_order_buy(order_id, trade_quantity, trade_fee, _sellorder.price);
            update_sell_order(_sellorder.order_id, _sellorder.quantity - trade_quantity);

            // 卖方收入
            withdraw_asset(_self, _sellorder.seller, _sellorder.core_asset_id, deal_amount - fee_amount); // 卖方手续费在这里扣除
            // 买方收入
            withdraw_asset(_self, buyer, quantity.asset_id, deal_quantity_amount);
            // sub_balances(buyer, amount);
            pay_amount_amount -= deal_amount + fee_amount;
            sub_balances(_sellorder.seller, trade_quantity);
            
            quantity -= trade_quantity;
        } else {
            break;
        }
    }

    if (pay_amount_amount > 0) {
        end_order(order_id);
    }

}

void exchangeywkt::mpsell(uint64_t core_asset_id) {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    graphene_assert(asset_id != core_asset_id, "买卖资产id不能一样");
    verifycoinstatus(asset_id, sell_order_type);
    verifyptcoinstatus(core_asset_id);

    contract_asset sell_amount{asset_amount, asset_id};
    uint64_t seller = get_trx_sender();

    auto idx = buyorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(sell_amount.asset_id);
    auto match_itr_upper = idx.upper_bound(sell_amount.asset_id);

    vector<buyorder> match_buy_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        if (core_asset_id == itr->core_asset_id) {
            match_buy_orders.emplace_back(*itr);
        }
    }

    if (match_buy_orders.size() == 0) {
        withdraw_asset(_self, seller, asset_id, asset_amount);
        return;
    }

    uint64_t order_id = insert_order(sell_amount, sell_amount, seller, sell_order_type, core_asset_id, 0, order_status_trading);
    sort(match_buy_orders.begin(), match_buy_orders.end(), buyoredercomp);

    for (buyorder _buyorder : match_buy_orders) { 
        if (sell_amount.amount > 0) {
            contract_asset trade_quantity{0, sell_amount.asset_id};
            if (_buyorder.quantity.amount <= sell_amount.amount) {
                trade_quantity.amount = _buyorder.quantity.amount;
            } else {
                trade_quantity.amount = sell_amount.amount;
            }
            int64_t deal_amount = _buyorder.quantity.amount * _buyorder.price / asset_recision;
            int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

            contract_asset deal_asset{ deal_amount, _buyorder.core_asset_id};
            contract_asset trade_fee{ fee_amount, _buyorder.core_asset_id};
            contract_asset amount{deal_amount + fee_amount, _buyorder.core_asset_id};

            insert_dealorder(order_id, _buyorder.order_id, core_asset_id, _buyorder.price, deal_asset, _buyorder.buyer, seller, trade_fee);
            // 增加收入记录和更新总收入
            contract_asset profit_fee{ fee_amount * 2, _buyorder.core_asset_id};
            add_income(profit_fee);
            insert_profit(profit_fee);

            update_order_sell(order_id, _buyorder.quantity, _buyorder.price);
            update_order_buy(_buyorder.order_id, _buyorder.quantity, trade_fee, _buyorder.price);
            update_buy_order(_buyorder.id, _buyorder.quantity - _buyorder.quantity);

            // 卖方收入
            withdraw_asset(_self, seller, _buyorder.core_asset_id, deal_amount - fee_amount); // 卖方手续费在这里扣除
            // 买方收入
            withdraw_asset(_self, _buyorder.buyer, _buyorder.quantity.asset_id, _buyorder.quantity.amount);
            sub_balances(_buyorder.buyer, amount);
            sub_balances(seller, _buyorder.quantity);
            
            sell_amount -= _buyorder.quantity;
        } else {
            break;
        }
    }

    if (sell_amount.amount > 0) {
        end_order(order_id);
    }
}

// @abi action
void exchangeywkt::deleteall() {
    uint64_t sender = get_trx_sender();
    authverify(sender);

    // 删除所有的账户
    for(auto it_user = accounts.begin(); it_user != accounts.end();) {
        for (auto asset_it = it_user->balances.begin(); asset_it != it_user->balances.end(); ++asset_it) {
            if (asset_it->amount > 0) {
                withdraw_asset(_self, it_user->owner, asset_it->asset_id, asset_it->amount);
            }
        }
        it_user = accounts.erase(it_user);
    }

    // 提取所有收益
    uint64_t profit_account_id = get_sysconfig(profit_account_id_ID);
    for(auto itr = incomes.begin(); itr != incomes.end();) {
        if (itr->amount > 0) {
            withdraw_asset(_self, profit_account_id, itr->asset_id, itr->amount);
        }
        itr = incomes.erase(itr);
    }

    // 删除所有得卖单
    for(auto itr = sellorders.begin(); itr != sellorders.end();) {
        itr = sellorders.erase(itr);
    }

    // 删除所有得买单
    for(auto itr = buyorders.begin(); itr != buyorders.end();) {
        itr = buyorders.erase(itr);
    }

    // 删除所有得币种
    for(auto itr = cointypes.begin(); itr != cointypes.end();) {
        itr = cointypes.erase(itr);
    }

    // 删除所有得币种
    for(auto itr = ptcointypes.begin(); itr != ptcointypes.end();) {
        itr = ptcointypes.erase(itr);
    }

    uint64_t max_table_size = get_sysconfig(table_save_count_ID) + get_sysconfig(once_delete_count_ID);
    delete_profit(max_table_size);
    delete_dealorder(max_table_size);
    delete_order(max_table_size);

    // 删除所有得配置
    for(auto itr = sysconfigs.begin(); itr != sysconfigs.end();) {
        itr = sysconfigs.erase(itr);
    }
}

GRAPHENE_ABI(exchangeywkt, (init)(updateconfig)(setcoin)(setptcoin)(fetchprofit)(pdsellorder)(pdbuyorder)(cancelorder)(mpbuy)(mpsell)(deleteall))