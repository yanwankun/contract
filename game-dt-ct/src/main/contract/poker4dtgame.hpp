#pragma once
#include <eosiolib/serialize.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.h>
#include <eosiolib/symbol.hpp>
#include <eosiolib/crypto.hpp>
#include <tuple>
#include <limits>
#include <vector>
#include <string>

namespace eosiosystem
{
    class system_contract;
}

namespace eosio {

    using std::set;
    using std::string;
    using std::vector;
    using std::to_string;
    using std::map;
        
    CONTRACT poker4dtgame : public contract {
    public:
        typedef std::map<int, bool> MAP_RESULT;
        // 游戏状态
        enum game_status_enum : int8_t {
            PROCESSING = 1, // 进行中
            AWARDED = 2 // 以开奖
        };

        // 平台状态
        enum pt_status_enum : int8_t {
            GAME_STATUS_LOCK = 1, // 已锁定
            GAME_STATUS_PLAY = 2 // 未锁定
        };

        // 奖励发送状态
        enum reward_status_enum : int8_t {
            BET_STATUS_NOT_AWARDED = 1, // 奖励还没有发
            BET_STATUS_AWARDED = 2 // 奖励已经发了
        };

        // 延迟类型
        enum delay_status_enum : int8_t {
            DIVEST_DTC_TYPE = 1, // 赎回dtc
            DIVEST_INVEST_TYPE = 2 // 撤回投资
        };

        enum bet_type_enum : int8_t {
            DRAGON_WIN = 1, // 龙赢
            DRAGON_EVEN = 2, // 龙双
            DRAGON_ODD = 3, // 龙单
            DRAGON_BLACK = 4, // 龙黑
            DRAGON_RED = 5, // 龙红
            SAME = 6,
            TIGER_WIN = 7, // 虎赢
            TIGER_EVEN = 8, // 虎双
            TIGER_ODD = 9, // 虎单
            TIGER_BLACK = 10, // 虎黑
            TIGER_RED = 11, // 虎红
        };

        const vector<int> wins_set = {0, 100, 105, 75, 90, 90, 800, 100, 105, 75, 90, 90};
        const vector<string> card_value = {"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
        const vector<string> card_color = {"", "黑桃", "红桃", "樱花", "方片"};
        const vector<string> bet_type_desc = {"", "DRAGON_WIN", "DRAGON_EVEN", "DRAGON_ODD", "DRAGON_BLACK", "DRAGON_RED", "FLAT CARD", 
        "TIGER_WIN", "TIGER_EVEN", "TIGER_ODD", "TIGER_BLACK", "TIGER_RED"};
        const vector<int> cards = {100,101,102,103,104,105,106,107,108,109,110,111,112,
                            200,201,202,203,204,205,206,207,208,209,210,211,212,
                            300,301,302,303,304,305,306,307,308,309,310,311,312,
                            400,401,402,403,404,405,406,407,408,409,410,411,412
                            }; // 所有得扑克牌 
        const int64_t MIN_INVERT_NUM = 10000; // 第一次投资的票数
        const int16_t PLATFORM_CUT = 150; // 平台抽成
        const uint64_t TIME_MULTIPLE = 1000000; // 1秒的毫秒数

        const symbol DTC_ASSET_ID = symbol(symbol_code("GDT"), 4);
        const symbol EOS_ASSET_ID = symbol(symbol_code("EOS"), 4);

        const uint16_t GAME_STATUS_VAR_ID = 0;
        const uint64_t GAME_LOCK_TIME_ID = 2; // 游戏锁定时间
        const uint64_t TABLE_SAVE_COUNT_ID = 3; // 表保存得得行数
        const uint64_t ONCE_DELETE_COUNT_ID = 4; // 表一次删除得行数
        const uint64_t DTC_TOTAL_ID = 5; // 质押的DTC总量
        const uint64_t GAME_NEXT_ID_VAR_ID = 6; // 记录x下一次gameid的字典id
        const uint64_t INVESTMENT_DILUTION_RATIO_ID = 7; // 投资稀释百分比得
        const uint64_t BET_NEXT_ID_VAR_ID = 8; // 下次游戏下注得id
        const uint64_t REWARD_RATE_ID_VAR_ID = 9; // gdt奖励得比率
        const uint64_t DIVEST_DELAY_ID_VAR_ID = 10; // 撤资延时时间
        const uint64_t REDEEM_DELAY_ID_VAR_ID = 11; //赎回延时时间
        const uint64_t MAX_WIN_LIMIT_RATE_ID_VAR_ID = 12; // 一局游戏的最大可以赢得百分比
        const uint64_t GAME_BET_TIME_ID_VAR_ID = 13; // 游戏下注时间描述
        const int64_t INVEST_ZOOM_UP = 10000; // 投资比例放大倍数，为了精确
        
        struct transfer_args {
            name from;
            name to;
            asset quantity;
            string memo;
         };

        TABLE player {
            name player;
            uint64_t bet_count;
            uint64_t win_count;;
            uint64_t loss_count;
            vector<extended_asset> bet_asset;
            vector<extended_asset> win_asset;

            vector<extended_asset> invests; // 投资金额
            vector<extended_asset> divests; // 撤资 金额
            vector<uint64_t> invest_nums; // 投资的票数

            uint64_t dtc_amount;
            uint64_t pledge_dtc_amount; // 赎回中DTC数量

            uint64_t primary_key() const {
                return player.value;
            }
        };

        // 投资记录表
        TABLE invest {
            uint64_t id;
            uint64_t pool_id;
            name player;
            uint64_t invest_num;

            uint64_t primary_key() const {
                return id;
            }
        };

        // 撤资表
        TABLE divest {
            uint64_t id;
            name user;
            uint8_t divest_type; // 撤回类型
            extended_asset divest_asset;
            int64_t invest_num; // 针对撤资来说是票数, 针对赎回DTC来说是GDC的数量
            uint64_t invset_time; // 撤资时间
            uint64_t primary_key() const {
                return id;
            }
        };

        TABLE prizepool {
            uint64_t id;
            extended_asset all_asset; // 总投资额
            // uint64_t all_invest_amount;
            uint64_t total_bet = 0;
            uint64_t bet_count = 0;
            uint64_t win_count = 0;
            uint64_t min_bet; // 最小投注额
            uint64_t max_bet; // 最大投注额
            uint64_t min_bank; // 最小坐庄额
            uint64_t invest_num = 0; // 总票数,第一个投资的人的票数为 1000
            uint64_t profit; // 利润
            uint64_t primary_key() const {
                return id;
            }
        };
       
        TABLE game {
            uint64_t id;  // 赌局id，如:201809130001
            uint8_t game_status; // 状态：0:进行中, 1:开奖中, 2:已结束
            uint64_t bet_start_id; // 下注开始id
            capi_checksum256 seed_hash;// 开奖种子hash
            capi_checksum256 seed_value;// 开奖种子value
            string block_hash;
            // vector<extended_asset> max_win_asset; // 当局最大可赢金额数量
            string player_source; // 玩家随机因子
            uint8_t dragon_value; // 第一张牌
            string dragon_desc;
            uint8_t tiger_value; // 第二张牌
            string tiger_desc;
            int64_t create_time; // 创建时间
            int64_t bet_end_time; // 投注结束时间
            int64_t reveal_time; // 开奖时间
            uint64_t primary_key() const { return id; }
        };

        TABLE bet {
            uint64_t id;
            uint64_t game_id; // 赌局id
            name player;
            int64_t bet_time;
            extended_asset bet_asset;
            int8_t bet_type;
            int8_t status; // 开奖状态
            string bet_random;
            bool is_win;
            extended_asset win_asset;
            
            uint64_t primary_key() const { return id; }
            uint64_t by_gameid() const { return game_id; } // 可以通过赌局id查询数据
        };       

        TABLE vardic {
            uint64_t id;
            uint16_t value;
            string desc;
            uint64_t primary_key() const {return id;}
        };

        typedef multi_index<name("player"), player> player_index;
        typedef multi_index<name("game"), game> game_index;
        typedef multi_index<name("bet"), bet,
                    indexed_by<name("gameid"), const_mem_fun<bet, uint64_t, &bet::by_gameid>>
        > bet_index;
        typedef multi_index<name("vardic"), vardic> vardic_index;
        typedef multi_index<name("prizepool"), prizepool> prizepool_index;
        typedef multi_index<name("divest"), divest> divest_index;
        typedef multi_index<name("invest"), invest> invest_index;

        poker4dtgame(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds),
                                                                        players(receiver, receiver.value), 
                                                                        games(receiver, receiver.value), 
                                                                        bets(receiver, receiver.value), 
                                                                        vardics(receiver, receiver.value),
                                                                        prizepools(receiver, receiver.value), 
                                                                        divests(receiver, receiver.value),
                                                                        invests(receiver, receiver.value) 
                                                                        {}

        ACTION init(); // 初始化
        ACTION upconfig(uint64_t id, uint16_t value, string desc); // 更新配置
        ACTION startgame(capi_checksum256& seed, string block_hash);    
        ACTION reveal(uint64_t game_id, capi_checksum256& hash, string block_hash); // 开奖 
        ACTION delgamebet(uint64_t game_id); // 删除某次游戏的下注记录
        ACTION deleteall(); // 删除所有
        ACTION updatepool(asset asset, name contract, uint64_t min_bet, uint64_t max_bet, uint64_t min_bank);
        ACTION redeem(name sender, int64_t amount);
        ACTION gamedivest(name sender, asset asset, uint64_t invest_num, name contcrat); // 撤资
        ACTION gamebetlog(uint64_t bet_id, name sender, uint64_t game_id, extended_asset bet_asset, string bet_type_desc, string bet_random);
        ACTION gamerevealog(uint64_t bet_id, name sender, uint64_t game_id, string block_hash, string bet_type_desc, extended_asset bet_asset, extended_asset win_asset, capi_checksum256 seed_value, string player_source, string dragon_desc, uint8_t dragon_value, string tiger_desc, uint8_t tiger_value);
        ACTION gamelog(uint64_t game_id, string block_hash, capi_checksum256 seed_hash, capi_checksum256 seed_value, string player_source, uint64_t next_bet_id, string dragon_desc, uint8_t dragon_value, string tiger_desc, uint8_t tiger_value);
        ACTION rootdivest(); // 实际撤资         

        static constexpr name token_account{"eosio.token"_n};
        static constexpr name myeostoken_account{"poker4dtoken"_n};
        static constexpr name active_permission{"active"_n};
        static constexpr name contract_account_name{"poker4dtgame"_n};

        using init_action = action_wrapper<"init"_n, &poker4dtgame::init>;
        using upconfig_action = action_wrapper<"upconfig"_n, &poker4dtgame::upconfig>;
        using delgamebet_action = action_wrapper<"delgamebet"_n, &poker4dtgame::delgamebet>;
        using startgame_action = action_wrapper<"startgame"_n, &poker4dtgame::startgame>;
        using reveal_action = action_wrapper<"reveal"_n, &poker4dtgame::reveal>;
        using deleteall_action = action_wrapper<"deleteall"_n, &poker4dtgame::deleteall>;
        using updatepool_action = action_wrapper<"updatepool"_n, &poker4dtgame::updatepool>;
        using gamedivest_action = action_wrapper<"gamedivest"_n, &poker4dtgame::gamedivest>;
        using redeem_action = action_wrapper<"redeem"_n, &poker4dtgame::redeem>;
        using gamebetlog_action = action_wrapper<"gamebetlog"_n, &poker4dtgame::gamebetlog>;
        using gamerevealog_action = action_wrapper<"gamerevealog"_n, &poker4dtgame::gamerevealog>;
        using gamelog_action = action_wrapper<"gamelog"_n, &poker4dtgame::gamelog>;
        using rootdivest_action = action_wrapper<"rootdivest"_n, &poker4dtgame::rootdivest>;

        void transfer(name from, name to, asset quantity, string memo); // 监听eosio.token 的转账
        void metransfer(name from, name to, asset quantity, string memo); // 监听自己得资产合约得转账
    
    private:
        void insertval(uint64_t id, uint16_t value, string desc);
        void insertseed(capi_checksum256& hash);
        void updateconfig(uint64_t id, uint16_t value, string desc);
        void gamebet(name sender, uint64_t game_id, extended_asset bet_asset, int8_t bet_type, string bet_random);
        void withdraw_asset(name to, extended_asset quantity, string memo);
        uint64_t getval(uint64_t id);
        void reward_transfer(extended_asset bet_asset, name to);
        void increaseVar(uint64_t id, int64_t delta);
        uint64_t insertgame(capi_checksum256& seed_hash);
        void decreaseVar(uint64_t id, int64_t delta);
        MAP_RESULT getWinsMap(uint8_t dragon_value, const uint8_t tiger_value);
        void win_asset_pay(name winner, extended_asset quantity, string memo);
        void verifystatus(); 
        // void verify_bet_amount(extended_asset bet_asset); 
        void pledge(name sender, extended_asset quantity); // 质押游戏币
        void gameinvest(name sender, extended_asset quantity); // 投资坐庄
        bool have_bet(uint64_t game_id); // 判断是否有人下注
        string get_card_desc(uint8_t card_num);

        player_index players;
        game_index games;
        bet_index bets;
        vardic_index vardics;
        prizepool_index prizepools;
        divest_index divests;
        invest_index invests;
    };

    extern "C"
    {
        void apply(uint64_t receiver, uint64_t code, uint64_t action)
        {
            auto self = receiver;
            if (code == poker4dtgame::token_account.value)
            {
                poker4dtgame mgdice(eosio::name(receiver), eosio::name(code), datastream<const char *>(nullptr, 0));
                const auto t = unpack_action_data<poker4dtgame::transfer_args>();
                mgdice.transfer(t.from, t.to, t.quantity, t.memo);
            } else if (code == poker4dtgame::myeostoken_account.value) {
                poker4dtgame mgdice(eosio::name(receiver), eosio::name(code), datastream<const char *>(nullptr, 0));
                const auto t = unpack_action_data<poker4dtgame::transfer_args>();
                mgdice.metransfer(t.from, t.to, t.quantity, t.memo);
            }
            else if (code == self || action == "onerror"_n.value)
            {
                switch (action)
                {
                    EOSIO_DISPATCH_HELPER(poker4dtgame, (init)(upconfig)(delgamebet)(startgame)(reveal)(deleteall)(updatepool)(redeem)(gamedivest)(gamebetlog)(gamerevealog)(gamelog)(rootdivest));
                    default:
                        eosio_assert(false, "it is not my action.");
                        break;
                }
            }
        }
    }

}