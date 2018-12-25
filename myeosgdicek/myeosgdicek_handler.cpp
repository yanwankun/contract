#include <eosiolib/transaction.hpp>
#include "eosio.token.hpp"
# include "myeosgdicek.hpp"

namespace eosio {

    void myeosgdicek::insertval(uint64_t id, uint64_t value) {
        auto it = vardics.find(id);
        eosio_assert(it == vardics.end(), "字典信息已经存在");

        vardics.emplace( _self, [&]( auto& a_var ) {
            a_var.id = id;
            a_var.value = value;
        });
    }

    uint64_t myeosgdicek::getval(uint64_t id) {
        auto it = vardics.find(id);
        eosio_assert(it != vardics.end(), "字典信息不存在");

        return it->value;
    }

    void myeosgdicek::insertseed(capi_checksum256& hash) {
        seeds.emplace( _self, [&]( auto& a_seed ) {
            a_seed.id = seeds.available_primary_key();
            a_seed.seed_hash = hash;
        });
    }

    void myeosgdicek::updateconfig(uint64_t id, uint16_t value) {
        auto it = vardics.find(id);
        if (it != vardics.end()) {
            insertval(id, value);
        }

        if (id == GAME_STATUS_VAR_ID) {
            eosio_assert(value == GAME_STATUS_LOCK || value == GAME_STATUS_PLAY, "游戏状态类型不支持");
        }

        vardics.modify(it , _self, [&](auto& m_var) {
            m_var.value = value;
        });
    }

    uint64_t myeosgdicek::findlastseedid() {
        auto it = seeds.rbegin() ;
        return it->id;
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
    void myeosgdicek::transfer(name from, name to, asset quantity, string memo) {
        if (from == _self || to != _self) {
            return;
        }
        vector<string> res = split(memo, ":");
    
        if (res[0].compare("bet") == 0) {
            eosio_assert(!gamePaused(), "game has been paused");
            int16_t bet_num = atoi(res[1].c_str());
            bool is_big = atoi(res[2].c_str()) == 1 ? true : false;

            capi_checksum256 client_seed;
            sha256( res[3].c_str(), res[3].size(), &client_seed);
            bet(from, quantity, bet_num, is_big, client_seed);
        } else if (res[0].compare("invest") == 0) {
            invest(from, quantity);
        } else if (res[0].compare("pledge") == 0) {
            pledge(from, quantity);  
        } else if (res[0].compare("updatepool") == 0) {
            updatepool(quantity, 100000, 1000000);
        }
    }

    void myeosgdicek::pledge(name sender, asset quantity) { // 质押gdc
        eosio_assert(quantity.symbol == GDC_ASSET_ID, "only GDC allowed.");
        auto p = players.find(sender.value);
        if (p == players.end()) {
            players.emplace(_self, [&](auto &o) {
                o.player = sender;
                o.gdcamount = quantity.amount;
            });
        } else {
            players.modify(p, _self, [&](auto &o) {
                o.gdcamount += quantity.amount;
            });
        }

        increaseVar(GDC_TOTAL_ID, quantity.amount);
    }

    void myeosgdicek::invest(name sender, asset quantity) { // 投资坐庄
        if (players.find(sender.value) == players.end()) {
            players.emplace( _self, [&]( auto& a_player ) {
                a_player.player = sender;
                // a_player.betcount = 0;
                // a_player.wincount = 0;
                // a_player.losscount = 0;
            });
        }
        auto p = players.find(sender.value);
        auto pp = prizepools.find(quantity.symbol.raw());
        eosio_assert(pp != prizepools.end(), "no pool for that asset");
        eosio_assert(quantity.amount >= pp->minbank, "invest amount too small");

        int64_t percentNum = 0;
        if (pp->investtotalpercent == 0) {
            percentNum = INVEST_ZOOM_UP;
        } else {
            int64_t percent = quantity.amount * INVEST_ZOOM_UP / (pp->pool.amount + quantity.amount);
            percentNum = percent * pp->investtotalpercent / (INVEST_ZOOM_UP - percent);
        }

        asset assetPercentInfo = quantity;
        assetPercentInfo.set_amount(percentNum);

        // 这里确保 player的investamount/divestamount/investpercent同时添加，index保持一致
        uint16_t assetIndex = std::distance(p->investamount.begin(), find_if(p->investamount.begin(), p->investamount.end(), [&](const auto &a) {
            return a.symbol.raw() == quantity.symbol.raw();
        }));
        if (assetIndex < (p->investamount.size())) {
            players.modify(p, _self, [&](auto &o){
                o.investamount[assetIndex] += quantity;
                o.investpercent[assetIndex] += assetPercentInfo;
            });
        } else {
            asset divAssetInfo = quantity;
            divAssetInfo.set_amount(percentNum);
            players.modify(p, _self, [&](auto &o){
                o.investamount.emplace_back(quantity);
                o.divestamount.emplace_back(divAssetInfo);
                o.investpercent.emplace_back(assetPercentInfo);
            });
        }
        
        prizepools.modify(pp, _self, [&](auto &o) {
            o.pool += quantity;
            o.investtotalpercent += percentNum;
        });
    }

    void myeosgdicek::updatepool(asset assetInfo, uint64_t minBet, uint64_t minBank) {
        require_auth( _self );
        
        bool exist = false;
        for (auto it = prizepools.begin(); it != prizepools.end(); it++) {
            if (it->pool.symbol == assetInfo.symbol) {
                exist = true;
            }
        }

        if (!exist) {
            assetInfo.amount = 1;
            prizepools.emplace(_self, [&](auto &o) {
                o.id = assetInfo.symbol.raw();
                o.pool = assetInfo;
                o.totalbet = 0;
                o.betcount = 0;
                o.wincount = 0;
                o.minbet = minBet;
                o.minbank = minBank;
            });
        } else {
            auto pp = prizepools.find(assetInfo.symbol.raw());
            eosio_assert(pp != prizepools.end(), "no pool for that asset");
            prizepools.modify(pp, _self, [&](auto &o) {
                o.minbet = minBet;
                o.minbank = minBank;
            });
        }
    }

    /**
     * 插入一条游戏记录
     * 使用延时交易开奖
     */
    void myeosgdicek::bet(name sender, asset bet_asset, int16_t bet_num, bool is_big, capi_checksum256 client_seed) {
        eosio_assert(bet_num >= (is_big ? GT_MIN_NUM : ST_MIN_NUM) && bet_num <= (is_big ? GT_MAX_NUM : ST_MAX_NUM), "猜测数字不合法");

        // 最小下注金额判断
        // todo

        auto pp = prizepools.find(bet_asset.symbol.raw());
        eosio_assert(pp != prizepools.end(), "no pool for that asset found");
        eosio_assert(bet_asset.amount >= pp->minbet, "invalid bet amount");
        uint64_t maxLimit = pp->pool.amount / MAX_WIN_LIMIT_RATE;
        int64_t winPayback = calulatePayback(bet_asset.amount, is_big ? (100 - bet_num) : bet_num);
        eosio_assert(winPayback <= maxLimit, "bet amount too large");

        // 实际进入池子的资金
        int64_t intoPool = bet_asset.amount * (10000 - PLATFORM_CUT) / 10000;
        asset intoPoolAsset = bet_asset;
        intoPoolAsset.set_amount(intoPool);
        prizepools.modify(pp, _self, [&](auto &o) {
            o.pool += intoPoolAsset;
            o.totalbet += bet_asset.amount;
            o.betcount += 1;
            o.profit += (bet_asset.amount - intoPool);
        });
       
        // 添加用户记录
        auto t_player = players.find(sender.value);
        if (t_player == players.end()) {
            players.emplace( _self, [&]( auto& a_player ) {
                a_player.player = sender;
                a_player.betcount = 1;
                a_player.wincount = 0;
                a_player.losscount = 0;
                a_player.bet_asset.push_back(bet_asset);
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
        uint64_t now = current_time() / TIME_MULTIPLE; 
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

    int64_t myeosgdicek::calulatePayback(int64_t amount, uint16_t rate) {
        int64_t odds = (10000 - PLATFORM_CUT) * 1000000 / rate; // 赔率
        return amount * odds / 1000000;
    }


    void myeosgdicek::win_asset_pay(name winner, asset win_asset) {
        INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {name("myeosgdicek"), active_permission} },
            { name("myeosgdicek"), winner, win_asset, string("this is a test") }
        );
    }

    void myeosgdicek::reward_transfer(asset bet_asset, name to) {
        if (EOS_ASSET_ID == bet_asset.symbol) {
            int64_t reward_amount = bet_asset.amount * REWARD_RATE / 100;
            asset reward_asset{reward_amount, GDC_ASSET_ID};
            string memo = "play myeosgdicek reward ";
            memo.append(to_string(reward_amount)).append(", I wish you a happy game and win the victory.");
            withdraw_asset(to, reward_asset, memo);
        }
    }

    void myeosgdicek::withdraw_asset(name to, asset quantity, string memo) {
        INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {name("myeosgdicek"), active_permission} },
            { name("myeosgdicek"), to, quantity, memo }
        );
    }

    bool myeosgdicek::gamePaused() {
        return getval(GAME_STATUS_VAR_ID) == GAME_STATUS_LOCK;
    }

    void myeosgdicek::increaseVar(uint64_t id, int64_t delta) {
        auto vi = vardics.find(id);
        eosio_assert(vi != vardics.end(), "var not find");
        vardics.modify(vi, _self, [&](auto &g) {
            g.value += delta;
        });
    }

    void myeosgdicek::decreaseVar(uint64_t id, int64_t delta) {
        auto vi = vardics.find(id);
        eosio_assert(vi != vardics.end(), "var not find");
        vardics.modify(vi, _self, [&](auto &g) {
            g.value -= delta;
        });
    }
}