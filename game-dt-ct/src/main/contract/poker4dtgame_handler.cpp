#include <eosiolib/transaction.hpp>
#include "eosio.token.hpp"
#include "poker4dtoken.hpp"
# include "poker4dtgame.hpp"

namespace eosio {

    /**
     * 插入配置值
     */  
    void poker4dtgame::insertval(uint64_t id, uint16_t value, string desc) {
        auto it = vardics.find(id);
        eosio_assert(it == vardics.end(), "Dictionary information already exists");

        vardics.emplace( _self, [&]( auto& a_var ) {
            a_var.id = id;
            a_var.value = value;
            a_var.desc = desc;
        });
    }

    /**
     * 获取配置值
     */
    uint64_t poker4dtgame::getval(uint64_t id) {
        auto it = vardics.find(id);
        eosio_assert(it != vardics.end(), "Dictionary information does not exist");

        return it->value;
    }

     bool poker4dtgame::have_bet(uint64_t game_id) {
        auto itr = bets.begin();
        if (itr != bets.end()) {
            if (itr->game_id == game_id) {
                return true;
            }
        }
        return false;
    }

    string poker4dtgame::get_card_desc(uint8_t card_num) {
        string card_t_color = card_color[cards[card_num]/100];
        string card_t_value = card_value[cards[card_num]%100];
        return card_t_color.append(" ").append(card_t_value);
    }

    /**
     * 插入游戏
     */ 
    uint64_t poker4dtgame::insertgame(capi_checksum256& seed_hash) {
        uint64_t now = current_time() / TIME_MULTIPLE; 
        uint64_t game_id = getval(GAME_NEXT_ID_VAR_ID);
        uint64_t bet_time = getval(GAME_BET_TIME_ID_VAR_ID);
        uint64_t lock_time = getval(GAME_LOCK_TIME_ID);

        games.emplace( _self, [&]( auto& a_game ) {
            a_game.id = game_id;
            a_game.game_status = PROCESSING;
            a_game.bet_start_id = bets.available_primary_key();
            a_game.seed_hash = seed_hash;
            a_game.create_time = now;
            a_game.bet_end_time = now + bet_time;
            a_game.reveal_time = now + bet_time + lock_time;
        });

        increaseVar(GAME_NEXT_ID_VAR_ID, 1);

        //  如果记录达到一定条数就删除一些
        uint64_t once_delete_count = getval(ONCE_DELETE_COUNT_ID);
        if (game_id > getval(TABLE_SAVE_COUNT_ID) && (game_id-1) % once_delete_count == 0) {
            uint64_t delete_count = 0;
            for( auto itr = games.begin(); itr != games.end(); ) {
                itr = games.erase(itr);
                delete_count++;
                if (delete_count >= once_delete_count) {
                    break;
                }
            }
        }
        return game_id;
    }

   /**
     * 更新配置
     */ 
    void poker4dtgame::updateconfig(uint64_t id, uint16_t value, string desc) {
        auto it = vardics.find(id);
        if (it == vardics.end()) {
            insertval(id, value, desc);
        }

        if (id == GAME_STATUS_VAR_ID) {
            eosio_assert(value == GAME_STATUS_LOCK || value == GAME_STATUS_PLAY, "Game status type is not supported");
        }

        vardics.modify(it , _self, [&](auto& m_var) {
            m_var.value = value;
            m_var.desc = desc;
        });
    }

    /**
     * 分割字符串
     */ 
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
    void poker4dtgame::transfer(name from, name to, asset quantity, string memo) {
        if (from == _self || to != _self) {
            return;
        }
        extended_asset ext_quantity{quantity, token_account};
        vector<string> res = split(memo, ":");
    
        if (res[0].compare("gamebet") == 0) { // 下注
            uint64_t game_id = atoi(res[1].c_str());
            uint8_t bet_type = atoi(res[2].c_str());
            gamebet(from, game_id, ext_quantity, bet_type, res[3]);
        } else if (res[0].compare("invest") == 0) { // 坐庄
            gameinvest(from, ext_quantity);
        }
    }

    // 这个是一个游戏支付的分流器
    void poker4dtgame::metransfer(name from, name to, asset quantity, string memo) {
        if (from == _self || to != _self) {
            return;
        }
        extended_asset ext_quantity{quantity, myeostoken_account};
        vector<string> res = split(memo, ":");
    
        if (res[0].compare("gamebet") == 0) {  // 下注
            uint64_t game_id = atoi(res[1].c_str());
            uint8_t bet_type = atoi(res[2].c_str());
            gamebet(from, game_id, ext_quantity, bet_type, res[3]);
        } else if (res[0].compare("pledge") == 0) { // 质押
            pledge(from, ext_quantity);
        } else if (res[0].compare("invest") == 0) { // 坐庄
            gameinvest(from, ext_quantity);
        }
    }

    /**
     * 投资坐庄
     * add : 增加先期投资分红优势，后期投资者会把投资得一部分分给先前得投资者
     */ 
    void poker4dtgame::gameinvest(name sender, extended_asset quantity) { // 投资坐庄
        if (players.find(sender.value) == players.end()) {
            players.emplace( _self, [&]( auto& a_player ) {
                a_player.player = sender;
            });
        }
        auto t_player = players.find(sender.value);

        auto t_pool = prizepools.find(quantity.quantity.symbol.raw());
        eosio_assert(t_pool != prizepools.end(), "no pool for that asset");
        eosio_assert(quantity.contract == t_pool->all_asset.contract, "invest asset is not pool asset");
        eosio_assert(quantity.quantity.amount >= t_pool->min_bank, "invest amount too small");

        int64_t invest_num = 0; // 新投资人的票数
        extended_asset inverst_asset = quantity;
        extended_asset fh_asset = quantity;
        if (t_pool->invest_num == 0) {
            invest_num = MIN_INVERT_NUM;
        } else {
            uint64_t inverst_dilution_amount = getval(INVESTMENT_DILUTION_RATIO_ID) * quantity.quantity.amount / 1000;
            extended_asset fh_asset = quantity;
            for (auto itr = invests.begin(); itr != invests.end();itr++) {
                if (itr->pool_id == t_pool->id && itr->invest_num > 0) {
                    uint64_t inverst_fh_amount = inverst_dilution_amount * itr->invest_num / t_pool->invest_num;
                    fh_asset.quantity.amount = inverst_fh_amount;
                    win_asset_pay(itr->player, fh_asset, "new investment income, welcome you to invest again in our game");
                }
            }
            uint64_t inverst_amount = quantity.quantity.amount - inverst_dilution_amount;
            inverst_asset.quantity.amount = inverst_amount;
            invest_num = (inverst_amount * t_pool->invest_num / t_pool->all_asset.quantity.amount);
        }

        prizepools.modify(t_pool, _self, [&](auto &m_pool) {
            m_pool.all_asset += inverst_asset;
            m_pool.invest_num += invest_num;
        });

        // 增加用户的投资记录
        uint16_t assetIndex = std::distance(t_player->invests.begin(), find_if(t_player->invests.begin(), t_player->invests.end(), [&](const auto &a) {
            return a.quantity.symbol.raw() == quantity.quantity.symbol.raw();
        }));
        if (assetIndex < (t_player->invests.size())) {
            players.modify(t_player, _self, [&](auto &m_player){
                m_player.invests[assetIndex] += quantity;
                m_player.invest_nums[assetIndex] += invest_num;
            });
        } else {
            extended_asset divest_asset = quantity;
            divest_asset.quantity.amount = 0; 
            players.modify(t_player, _self, [&](auto &m_player){
                m_player.invests.emplace_back(quantity);
                m_player.divests.emplace_back(divest_asset);
                m_player.invest_nums.emplace_back(invest_num);
            });
        }
        
        // 如果以前投资过就更新投资记录
        for (auto itr = invests.begin(); itr != invests.end();itr++) {
            if (itr->pool_id == t_pool->id && itr->player == sender) {
                invests.modify(itr, _self, [&](auto &m_inverst){
                    m_inverst.invest_num += invest_num;
                });
                return;
            }
        }    

        // 如果以前没有投资过，插入投资记录
        uint64_t invest_id = invests.available_primary_key();
        invests.emplace( _self, [&]( auto& a_invest ) {
            a_invest.id = invest_id;
            a_invest.pool_id = t_pool->id;
            a_invest.player = sender;
            a_invest.invest_num = invest_num;
        }); 
    }

    // void poker4dtgame::verify_bet_amount(extended_asset bet_asset) {
    //     if (bet_asset.contract == token_account) {
    //         eosio_assert(bet_asset.quantity.amount >= 1000 && bet_asset.quantity.amount <= 400000, "bet amount is between 0.1 EOS and 40 EOS");
    //     } else if (bet_asset.contract == myeostoken_account) {
    //         eosio_assert(bet_asset.quantity.amount >= 20000 && bet_asset.quantity.amount <= 10000000, "bet amount is between 2 DTC and 1000 DTC");
    //     }
    // }

    void poker4dtgame::gamebet(name sender, uint64_t game_id, extended_asset bet_asset, int8_t bet_type, string bet_random) {
        // 添加用户记录
        eosio_assert(bet_random.length() <= 8, "User random factor cannot exceed 8 characters");
        uint64_t now = current_time() / TIME_MULTIPLE; 
        auto t_game = games.find(game_id);
        eosio_assert(t_game != games.end(), "Game data does not exist");
        eosio_assert(now < t_game->bet_end_time, "The game bet time is over, please wait for the next bet");

        auto t_pool = prizepools.find(bet_asset.quantity.symbol.raw());
        eosio_assert( t_pool != prizepools.end(), "Reward pool does not exist");
        eosio_assert(t_pool->min_bet <= bet_asset.quantity.amount && t_pool->max_bet >= bet_asset.quantity.amount, "The bet amount is not within the allowable range");

        auto t_player = players.find(sender.value);
        if (t_player == players.end()) {
            players.emplace( _self, [&]( auto& a_player ) {
                a_player.player = sender;
                a_player.bet_count = 1;
                a_player.win_count = 0;
                a_player.loss_count = 0;
                a_player.bet_asset.emplace_back(bet_asset);
            });
        } else {
            int asset_index = 0;
            bool add = false;
            for (auto asset_it = t_player->bet_asset.begin(); asset_it != t_player->bet_asset.end(); ++asset_it) {
                if ((bet_asset.quantity.symbol) == asset_it->quantity.symbol) {
                    players.modify(t_player, _self, [&](auto &m_player) {
                        m_player.bet_asset[asset_index] += bet_asset;
                        m_player.bet_count += 1;
                    });
                    add = true;
                    break;
                }
                asset_index++;
            }
            if (!add) {
                players.modify(t_player, _self, [&](auto &m_player) {
                    m_player.bet_asset.emplace_back(bet_asset);
                    m_player.bet_count += 1;
                });
            }
        }

        // 实际进入池子的资金
        int64_t intoPool = bet_asset.quantity.amount * (10000 - PLATFORM_CUT) / 10000;
        extended_asset intoPoolAsset = bet_asset;
        intoPoolAsset.quantity.set_amount(intoPool);
        prizepools.modify(t_pool, _self, [&](auto &m_pool) {
            m_pool.all_asset += intoPoolAsset;
            m_pool.total_bet += bet_asset.quantity.amount;
            m_pool.bet_count += 1;
            m_pool.profit += bet_asset.quantity.amount - intoPool;
        });

        // 保存用户下注记录
        uint64_t bet_id = getval(BET_NEXT_ID_VAR_ID);
        increaseVar(BET_NEXT_ID_VAR_ID, 1);
        bets.emplace( _self, [&]( auto& a_bet ) {
            a_bet.id = bet_id;
            a_bet.player = sender;
            a_bet.bet_time = now;
            a_bet.bet_asset = bet_asset;
            a_bet.game_id = game_id;
            a_bet.bet_type = bet_type;
            a_bet.bet_random = bet_random;
            a_bet.status = BET_STATUS_NOT_AWARDED;
        });

        // 更改游戏随机数
        string n_random_str = t_game->player_source;
        if (n_random_str.length() < 32) {
            uint8_t length = 32 - n_random_str.length() >= bet_random.length() ? bet_random.length() : 32 - n_random_str.length();
            n_random_str.append(bet_random.substr(0, length));
            games.modify(t_game, _self, [&](auto &m_game) {
                m_game.player_source = n_random_str;
            });
        } else {
            capi_checksum256 t_random_seed;
            string random_str = bet_random;
            random_str.append(to_string(sender.value)).append(to_string(now));
            sha256( random_str.c_str(), random_str.size(), &t_random_seed);
            int number1 = (int)(t_random_seed.hash[0] + t_random_seed.hash[1] + t_random_seed.hash[2] + t_random_seed.hash[3] + t_random_seed.hash[4] + t_random_seed.hash[5] + t_random_seed.hash[6]);
            if (number1%100 < 40) {
                n_random_str = n_random_str.substr(2, n_random_str.length() - 2);
                n_random_str.append(bet_random.substr(0,2));
                games.modify(t_game, _self, [&](auto &m_game) {
                    m_game.player_source = n_random_str;
                });
            }
        }

        INLINE_ACTION_SENDER(eosio::poker4dtgame, gamebetlog)(
            contract_account_name, { {contract_account_name, active_permission} },
            { bet_id, sender, game_id, bet_asset, bet_type_desc[bet_type], bet_random }
        );
        reward_transfer(bet_asset, sender);
    }

    /**
     * 下注获胜奖金支付
     */ 
    void poker4dtgame::win_asset_pay(name winner, extended_asset quantity, string memo) {
        if (quantity.contract == token_account) {
            INLINE_ACTION_SENDER(eosio::token, transfer)(
                token_account, { {contract_account_name, active_permission} },
                { contract_account_name, winner, quantity.quantity, memo }
            );
        } else if (quantity.contract == myeostoken_account) {
            INLINE_ACTION_SENDER(eosio::poker4dtoken, transfer)(
                myeostoken_account, { {contract_account_name, active_permission} },
                { contract_account_name, winner, quantity.quantity, memo }
            );
        }
    }

    /**
     * 奖励得代币发送
     */ 
    void poker4dtgame::reward_transfer(extended_asset bet_asset, name to) {
        if (EOS_ASSET_ID == bet_asset.quantity.symbol) {
            int64_t reward_amount = bet_asset.quantity.amount * getval(REWARD_RATE_ID_VAR_ID) / 100;
            asset asset{reward_amount, DTC_ASSET_ID};
            extended_asset reward_asset{asset, myeostoken_account};
            string memo = "play game reward ";
            memo.append(to_string(reward_amount)).append(", I wish you a happy game and win the victory.");
            withdraw_asset(to, reward_asset, memo);
        }
    }

    /**
     * 提现资产
     */ 
    void poker4dtgame::withdraw_asset(name to, extended_asset quantity, string memo) {
        if (quantity.contract == token_account) {
            INLINE_ACTION_SENDER(eosio::token, transfer)(
                token_account, { {contract_account_name, active_permission} },
                { contract_account_name, to, quantity.quantity, memo }
            );
        } else if (quantity.contract == myeostoken_account) {
            INLINE_ACTION_SENDER(eosio::poker4dtoken, transfer)(
                myeostoken_account, { {contract_account_name, active_permission} },
                { contract_account_name, to, quantity.quantity, memo }
            );
        }
    }

    /**
     * 自动增加变量值
     */ 
    void poker4dtgame::increaseVar(uint64_t id, int64_t delta) {
        auto vi = vardics.find(id);
        eosio_assert(vi != vardics.end(), "var not find");
        vardics.modify(vi, _self, [&](auto &g) {
            g.value += delta;
        });
    }

    /**
     * 自动减少变量值
     */ 
    void poker4dtgame::decreaseVar(uint64_t id, int64_t delta) {
        auto vi = vardics.find(id);
        eosio_assert(vi != vardics.end(), "var not find");
        vardics.modify(vi, _self, [&](auto &g) {
            g.value -= delta;
        });
    }

    /**
     * 质押DTC
     */ 
    void poker4dtgame::pledge(name sender, extended_asset quantity) { // 质押DTC
        eosio_assert(quantity.quantity.symbol == DTC_ASSET_ID, "only DTC allowed.");
        auto t_player = players.find(sender.value);
        if (t_player == players.end()) {
            players.emplace(_self, [&](auto &a_player) {
                a_player.player = sender;
                a_player.dtc_amount = quantity.quantity.amount;
            });
        } else {
            players.modify(t_player, _self, [&](auto &m_player) {
                m_player.dtc_amount += quantity.quantity.amount;
            });
        }

        increaseVar(DTC_TOTAL_ID, quantity.quantity.amount);
    }

    /**
     * 判断游戏平台状态
     */ 
    void poker4dtgame::verifystatus() {
        eosio_assert(getval(GAME_STATUS_VAR_ID) == GAME_STATUS_PLAY, "plateform is lock");
    }

    /**
     * 根据牌获取赢得结果列表
     */ 
    poker4dtgame::MAP_RESULT poker4dtgame::getWinsMap(uint8_t dragon_value, const uint8_t tiger_value) {
        MAP_RESULT map;
        int8_t dragon_value_v = cards[dragon_value] % 100;
        int8_t dragon_color = cards[dragon_value] / 100;
        int8_t tiger_value_v = cards[tiger_value] % 100;
        int8_t tiger_color = cards[tiger_value] / 100;

        // 判断输赢 和 一样
        if (dragon_value_v > tiger_value_v) {
            // 龙大
            map[DRAGON_WIN] = true;
        } else if (dragon_value_v == tiger_value_v) {
            // 一样大
            map[SAME] = true;
        } else {
            map[TIGER_WIN] = true;
        }

        // 判断颜色
        if (dragon_color == 1 || dragon_color == 3) {
            map[DRAGON_BLACK] = true;
        } else {
            map[DRAGON_RED] = true;
        }

        if (tiger_color == 1 || tiger_color == 3) {
            map[TIGER_BLACK] = true;
        } else {
            map[TIGER_RED] = true;
        }

        // 判断单双
        if (dragon_value_v % 2 == 0) {
            map[DRAGON_ODD] = true;
        } else {
            map[DRAGON_EVEN] = true;
        }

        if (tiger_value_v % 2 == 0) {
            map[TIGER_ODD] = true;
        } else {
            map[TIGER_EVEN] = true;
        }

        return map;
    }
}