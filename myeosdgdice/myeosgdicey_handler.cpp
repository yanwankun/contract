#include <eosiolib/transaction.hpp>
#include "eosio.token.hpp"
# include "myeosgdicey.hpp"

namespace eosio {

    void myeosgdicey::insertval(uint64_t id, uint64_t value) {
        auto it = vardics.find(id);
        eosio_assert(it == vardics.end(), "字典信息已经存在");

        vardics.emplace( _self, [&]( auto& a_var ) {
            a_var.id = id;
            a_var.value = value;
        });
    }

    void myeosgdicey::insertseed(capi_checksum256& hash) {
        seeds.emplace( _self, [&]( auto& a_seed ) {
            a_seed.id = seeds.available_primary_key();
            a_seed.seed_hash = hash;
            // a_seed.seed_value = default_seed_string;
        });
    }

    void myeosgdicey::updateconfig(uint64_t id, uint16_t value) {
        auto it = vardics.find(id);
        eosio_assert(it != vardics.end(), "字典信息不存在");

        if (id == game_status_var_id) {
            eosio_assert(value == 1 || value == 2, "游戏状态类型不支持");
        }

        vardics.modify(it , _self, [&](auto& m_var) {
            m_var.value = value;
        });
    }

    uint64_t myeosgdicey::findlastseedid() {
        // for (auto it = seeds.rbegin() ;it != seeds.rend(); it++) {
        //     if (it->seed_value.compare(default_seed_string) == 0) {
        //         return it->id;
        //     }
        // }
        // 永远不会出现这个情况
        auto it = seeds.rbegin() ;
        return it->id;
        // eosio_assert(false, "最近种子不存在");
    }

    vector<string> split(const string& str, const string& delim) {
        vector<string> res;
        if("" == str) return res;
        //先将要切割的字符串从string类型转换为char*类型
        char * strs = new char[str.length() + 1] ; //不要忘了
        strcpy(strs, str.c_str()); 
    
        char * d = new char[delim.length() + 1];
        strcpy(d, delim.c_str());
    
        char *p = strtok(strs, d);
        while(p) {
            string s = p; //分割得到的字符串转换为string类型
            res.push_back(s); //存入结果数组
            p = strtok(NULL, d);
        }
    
        return res;
    }

    // 这个是一个游戏支付的分流器
    void myeosgdicey::transfer(name from, name to, asset quantity, string memo) {
        if (from == _self || to != _self) {
            return;
        }
        // 如有是游戏下注 bet-bet_num-1-client_seed
        vector<string> res = split(memo, ":");
    
        if (res[0].compare("bet") == 0) {
            // string aduuid = res[1];
            // asset base_bounty = quantity;
            // base_bounty.set_amount(atoll(res[2].c_str()));
            int16_t bet_num = atoi(res[1].c_str());
            bool is_big = atoi(res[2].c_str()) == 1 ? true : false;

            capi_checksum256 client_seed;
            sha256( res[3].c_str(), res[3].size(), &client_seed);

            // capi_checksum256 client_seed(res[3]);

            bet(from, quantity, bet_num, is_big, client_seed);
        }
    }

    /**
     * 插入一条游戏记录
     * 使用延时交易开奖
     */
    void myeosgdicey::bet(name sender, asset bet_asset, int16_t bet_num, bool is_big, capi_checksum256 client_seed) {
        eosio_assert(bet_num >= (is_big ? gt_min_num : st_min_num) && bet_num <= (is_big ? gt_max_num : st_max_num), "猜测数字不合法");
       
        // 添加用户记录
        auto t_player = players.find(sender.value);
        if (t_player == players.end()) {
            players.emplace( _self, [&]( auto& a_player ) {
                a_player.player = sender;
                a_player.betcount = 1;
                a_player.wincount = 0;
                a_player.losscount = 0;
                a_player.bet_asset.push_back(bet_asset);
                // a_player.win_asset;
                // a_player.loss_asset;
            });
        } else {
            int asset_index = 0;
            bool add = false;
            for (auto asset_it = t_player->bet_asset.begin(); asset_it != t_player->bet_asset.end(); ++asset_it) {
                if ((bet_asset.symbol) == asset_it->symbol) {
                    players.modify(t_player, _self, [&](auto &m_player) {
                        m_player.bet_asset[asset_index] += bet_asset;
                        m_player.betcount += 1;
                    });
                    add = true;
                    break;
                }
                asset_index++;
            }
            if (!add) {
                players.modify(t_player, _self, [&](auto &m_player) {
                    m_player.bet_asset.emplace_back(bet_asset);
                    m_player.betcount += 1;
                });
            }
        }

        // 保存用户下注记录
        uint64_t seed_id = findlastseedid();
        uint64_t now = current_time() / time_multiple; 
        uint64_t bet_id = bets.available_primary_key();
        bets.emplace( _self, [&]( auto& a_bet ) {
            a_bet.id = bet_id;
            a_bet.player = sender;
            a_bet.bet_time = now;
            a_bet.bet_asset = bet_asset;
            a_bet.is_big = is_big;
            a_bet.bet_num = bet_num;
            a_bet.seed_id = seed_id;
            a_bet.client_seed = client_seed;
            a_bet.lucky_number = 0;
        });

        eosio::transaction txn{};
        txn.actions.emplace_back(
            eosio::permission_level(_self, name("active")),
            _self,
            name("lottery"),
            std::make_tuple(bet_id)
        );
        txn.delay_sec = 10;
        txn.send(bet_id, _self, false);
    }

    int64_t calulatePayback(int64_t amount, uint16_t rate) {
        int64_t odds = (10000 - platform_cut) * 1000000 / rate; // 赔率
        return amount * odds / 1000000;
    }


    void myeosgdicey::win_asset_pay(name winner, asset win_asset) {
        INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {name("myeosgdicey"), active_permission} },
            { name("myeosgdicey"), winner, win_asset, string("this is a test") }
        );
    }
}