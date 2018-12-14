#include "gxc_exchange_help.cpp"

using namespace graphene;

void gxcexchangec::deposit()
{
    int64_t asset_amount = get_action_asset_amount();
    uint64_t asset_id = get_action_asset_id();
    contract_asset amount{asset_amount, asset_id};

    uint64_t owner = get_trx_sender();
    add_balances(owner, amount);
    insert_depositlog(owner, amount);
}

void gxcexchangec::withdraw(std::string to_account, contract_asset amount)
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
void gxcexchangec::pendingorder(uint8_t type, contract_asset quantity, int64_t price) 
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

void gxcexchangec::cancelorder(uint8_t type, uint64_t id) {

    uint64_t sender = get_trx_sender();
    if (sell_order_type == type) {
        cancel_sell_order_fun(id, sender); 
    } else if (buy_order_type == type) {
        cancel_buy_order_fun(id, sender);
    }
}

void gxcexchangec::fetchprofit(std::string to_account, contract_asset amount) {
    uint64_t sender = get_trx_sender();
    authverify(sender);

    int64_t account_id = get_account_id(to_account.c_str(), to_account.size());
    graphene_assert(account_id >= 0, "目的账户不存在");

    sub_income(amount);
    withdraw_asset(_self, account_id, amount.asset_id, amount.amount);
}

void gxcexchangec::updateconfig(uint64_t id, uint64_t value) {
    uint64_t sender = get_trx_sender();
    authverify(sender);
    update_sysconfig(id, value);
}

void gxcexchangec::init(){
    auto it = sysconfigs.find(profit_account_id_ID);
    graphene_assert(it == sysconfigs.end(), "配置信息已存在,不能进行init");
    
    insert_sysconfig(profit_account_id_ID, profit_account_id_VALUE);
    insert_sysconfig(max_match_order_count_ID, max_match_order_count_VALUE);
    insert_sysconfig(match_amount_times_ID, match_amount_times_VALUE);
    insert_sysconfig(table_save_count_ID, table_save_count_VALUE);
    insert_sysconfig(once_delete_count_ID, once_delete_count_VALUE);
    insert_sysconfig(platform_status_ID, platform_un_lock_status_value);

}

void gxcexchangec::deleteall(){
    uint64_t sender = get_trx_sender();
    authverify(sender);

    // 删除所有得卖单
    for(auto itr = sellorders.begin(); itr != sellorders.end();) {
        unlock_lock_balance(itr->seller, itr->quantity);
        itr = sellorders.erase(itr);
    }

    // 删除所有得买单
    for(auto itr = buyorders.begin(); itr != buyorders.end();) {
        contract_asset buy_asset{ itr->quantity.amount * itr->price, platform_core_asset_id_VALUE};
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


void gxcexchangec::ptdeposite(){
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

void gxcexchangec::ptwithdraw(contract_asset amount) {
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

void gxcexchangec::buyptcoin() {
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

void gxcexchangec::sellptcoin(contract_asset amount) {
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

GRAPHENE_ABI(gxcexchangec, (deposit)(withdraw)(pendingorder)(cancelorder)(fetchprofit)(updateconfig)(init)(deleteall)(ptdeposite)(ptwithdraw)(buyptcoin)(sellptcoin))