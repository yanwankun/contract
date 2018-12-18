#include "exchangeywk_help.cpp"

using namespace graphene;

void exchangeywk::init(){
    auto it = sysconfigs.find(profit_account_id_ID);
    graphene_assert(it == sysconfigs.end(), "配置信息已存在,不能进行init");
    
    insert_sysconfig(profit_account_id_ID, profit_account_id_VALUE);
    insert_sysconfig(max_match_order_count_ID, max_match_order_count_VALUE);
    insert_sysconfig(match_amount_times_ID, match_amount_times_VALUE);
    insert_sysconfig(table_save_count_ID, table_save_count_VALUE);
    insert_sysconfig(once_delete_count_ID, once_delete_count_VALUE);
    insert_sysconfig(platform_status_ID, platform_un_lock_status_value);
}

void exchangeywk::updateconfig(uint64_t id, uint64_t value) {
    uint64_t sender = get_trx_sender();
    authverify(sender);
    update_sysconfig(id, value);
}

// 添加支持得资产
void exchangeywk::addcointype(uint64_t id) {
    uint64_t sender = get_trx_sender();
    authverify(sender);

    auto it = cointypes.find(id);
    graphene_assert(it == cointypes.end(), "币种信息已存在");
    cointypes.emplace(0, [&](auto &a_coin) {
        a_coin.id = id;
        a_coin.value = value;
    });
}

// 配置更新
void exchangeywk::upcointype(uint64_t id, uint64_t value) {
    uint64_t sender = get_trx_sender();
    authverify(sender);

    auto it = cointypes.find(id);
    graphene_assert(it != cointypes.end(), "币种信息不存在");
    graphene_assert(value >= 1 && value <= 4, "币种状态不支持");
    cointypes.modify(it, 0, [&](auto &m_coin) {
        m_coin.value = value;
    });
}

void exchangeywk::pdsellorder(int64_t price) {

    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    verifycoinstatus(asset_id, sell_order_type);
    contract_asset sell_amount{asset_amount, asset_id};

    graphene_assert(sell_amount.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");

    uint64_t user = get_trx_sender();
    add_balances(user, sell_amount);
    uint64_t order_id = insert_order(sell_amount, sell_amount, user, sell_order_type, price, order_status_trading); 
    sell_order_fun(order_id, sell_amount, price, user);

}

void exchangeywk::pdbuyorder(contract_asset quantity, int64_t price) {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    verifycoinstatus(asset_id, buy_order_type);

    graphene_assert(asset_amount > quantity.amount * price * (exchange_fee_base_amount/(exchange_fee_base_amount + exchange_fee))/ asset_recision , "支付金额不足以支付交易金额和手续费");
    contract_asset pay_amount{asset_amount, asset_id};

    graphene_assert(quantity.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");

    uint64_t user = get_trx_sender();
    add_balances(user, pay_amount);

    uint64_t order_id = insert_order(pay_amount, quantity, user, buy_order_type, price, order_status_trading);
    buy_order_fun(order_id, quantity, price, user);
}

void exchangeywk::cancelorder(uint8_t type, uint64_t id) {
    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        cancel_sell_order_fun(id, sender); 
    } else if (buy_order_type == type) {
        cancel_buy_order_fun(id, sender);
    }
}

void exchangeywk::mpbuy(contract_asset quantity) {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    verifycoinstatus(asset_id, buy_order_type);

    contract_asset pay_amount{asset_amount, asset_id};
    uint64_t buyer = get_trx_sender();

    auto idx = sellorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<sellorder> match_sell_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        match_sell_orders.emplace_back(*itr);
    }

    if (match_sell_orders.size() == 0) {
        withdraw_asset(_self, buyer, asset_id, asset_amount);
        return;
    }

    uint64_t order_id = insert_order(pay_amount, quantity, buyer, buy_order_type, 0, order_status_trading);
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

            contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
            contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
            // contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE}; // 买方减少得金额

            insert_dealorder(_sellorder.order_id, order_id, _sellorder.price, deal_asset, buyer, _sellorder.seller, trade_fee);
            // 增加收入记录和更新总收入
            insert_profit(fee_amount * 2);
            contract_asset profit_fee{ fee_amount * 2, platform_core_asset_id_VALUE};
            add_income(profit_fee);

            update_order_sell(_sellorder.order_id, trade_quantity, _sellorder.price);
            update_order_buy(order_id, trade_quantity, trade_fee, _sellorder.price);
            update_sell_order(_sellorder.order_id, _sellorder.quantity - trade_quantity);

            // 卖方收入
            withdraw_asset(_self, _sellorder.seller, platform_core_asset_id_VALUE, deal_amount - fee_amount); // 卖方手续费在这里扣除
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

void exchangeywk::mpsell() {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    verifycoinstatus(asset_id, sell_order_type);

    contract_asset sell_amount{asset_amount, asset_id};
    uint64_t seller = get_trx_sender();

    auto idx = buyorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(sell_amount.asset_id);
    auto match_itr_upper = idx.upper_bound(sell_amount.asset_id);

    vector<buyorder> match_buy_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        match_buy_orders.emplace_back(*itr);
    }

    if (match_buy_orders.size() == 0) {
        withdraw_asset(_self, seller, asset_id, asset_amount);
        return;
    }

    uint64_t order_id = insert_order(sell_amount, sell_amount, seller, sell_order_type, 0, order_status_trading);
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

            contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
            contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
            contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE};

            insert_dealorder(order_id, _buyorder.order_id, _buyorder.price, deal_asset, _buyorder.buyer, seller, trade_fee);
            // 增加收入记录和更新总收入
            insert_profit(fee_amount * 2);
            contract_asset profit_fee{ fee_amount * 2, platform_core_asset_id_VALUE};
            add_income(profit_fee);

            update_order_sell(order_id, _buyorder.quantity, _buyorder.price);
            update_order_buy(_buyorder.order_id, _buyorder.quantity, trade_fee, _buyorder.price);
            update_buy_order(_buyorder.id, _buyorder.quantity - _buyorder.quantity);

            // 卖方收入
            withdraw_asset(_self, seller, platform_core_asset_id_VALUE, deal_amount - fee_amount); // 卖方手续费在这里扣除
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
void exchangeywk::deleteall() {
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

    // 删除所有得配置
    for(auto itr = sysconfigs.begin(); itr != sysconfigs.end();) {
        itr = sysconfigs.erase(itr);
    }
    uint64_t max_table_size = get_sysconfig(table_save_count_ID) + get_sysconfig(once_delete_count_ID);
    delete_profit(max_table_size);
    delete_dealorder(max_table_size);
    delete_order(max_table_size);
}

GRAPHENE_ABI(exchangeywk, (init)(updateconfig)(addcointype)(upcointype)(pdsellorder)(pdbuyorder)(cancelorder)(mpbuy)(mpsell)(deleteall))