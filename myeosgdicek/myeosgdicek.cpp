# include "myeosgdicek_handler.cpp"


namespace eosio { 

    /**
     * 初始化第一个种子和设置配置
     */ 
    ACTION myeosgdicek::init(capi_checksum256& hash) {
        require_auth( _self );
        // 插入游戏状态配置
        insertval(GAME_STATUS_VAR_ID, GAME_STATUS_PLAY);
        insertval(BET_NMBER_ID, 0);
        insertval(GDC_TOTAL_ID, 0);
        // 插入第一个种子hash
        insertseed(hash);
    }

    ACTION myeosgdicek::updatepool(asset assetInfo, uint64_t minBet, uint64_t minBank) {
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

    ACTION myeosgdicek::divest(name sender, asset assetInfo) {// 撤资
        eosio_assert(!gamePaused(), "game has been paused");
        eosio_assert(assetInfo.amount > 0, "invalid withdraw amount");
        
        auto p = players.find(sender.value);
        eosio_assert(p != players.end(), "player has no asset in this contract");
        auto pp = prizepools.find(assetInfo.symbol.raw());
        eosio_assert(pp != prizepools.end(), "no pool for that asset");

        int assetIndex = 0;
        for (auto playerAsset = p->investpercent.begin(); playerAsset != p->investpercent.end(); ++playerAsset) {
            if (assetInfo.symbol == playerAsset->symbol) {
                uint64_t playerPercent = (playerAsset->amount) * INVEST_ZOOM_UP / pp->investtotalpercent;
                uint64_t trueAmount = playerPercent * (pp->pool.amount) / INVEST_ZOOM_UP;
                eosio_assert(trueAmount >= assetInfo.amount, "no enough balance");
                
                uint64_t percentNum = assetInfo.amount * pp->investtotalpercent / pp->pool.amount;
                if (trueAmount == assetInfo.amount) { // 全部取出
                    players.modify(p, _self, [&](auto &o) {
                        o.investpercent[assetIndex].amount = 0;
                        o.divestamount[assetIndex].amount += trueAmount;
                    });
                } else {
                    percentNum += ((assetInfo.amount * pp->investtotalpercent) % pp->pool.amount > 0 ? 1 : 0);
                    players.modify(p, _self, [&](auto &o) {
                        o.investpercent[assetIndex].amount -= percentNum;
                        o.divestamount[assetIndex].amount += trueAmount;
                    });
                }
                prizepools.modify(pp, _self, [&](auto &o) {
                    o.pool -= assetInfo;
                    o.investtotalpercent -= percentNum;
                });
                withdraw_asset(token_account, sender, assetInfo, "");
                break;
            }
            assetIndex++;
        }
    }

    ACTION myeosgdicek::redeem(name sender, int64_t amount) { // 赎回GDC
        require_auth( _self );
        
        auto p = players.find(sender.value);
        eosio_assert(p != players.end(), "no user found");
        eosio_assert(p->gdcamount >= amount, "no enough Pledged GDC.");
        players.modify(p, _self, [&](auto &o) {
            o.gdcamount -= amount;
        });
        decreaseVar(GDC_TOTAL_ID, amount);

        asset redeemAsset{amount, GDC_ASSET_ID};
        withdraw_asset(token_account, p->player, redeemAsset, "redeem gdc");
    }

    /**
     * 更改配置
     */ 
    ACTION myeosgdicek::upconfig(uint64_t id, uint8_t value) {
        require_auth( _self );
        updateconfig(id, value);
    }

    /**
     * 更新种子
     */ 
    ACTION myeosgdicek::upseed(capi_checksum256& seed, capi_checksum256& hash) {
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

    ACTION myeosgdicek::lottery(uint64_t bet_id) {
        auto t_bet = bets.find(bet_id);

        eosio_assert(t_bet != bets.end(), "下注记录不存在");
        eosio_assert(t_bet->lucky_number == 0, "已经开过将了");

        auto t_seed = seeds.find(t_bet->seed_id);
        auto last_seed_id = findlastseedid();

        eosio_assert(t_seed != seeds.end() && t_seed->id < last_seed_id , "种子还未上传");

        auto t_player = players.find(t_bet->player.value);
        eosio_assert(t_player != players.end(), "用户数据不存在");

        uint64_t now = current_time();

        string final_seed_string = "";
        final_seed_string.append(to_string(t_bet->player.value)).append(to_string(t_player->betcount)).append(to_string(now));

        capi_checksum256 t_info_seed;
        sha256( final_seed_string.c_str(), final_seed_string.size(), &t_info_seed);

        int number1 = (int)(
                        t_info_seed.hash[0] + 
                        t_info_seed.hash[1] + 
                        t_info_seed.hash[2] + 
                        t_info_seed.hash[3] + 
                        t_info_seed.hash[4] + 
                        t_info_seed.hash[5]);
        
        int number2 = (int)(
                        t_seed->seed_value.hash[0] + 
                        t_bet->client_seed.hash[1] + 
                        t_seed->seed_value.hash[2] + 
                        t_bet->client_seed.hash[3] + 
                        t_seed->seed_value.hash[4] + 
                        t_bet->client_seed.hash[5]);

        int lucky_number = (number1 + number2) % 100 + 1;

       
        bool is_win = t_bet->is_big ? lucky_number > t_bet->bet_num : lucky_number < t_bet->bet_num;
        if (is_win) {
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

        // 奖励发放
        reward_transfer(t_bet->bet_asset, t_bet->player);
    }

    ACTION myeosgdicek::deleteall() {
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
        for(auto itr = prizepools.begin(); itr != prizepools.end();) {
            itr = prizepools.erase(itr);
        }
    }

}