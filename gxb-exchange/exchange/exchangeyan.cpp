#include "exchangeyan_help.cpp"

using namespace graphene;


void exchangeyan::sellorder(int64_t price) {

    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    contract_asset sell_amount{asset_amount, asset_id};

    graphene_assert(sell_amount.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");

    uint64_t user = get_trx_sender();
    add_balances(user, sell_amount);
    uint64_t order_id = insert_order(sell_amount, sell_amount, user, sell_order_type, price, order_status_trading); 
    sell_order_fun(order_id, sell_amount, price, user);

}

void exchangeyan::buyorder(contract_asset quantity, int64_t price) {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();

    graphene_assert(asset_amount > quantity.amount * price * (exchange_fee_base_amount/(exchange_fee_base_amount + exchange_fee))/ asset_recision , "支付金额不足以支付交易金额和手续费");
    contract_asset pay_amount{asset_amount, asset_id};

    graphene_assert(quantity.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");

    uint64_t user = get_trx_sender();
    add_balances(user, pay_amount);

    uint64_t order_id = insert_order(pay_amount, quantity, user, buy_order_type, price, order_status_trading);
    buy_order_fun(order_id, quantity, price, user);
}

void exchangeyan::cancelorder(uint8_t type, uint64_t id) {
    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        cancel_sell_order_fun(id, sender); 
    } else if (buy_order_type == type) {
        cancel_buy_order_fun(id, sender);
    }
}

void exchangeyan::mpbuy(contract_asset quantity) {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
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
            if (_sellorder.quantity.amount <= quantity.amount) {
                int64_t deal_quantity_amount;
                int64_t deal_amount = _sellorder.quantity.amount * _sellorder.price / asset_recision;
                if (pay_amount_amount >= deal_amount) {
                    deal_quantity_amount = _sellorder.quantity.amount;
                } else {
                    deal_quantity_amount = pay_amount_amount * asset_recision / price;
                    deal_amount = pay_amount_amount;
                }
                // 成交资产
                contract_asset trade_quantity{deal_quantity_amount, quantity.asset_id};

                int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

                contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
                contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
                // contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE}; // 买方减少得金额

                inser_dealorder(_sellorder->order_id, order_id, _sellorder.price, deal_asset, buyer, _sellorder->seller, trade_fee);
                update_order_sell(_sellorder->order_id, trade_quantity);
                update_order_buy(order_id, trade_quantity, trade_fee, price);
                update_sell_order(_sellorder->order_id, _sellorder->quantity - trade_quantity);

                // 卖方收入
                withdraw_asset(_self, _sellorder.seller, platform_core_asset_id_VALUE, deal_amount - trade_fee); // 卖方手续费在这里扣除
                // 买方收入
                withdraw_asset(_self, buyer, quantity.asset_id, deal_quantity_amount);
                // sub_balances(buyer, amount);
                pay_amount_amount -= deal_amount + fee_amount;
                sub_balances(_sellorder.seller, trade_quantity);
                
                quantity -= trade_quantity;
            } else {
                int64_t deal_quantity_amount;
                int64_t deal_amount = quantity.amount * _sellorder.price / asset_recision;
                if (pay_amount_amount >= deal_amount) {
                    deal_quantity_amount = quantity.amount;
                } else {
                    deal_quantity_amount = pay_amount_amount * asset_recision / price;
                    deal_amount = pay_amount_amount;
                }
                int64_t fee_amount = deal_amount * exchange_fee / exchange_fee_base_amount;

                contract_asset deal_asset{ deal_amount, platform_core_asset_id_VALUE};
                contract_asset trade_fee{ fee_amount, platform_core_asset_id_VALUE};
                contract_asset amount{deal_amount + fee_amount, platform_core_asset_id_VALUE}; // 买方减少得金额

                inser_dealorder(_sellorder->order_id, order_id, _sellorder.price, deal_asset, buyer, _sellorder->seller, trade_fee);
                update_order_sell(_sellorder->order_id, deal_quantity_amount);
                update_order_buy(order_id, deal_quantity_amount, trade_fee, price);
                update_sell_order(_sellorder->order_id, _sellorder->quantity - deal_quantity_amount);

                // 卖方收入
                withdraw_asset(_self, _sellorder.seller, platform_core_asset_id_VALUE, deal_amount - trade_fee); // 卖方手续费在这里扣除
                // 买方收入
                withdraw_asset(_self, buyer, quantity.asset_id, deal_quantity_amount);
                pay_amount_amount -= deal_amount + fee_amount;
                sub_balances(_sellorder.seller, deal_quantity_amount);

                quantity -= deal_quantity_amount;
            }
        } else {
            break;
        }
    }

    if (pay_amount_amount > 0) {
        int64_t refund_asset_amount = pay_amount_amount * (exchange_fee_base_amount + exchange_fee) / exchange_fee_base_amount;
        withdraw_asset(_self, buyer, platform_core_asset_id_VALUE, refund_asset_amount);
    }

}

void exchangeyan::mpsell() {
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    contract_asset sell_amount{asset_amount, asset_id};
    uint64_t seller = get_trx_sender();

    auto idx = buyorders.template get_index<N(asset)>();
    auto match_itr_lower = idx.lower_bound(quantity.asset_id);
    auto match_itr_upper = idx.upper_bound(quantity.asset_id);

    vector<buyorder> match_buy_orders;
    for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
        match_buy_orders.emplace_back(*itr);
    }

    if (match_buy_orders.size() == 0) {
        withdraw_asset(_self, seller, asset_id, asset_amount);
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
        withdraw_asset(_self, seller, asset_id, quantity.amount);
    }
}

GRAPHENE_ABI(exchangeyan, (sellorder)(buyorder)(cancelorder)(mpbuy)(mpsell))


/**
 * 
void exchangeyan::deposit()
{
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    contract_asset amount{asset_amount, asset_id};

    uint64_t owner = get_trx_sender();
    add_balances(owner, amount);
    insert_depositlog(owner, amount);
}

void exchangeyan::withdraw(std::string to_account, contract_asset amount)
{

    int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
    graphene_assert(account_id >= 0, "目的账户不存在");
    graphene_assert(amount.amount > 0, "体现资产数目不能为小于或者等于零");

    uint64_t owner = get_trx_sender();
    auto it = accounts.find(owner);
    graphene_assert(it != accounts.end(), "账户没有该资产");

    sub_balances(owner, amount);
    withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
    insert_withdrawlog(owner, amount);
}

/*
 * 挂单说明 
 * type 挂单类型 1 买单 2 卖单
 * quantity 挂单金额
 * price 挂单单价  
 * 
 * 处理过程说明 
 * 其次 如果是卖单， 就从买单列表中查询相同资产得买单，找出所有价格合适得订单，从中匹配交易，如果有剩余则存入卖单列表，如没有则结束， 买单跟卖单处理过程类似
 * 
 * 
void exchangeyan::pendingorder(uint8_t type, contract_asset quantity, int64_t price) 
{
    statusverify();
    graphene_assert(quantity.amount > 0, "挂单金额不能小于或等于零");
    graphene_assert(price > 0, "挂单价格不能小于或等于零");
    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        sell_order_fun(quantity, price, sender); 
    } else if (buy_order_type == type) {
        buy_order_fun(quantity, price, sender);
    }

}

void exchangeyan::cancelorder(uint8_t type, uint64_t id) {

    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        cancel_sell_order_fun(id, sender); 
    } else if (buy_order_type == type) {
        cancel_buy_order_fun(id, sender);
    }
}

void exchangeyan::fetchprofit(std::string to_account, contract_asset amount) {
    uint64_t sender = get_trx_sender();
    authverify(sender);

    int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
    graphene_assert(account_id >= 0, "目的账户不存在");

    sub_income(amount);
    withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
}

void exchangeyan::updateconfig(uint64_t id, uint64_t value) {
    uint64_t sender = get_trx_sender();
    authverify(sender);
    update_sysconfig(id, value);
}

void exchangeyan::init(){
    auto it = sysconfigs.find(profit_account_id_ID);
    graphene_assert(it == sysconfigs.end(), "配置信息已存在,不能进行init");
    
    insert_sysconfig(profit_account_id_ID, profit_account_id_VALUE);
    insert_sysconfig(max_match_order_count_ID, max_match_order_count_VALUE);
    insert_sysconfig(match_amount_times_ID, match_amount_times_VALUE);
    insert_sysconfig(table_save_count_ID, table_save_count_VALUE);
    insert_sysconfig(once_delete_count_ID, once_delete_count_VALUE);
    insert_sysconfig(platform_status_ID, platform_un_lock_status_value);

}

void exchangeyan::deleteall(){
    uint64_t sender = get_trx_sender();
    authverify(sender);

    // 删除所有得卖单
    for(auto itr = sellorders.begin(); itr != sellorders.end();) {
        unlock_lock_balance(itr->seller, itr->quantity);
        itr = sellorders.erase(itr);
    }

    // 删除所有得买单
    for(auto itr = buyorders.begin(); itr != buyorders.end();) {
        contract_asset buy_asset{ itr->quantity.amount * itr->price / asset_recision, platform_core_asset_id_VALUE};
        unlock_lock_balance(itr->buyer, buy_asset);
        itr = buyorders.erase(itr);
    }

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

    delete_sysconfig();
    delete_pledge();
    uint64_t max_table_size = get_sysconfig(table_save_count_ID) + get_sysconfig(once_delete_count_ID);
    delete_profit(max_table_size);
    delete_depositlog(max_table_size);
    delete_withdrawlog(max_table_size);
    delete_dealorder(max_table_size);
}


void exchangeyan::ptdeposite(){
    uint64_t asset_id = get_action_asset_id();
    graphene_assert(asset_id == ptcoin_trade_coin_id || asset_id == platform_core_asset_id_VALUE, "该类资产不支持");
    int64_t asset_amount = get_action_asset_amount();
    contract_asset amount{asset_amount, asset_id};

    if (asset_id == ptcoin_trade_coin_id) { // 相当于gxc
        add_pledge(plateform_deposite_gxc_ID, amount);
    } else { // 预先存储得平台币，别人可以用gxc来购买
        add_pledge(plateform_deposite_platecoin_ID, amount); 
    }
}

void exchangeyan::ptwithdraw(contract_asset amount) {
    uint64_t sender = get_trx_sender();
    authverify(sender);

    uint64_t asset_id = amount.asset_id;
    graphene_assert(asset_id == ptcoin_trade_coin_id || asset_id == platform_core_asset_id_VALUE, "该类资产不支持");

    if (asset_id == ptcoin_trade_coin_id) { 
        sub_pledge(plateform_deposite_gxc_ID, amount);
    } else { 
        sub_pledge(plateform_deposite_platecoin_ID, amount); 
    }
}

void exchangeyan::buyptcoin() {
    uint64_t asset_id = get_action_asset_id();
    graphene_assert(asset_id == ptcoin_trade_coin_id, "该类资产不支持");
    int64_t asset_amount = get_action_asset_amount();
    graphene_assert(asset_amount >= 100000 && asset_amount%100000 == 0, "购买支付金额不能低于100000且必须为100000得整数数倍");

    int64_t ptcoint_amount = asset_amount * ptcoin_ratio;
    int64_t fee_amount = (ptcoint_amount / 100) * ptcoin_trade_fee_ratio;
    fee_amount = fee_amount > ptcoin_trade_fee_max ? ptcoin_trade_fee_max : fee_amount;
    fee_amount = fee_amount < ptcoin_trade_fee_min ? ptcoin_trade_fee_min : fee_amount;

    graphene_assert(fee_amount < ptcoint_amount, "金额太小");
    uint64_t sender = get_trx_sender();

    // 锁定资产
    contract_asset lock_amount{asset_amount, asset_id};
    add_pledge(plateform_deposite_lock_gxc_ID, lock_amount);

    // 增加用户余额和系统收益
    contract_asset buy_amount{ptcoint_amount - fee_amount, platform_core_asset_id_VALUE};
    contract_asset profit_amount{fee_amount, platform_core_asset_id_VALUE};

    add_balances(sender, buy_amount);
    insert_profit(profit_amount);
    add_income(profit_amount);
}

void exchangeyan::sellptcoin(contract_asset amount) {
    graphene_assert(amount.asset_id == platform_core_asset_id_VALUE, "该类资产不支持");

    int64_t fee_amount = (amount.amount / 100) * ptcoin_trade_fee_ratio;
    fee_amount = fee_amount > ptcoin_trade_fee_max ? ptcoin_trade_fee_max : fee_amount;
    fee_amount = fee_amount < ptcoin_trade_fee_min ? ptcoin_trade_fee_min : fee_amount;

    contract_asset profit_amount{fee_amount, platform_core_asset_id_VALUE};
    insert_profit(profit_amount);
    add_income(profit_amount);

    contract_asset sell_amount{(amount.amount - fee_amount) / ptcoin_ratio, platform_core_asset_id_VALUE};

    auto itr = pledges.find(plateform_deposite_lock_gxc_ID);
    graphene_assert(itr != pledges.end(), "保证金相关资金不存在");
    graphene_assert(itr->amount.asset_id == ptcoin_trade_coin_id, "保证金资金出错");

    if (sell_amount.amount > itr->amount.amount) {
        ptcoin_lock();
        return;
    }
    uint64_t sender = get_trx_sender();
    sub_balances(sender, amount);
    sub_pledge(plateform_deposite_lock_gxc_ID, sell_amount);
    withdraw_asset(_self, sender, sell_amount.asset_id, sell_amount.amount);
}

GRAPHENE_ABI(exchangeyan, (deposit)(withdraw)(pendingorder)(cancelorder)(fetchprofit)(updateconfig)(init)(deleteall)(ptdeposite)(ptwithdraw)(buyptcoin)(sellptcoin))


**/
