# include "poker4dtgame_handler.cpp"


namespace eosio { 

    /**
     * 插入配置信息
     */ 
    ACTION poker4dtgame::init() {
        require_auth( _self );
        // 插入游戏状态配置
        insertval(GAME_STATUS_VAR_ID, GAME_STATUS_PLAY, "game status"); // 游戏状态
        insertval(GAME_LOCK_TIME_ID, 8, "lottery delay time"); // 游戏锁定时间
        insertval(DTC_TOTAL_ID, 0, "total dtc count"); // 总质押gdc得数量
        insertval(TABLE_SAVE_COUNT_ID, 20, "the max number of table to save"); // 表保存得最大数量
        insertval(ONCE_DELETE_COUNT_ID, 10, "the number of row once delete"); // 每次删除表得数量
        insertval(GAME_NEXT_ID_VAR_ID, 0, "next game id"); // 下次得游戏id
        insertval(BET_NEXT_ID_VAR_ID, 0, "next bet id"); // 下次下注得id
        insertval(INVESTMENT_DILUTION_RATIO_ID, 20, "Investment dilution ratio"); // 投资稀释比列
        insertval(REWARD_RATE_ID_VAR_ID, 5000, "gdc reward percent"); // gdc 奖励百分比
        insertval(REDEEM_DELAY_ID_VAR_ID, 3600, "Redemption delay"); // 赎回延时
        insertval(DIVEST_DELAY_ID_VAR_ID, 60, "divest delay"); // 撤资延时 
        insertval(GAME_BET_TIME_ID_VAR_ID, 110, "game bet time"); // 游戏下注时间得长度 
        insertval(MAX_WIN_LIMIT_RATE_ID_VAR_ID, 50, "Maximum win pool percentage"); // 最大可赢奖池百分比
    }

    /**
     * 更改配置
     */ 
    ACTION poker4dtgame::upconfig(uint64_t id, uint16_t value, string desc) {
        require_auth( _self );
        updateconfig(id, value, desc);
    }

    /**
     * 更新奖池
     */ 
    ACTION poker4dtgame::updatepool(asset asset, name contract, uint64_t min_bet, uint64_t max_bet, uint64_t min_bank) {
        require_auth( _self );
        extended_asset assetInfo{asset, contract};
        
        bool exist = false;
        for (auto it = prizepools.begin(); it != prizepools.end(); it++) {
            if (it->all_asset.quantity.symbol == assetInfo.quantity.symbol) {
                exist = true;
            }
        }

        if (!exist) {
            assetInfo.quantity.amount = 0;
            prizepools.emplace(_self, [&](auto &a_pool) {
                a_pool.id = assetInfo.quantity.symbol.raw();
                a_pool.all_asset = assetInfo;
                a_pool.min_bet = min_bet;
                a_pool.max_bet = max_bet;
                a_pool.min_bank = min_bank;
            });
        } else {
            auto t_pool = prizepools.find(assetInfo.quantity.symbol.raw());
            eosio_assert(t_pool != prizepools.end(), "no pool for that asset");
            eosio_assert(assetInfo.contract == t_pool->all_asset.contract, "invest asset is not pool asset");
            prizepools.modify(t_pool, _self, [&](auto &m_pool) {
                m_pool.min_bet = min_bet;
                m_pool.max_bet = max_bet;
                m_pool.min_bank = min_bank;
            });
        }
    }

    // 用户下注记录
    ACTION poker4dtgame::gamebetlog(uint64_t bet_id, name sender, uint64_t game_id, extended_asset bet_asset, string bet_type_desc, string bet_random) {
        require_auth( _self );
        auto t_bet = bets.find(bet_id);
        eosio_assert(t_bet != bets.end(), "Bet record does not exist");
        require_recipient( sender );
        require_recipient( contract_account_name );
    }

    ACTION poker4dtgame::gamerevealog(uint64_t bet_id, name sender, uint64_t game_id, string block_hash, string bet_type_desc, extended_asset bet_asset, extended_asset win_asset, capi_checksum256 seed_value, string player_source, string dragon_desc, uint8_t dragon_value, string tiger_desc, uint8_t tiger_value) {
        require_auth( _self );
        auto t_bet = bets.find(bet_id);
        eosio_assert(t_bet != bets.end(), "Bet record does not exist");
        require_recipient( sender );
        require_recipient( contract_account_name );
    }

    ACTION poker4dtgame::redeem(name sender, int64_t amount) { // 赎回DTC        
        auto t_player = players.find(sender.value);
        eosio_assert(t_player != players.end(), "Player data does not exist");
        eosio_assert(t_player->dtc_amount >= amount, "no enough Pledged GDC.");
        players.modify(t_player, _self, [&](auto &m_player) {
            m_player.dtc_amount -= amount;
            m_player.pledge_dtc_amount += amount;
        });

        uint64_t divest_id = divests.available_primary_key();
        uint64_t now = current_time() / TIME_MULTIPLE; 
        divests.emplace(_self, [&](auto &a_divest) {
            a_divest.id = divest_id;
            a_divest.user = sender;
            a_divest.divest_type = DIVEST_DTC_TYPE;
            a_divest.invest_num = amount;
            a_divest.invset_time = now;
        });
        
    }

    ACTION poker4dtgame::gamedivest(name sender, asset asset, uint64_t invest_num, name contract) {// 撤资
        extended_asset assetInfo{asset, contract};
        
        auto t_player = players.find(sender.value);
        eosio_assert(t_player != players.end(), "Player data does not exist");
        auto t_pool = prizepools.get(assetInfo.quantity.symbol.raw(), "Prize pool data does not exist");

        int assetIndex = 0;
        for (auto playerAsset = t_player->invests.begin(); playerAsset != t_player->invests.end(); ++playerAsset) {
            if (assetInfo.quantity.symbol == playerAsset->quantity.symbol) {
                eosio_assert(invest_num <= t_player->invest_nums[assetIndex], "The number exceeds the number of evacuable votes");

                players.modify(t_player, _self, [&](auto &m_player) {
                    m_player.invest_nums[assetIndex] -= invest_num;
                });

                // 添加到撤资列表中去
                uint64_t divest_id = divests.available_primary_key();
                uint64_t now = current_time() / TIME_MULTIPLE; 
                divests.emplace(_self, [&](auto &a_divest) {
                    a_divest.id = divest_id;
                    a_divest.user = sender;
                    a_divest.divest_type = DIVEST_INVEST_TYPE;
                    a_divest.divest_asset = assetInfo;
                    a_divest.invest_num = invest_num;
                    a_divest.invset_time = now;
                });

                break;
            }
            assetIndex++;
        }

        // 修改用户得投资记录
        for (auto itr = invests.begin(); itr != invests.end();itr++) {
            if (itr->pool_id == t_pool.id && itr->player == sender) {
                if (itr->invest_num == invest_num) {
                    invests.erase(itr);
                } else {
                    invests.modify(itr, _self, [&](auto &m_inverst){
                        m_inverst.invest_num -= invest_num;
                    });
                }
                return;
            }
        }
    }
   
    ACTION poker4dtgame::startgame(capi_checksum256& seed, string block_hash) {
        require_auth( _self );
        verifystatus();
        
        uint64_t game_id = getval(GAME_NEXT_ID_VAR_ID);
        if (game_id != 0) {
            auto t_game = games.find(game_id - 1);
            // char *c_src = (char *) &seed;
            // assert_sha256(c_src, sizeof(seed), &t_game->seed_hash);
            // 开启自动开牌
            uint64_t now = current_time() / TIME_MULTIPLE;             
            eosio_assert(now + 2 - t_game->reveal_time >= 0, "It’s too early to start playing");
            eosio::transaction txn{};
            txn.actions.emplace_back(
                eosio::permission_level(_self, name("active")),
                _self,
                name("reveal"),
                std::make_tuple(game_id-1, seed, block_hash)
            );
            txn.delay_sec = 2;
            txn.send(game_id-1, _self, false);
        } else {
            insertgame(seed);
        }

    }

    ACTION poker4dtgame::reveal(uint64_t game_id, capi_checksum256& seed, string block_hash) {
        require_auth( _self );

        uint64_t now = current_time() / TIME_MULTIPLE;
        auto t_game = games.find(game_id);
        eosio_assert(t_game != games.end(), "Game data does not exist");
        eosio_assert(t_game->bet_end_time + getval(GAME_LOCK_TIME_ID) <= now , "Not yet reached the draw time");

        string final_seed_string = "";
        final_seed_string.append(to_string(game_id)).append(block_hash);

        capi_checksum256 t_info_seed;
        capi_checksum256 t_random_seed;
        sha256( final_seed_string.c_str(), final_seed_string.size(), &t_info_seed);
        sha256( t_game->player_source.c_str(), t_game->player_source.size(), &t_random_seed);

        int number1 = (int)(t_info_seed.hash[0] + t_info_seed.hash[1] + t_info_seed.hash[2] + t_info_seed.hash[3] + t_info_seed.hash[4] + t_info_seed.hash[5] + t_info_seed.hash[6]);
        int number2 = (int)(t_random_seed.hash[0] + t_random_seed.hash[1] + t_random_seed.hash[2] + t_random_seed.hash[3] + t_random_seed.hash[4] + t_random_seed.hash[5] + t_random_seed.hash[6]);
        int number3 = (int)(seed.hash[0] + seed.hash[1] + seed.hash[2] + seed.hash[3] + seed.hash[4] + seed.hash[5] + seed.hash[6]);

        int number4 = (int)(t_info_seed.hash[7] + t_info_seed.hash[8] + t_info_seed.hash[9] + t_info_seed.hash[10] + t_info_seed.hash[11] + t_info_seed.hash[12] + t_info_seed.hash[13]);
        int number5 = (int)(t_random_seed.hash[7] + t_random_seed.hash[8] + t_random_seed.hash[9] + t_random_seed.hash[10] + t_random_seed.hash[11] + t_random_seed.hash[12] + t_random_seed.hash[13]);
        int number6 = (int)(seed.hash[7] + seed.hash[8] + seed.hash[9] + seed.hash[10] + seed.hash[11] + seed.hash[12] + seed.hash[13]);

        int card_number_1 = (number1 + number2 + number3) % 52;
        int card_number_2 = (number4 + number5 + number5) % 52;

        // 计算中奖结果
        MAP_RESULT result = getWinsMap(card_number_1, card_number_2);
        auto match_itr_lower = bets.begin();
        auto match_itr_upper = bets.end();

        string card_1_desc = get_card_desc(card_number_1);
        string card_2_desc = get_card_desc(card_number_2);

        for ( auto itr = match_itr_lower; itr != match_itr_upper; itr++ ) {
            if (itr->game_id != game_id || itr->status == BET_STATUS_AWARDED) {
                continue;
            }
            extended_asset win_asset = itr->bet_asset;
            win_asset.quantity.amount = 0;
            if (result[itr->bet_type]) {
                win_asset.quantity.amount = itr->bet_asset.quantity.amount * (100 + wins_set[itr->bet_type]) / 100;
                bets.modify(itr, _self, [&](auto &m_bet) {
                    m_bet.is_win = true;
                    m_bet.win_asset = win_asset;
                    m_bet.status = BET_STATUS_AWARDED;
                });

                // 减少奖池金额
                auto t_pool = prizepools.find(itr->bet_asset.quantity.symbol.raw());
                eosio_assert(t_pool != prizepools.end() , "Prize pool data does not exist");
                if (t_pool->all_asset.quantity.amount < win_asset.quantity.amount) { // "奖池资产不足以开奖,游戏暂停,等待游戏方开奖"
                    upconfig(GAME_STATUS_VAR_ID, GAME_STATUS_LOCK, "amount is not enough to pay award");
                }

                prizepools.modify(t_pool, _self, [&](auto &m_pool) {
                    m_pool.all_asset -= win_asset;
                    m_pool.win_count += 1;
                });

                int asset_index = 0;
                bool add = false;
                auto t_player = players.find(itr->player.value);
                eosio_assert(t_player != players.end(), "Player data does not exist");
                for (auto asset_it = t_player->win_asset.begin(); asset_it != t_player->win_asset.end(); ++asset_it) {
                    if ((win_asset.quantity.symbol) == asset_it->quantity.symbol) {
                        players.modify(t_player, _self, [&](auto &m_player) {
                            m_player.win_asset[asset_index].quantity += win_asset.quantity;
                            m_player.win_count += 1;
                        });
                        add = true;
                        break;
                    }
                    asset_index++;
                }
                if (!add) {
                    players.modify(t_player, _self, [&](auto &m_player) {
                        m_player.win_asset.emplace_back(win_asset);
                        m_player.win_count += 1;
                    });
                }

                win_asset_pay(itr->player, win_asset, "Congratulations on your bet to win, this is your reward, look forward to your next victory.");
            
            } else {
                bets.modify(itr, _self, [&](auto &m_bet) {
                    m_bet.is_win = false;
                    m_bet.status = BET_STATUS_AWARDED;
                });
            }

            // 开奖记录
            INLINE_ACTION_SENDER(eosio::poker4dtgame, gamerevealog)(
                contract_account_name, { {contract_account_name, active_permission} },
                { itr->id, itr->player, game_id, block_hash, bet_type_desc[itr->bet_type], itr->bet_asset, win_asset, seed, t_game->player_source, card_1_desc, card_number_1, card_2_desc, card_number_2}
            );

        }

        games.modify(t_game, _self, [&](auto &m_game) {
            m_game.dragon_value = card_number_1;
            m_game.dragon_desc = card_1_desc;
            m_game.tiger_value = card_number_2;
            m_game.tiger_desc = card_2_desc;
            m_game.seed_value = seed;
            m_game.block_hash = block_hash;
            m_game.reveal_time = now;
            m_game.game_status = AWARDED;
        });

        if (divests.begin() != divests.end()) {
            // 开启延时撤资
            eosio::transaction txn{};
            txn.actions.emplace_back(
                eosio::permission_level(_self, name("active")),
                _self,
                name("rootdivest"),
                std::make_tuple()
            );
            txn.delay_sec = 2; // 2秒后实际撤资
            txn.send(game_id, _self, false);
        }

        if (have_bet(t_game->id - 1)) {
            INLINE_ACTION_SENDER(eosio::poker4dtgame, delgamebet)(
                contract_account_name, { {contract_account_name, active_permission} },
                { t_game->id - 1 }
            );
        }

        // 游戏记录action
        INLINE_ACTION_SENDER(eosio::poker4dtgame, gamelog)(
            contract_account_name, { {contract_account_name, active_permission} },
            { game_id, block_hash, t_game->seed_hash, seed, t_game->player_source, getval(BET_NEXT_ID_VAR_ID), card_1_desc, card_number_1, card_2_desc, card_number_2 }
        );

        // 开始下一局游戏
        insertgame(seed);

    }

    ACTION poker4dtgame::gamelog(uint64_t game_id, string block_hash, capi_checksum256 seed_hash, capi_checksum256 seed_value, string player_source, uint64_t next_bet_id, string dragon_desc, uint8_t dragon_value, string tiger_desc, uint8_t tiger_value) {
        require_auth( _self );
        auto t_game = games.find(game_id);
        eosio_assert(t_game != games.end(), "game record does not exist");
        require_recipient( contract_account_name );
    }

    ACTION poker4dtgame::rootdivest() { // 实际撤资
        require_auth( _self );

        uint64_t now = current_time() / TIME_MULTIPLE;
        if (divests.begin() != divests.end()) {
            uint64_t invset_lock_time = getval(REDEEM_DELAY_ID_VAR_ID); // 赎回延时时间
            uint64_t divest_lock_time = getval(DIVEST_DELAY_ID_VAR_ID); // 撤资延时时间
            for (auto itr = divests.begin(); itr != divests.end();) {
                if (itr->divest_type == DIVEST_INVEST_TYPE) { // 撤回投资
                    if (now - itr->invset_time >= divest_lock_time) {
                        auto d_player = players.find(itr->user.value);
                        eosio_assert(d_player != players.end() , "Player data does not exist");
                        auto d_pool = prizepools.find(itr->divest_asset.quantity.symbol.raw());
                        eosio_assert(d_pool != prizepools.end() , "Prize pool data does not exist");
                        extended_asset divest_asset = itr->divest_asset;

                        int assetIndex = 0;
                        for (auto playerAsset = d_player->invests.begin(); playerAsset != d_player->invests.end(); ++playerAsset) {
                            if (divest_asset.quantity.symbol == playerAsset->quantity.symbol) {
                                uint64_t divestAmount = d_pool->all_asset.quantity.amount * itr->invest_num / d_pool->invest_num ;
                                divest_asset.quantity.amount = divestAmount;
                                players.modify(d_player, _self, [&](auto &m_player) {
                                    m_player.divests[assetIndex].quantity.amount += divestAmount;
                                });
                                prizepools.modify(d_pool, _self, [&](auto &m_pool) {
                                    m_pool.all_asset.quantity.amount -= divestAmount;
                                    m_pool.invest_num -= itr->invest_num;
                                });
                                withdraw_asset(d_player->player, divest_asset, "Withdrawal success, looking forward to your next investment");
                                break;
                            }
                            assetIndex++;
                        }
                        itr = divests.erase(itr);
                    } else {
                        itr++;
                    }
                } else if (itr->divest_type == DIVEST_DTC_TYPE){ // 赎回DTC
                    if (now - itr->invset_time >= invset_lock_time) {
                        auto d_player = players.find(itr->user.value);
                        eosio_assert(d_player != players.end() , "Player data does not exist");

                        players.modify(d_player, _self, [&](auto &m_player) {
                            m_player.pledge_dtc_amount -= itr->invest_num;
                        });

                        decreaseVar(DTC_TOTAL_ID, itr->invest_num);
                        asset asset{itr->invest_num, DTC_ASSET_ID};
                        extended_asset redeemAsset{asset, myeostoken_account};
                        withdraw_asset(d_player->player, redeemAsset, "redeem gdc");

                        itr = divests.erase(itr);
                    } else {
                        itr++;
                    }
                } else {
                    itr++;
                }
            }
        }
    }

    /**
     * 删除下注记录
     */ 
    ACTION poker4dtgame::delgamebet(uint64_t game_id) {
        require_auth( _self );
        if (bets.begin() != bets.end()) {
            for (auto itr = bets.begin(); itr != bets.end();) {
                if (itr->game_id <= game_id) {
                    itr = bets.erase(itr);
                } else {
                    break;
                }
            }
        }
    }

    ACTION poker4dtgame::deleteall() {
        require_auth( _self );
        for(auto itr = players.begin(); itr != players.end();) {
            itr = players.erase(itr);
        }
        for(auto itr = bets.begin(); itr != bets.end();) {
            itr = bets.erase(itr);
        }
        for(auto itr = vardics.begin(); itr != vardics.end();) {
            itr = vardics.erase(itr);
        }
        for(auto itr = games.begin(); itr != games.end();) {
            itr = games.erase(itr);
        }
        for(auto itr = prizepools.begin(); itr != prizepools.end();) {
            itr = prizepools.erase(itr);
        }
        for(auto itr = invests.begin(); itr != invests.end();) {
            itr = invests.erase(itr);
        }
        for(auto itr = divests.begin(); itr != divests.end();) {
            itr = divests.erase(itr);
        }
    }

}