# include "myeosgdicey_handler.cpp"


namespace eosio { 

    /**
     * 初始化第一个种子和设置配置
     */ 
    ACTION myeosgdicey::init(capi_checksum256& hash) {
        require_auth( _self );

        // 插入游戏状态配置
        insertval(game_status_var_id, game_status_play);
        // 插入第一个种子hash
        insertseed(hash);
    }

    /**
     * 更改配置
     */ 
    ACTION myeosgdicey::upconfig(uint64_t id, uint8_t value) {
        require_auth( _self );
        updateconfig(id, value);
    }

    /**
     * 更新种子
     */ 
    ACTION myeosgdicey::upseed(capi_checksum256& seed, capi_checksum256& hash) {
        require_auth( _self );
        
        auto seedid = findlastseedid();
        auto t_seed = seeds.find(seedid);
        
        char *c_src = (char *) &seed;
        assert_sha256(c_src, sizeof(seed), &t_seed->seed_hash);

        seeds.modify(t_seed , _self, [&](auto &m_seed) {
            m_seed.seed_value = seed;
        });

        insertseed(hash);
    }

    ACTION myeosgdicey::lottery(uint64_t bet_id) {
        auto t_bet = bets.find(bet_id);

        eosio_assert(t_bet != bets.end(), "下注记录不存在");
        eosio_assert(t_bet->lucky_number == 0, "已经开过将了");

        auto t_seed = seeds.find(t_bet->seed_id);
        auto last_seed_id = findlastseedid();

        eosio_assert(t_seed != seeds.end() && t_seed->id < last_seed_id , "种子还未上传");

        auto t_player = players.find(t_bet->player.value);
        eosio_assert(t_player != players.end(), "用户数据不存在");

        // 最终种子 = 服务端种子 + 客户端种子+ 投注人+ 投注次数 + 时间
        uint64_t now = current_time();
        // string final_seed_string = "";
        // char *c_str_server = (char *)t_seed->seed_value;
        // char *c_str_client = (char *)t_bet->client_seed;
        // final_seed_string.append(t_bet->player.value).append(to_string(t_player->betcount)).append(to_string(now));
        // char *c_str = final_seed_string.data();
        
        int lucky_number = (int)(
                        t_seed->seed_value.hash[0] + 
                        t_bet->client_seed.hash[1] + 
                        t_seed->seed_value.hash[2] + 
                        t_bet->client_seed.hash[3] + 
                        t_seed->seed_value.hash[4] + 
                        t_bet->client_seed.hash[5]) % 100 + 1;

       
        bool is_win = t_bet->is_big ? lucky_number > t_bet->bet_num : lucky_number < t_bet->bet_num;
        if (is_win) {
            // asset win_asset{0, asset.asset_id};
            int64_t win_amount = calulatePayback(t_bet->bet_asset.amount, t_bet->is_big ? t_bet->bet_num : (10000 - t_bet->bet_num));
            asset win_asset = t_bet->bet_asset; 
            win_asset.set_amount(win_amount);

            bets.modify(t_bet, _self, [&](auto &m_bet) {
                m_bet.lucky_number = lucky_number;
                m_bet.server_seed = t_seed->seed_value;
                m_bet.win_asset = win_asset;
                m_bet.lottery_t_micros = now;
            });

            int asset_index = 0;
            bool add = false;
            for (auto asset_it = t_player->win_asset.begin(); asset_it != t_player->win_asset.end(); ++asset_it) {
                if ((win_asset.symbol) == asset_it->symbol) {
                    players.modify(t_player, _self, [&](auto &m_player) {
                        m_player.win_asset[asset_index] += win_asset;
                        m_player.wincount += 1;
                    });
                    add = true;
                    break;
                }
                asset_index++;
            }
            if (!add) {
                players.modify(t_player, _self, [&](auto &m_player) {
                    m_player.bet_asset.emplace_back(win_asset);
                    m_player.wincount += 1;
                });
            }

            win_asset_pay(t_bet->player, win_asset);
        } else {
            players.modify(t_player, _self, [&](auto &m_player) {
                m_player.losscount += 1;
            });
            bets.modify(t_bet, _self, [&](auto &m_bet) {
                m_bet.lucky_number = lucky_number;
                m_bet.server_seed = t_seed->seed_value;
                m_bet.lottery_t_micros = now;
            });
        }
    }

    ACTION myeosgdicey::deleteall() {
        for(auto itr = players.begin(); itr != players.end();) {
            itr = players.erase(itr);
        }
        for(auto itr = bets.begin(); itr != bets.end();) {
            itr = bets.erase(itr);
        }
        for(auto itr = vardics.begin(); itr != vardics.end();) {
            itr = vardics.erase(itr);
        }
        for(auto itr = seeds.begin(); itr != seeds.end();) {
            itr = seeds.erase(itr);
        }
    }

}