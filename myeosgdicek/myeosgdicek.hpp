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

    // 游戏状态
    static const uint16_t GAME_STATUS_LOCK = 1;
    static const uint16_t GAME_STATUS_PLAY = 2;

    static const uint16_t GT_MIN_NUM = 3; // 猜大最小3
    static const uint16_t GT_MAX_NUM = 99; // 猜大最大98
    static const uint16_t ST_MIN_NUM = 1; // 猜小最小1
    static const uint16_t ST_MAX_NUM = 97; // 猜小最大97
    static const int16_t PLATFORM_CUT = 150; // 平台抽成
    static const uint64_t TIME_MULTIPLE = 1000000; // 1秒的毫秒数
    const int REWARD_RATE = 50;  // 奖励bet的比率，

    const symbol GDC_ASSET_ID = symbol(symbol_code("GDC"), 4);
    const symbol EOS_ASSET_ID = symbol(symbol_code("EOS"), 4);

    static const uint16_t GAME_STATUS_VAR_ID = 0;
    static const uint64_t BET_NMBER_ID = 1; // 记录总下注次数的id
    static const uint64_t GDC_TOTAL_ID = 5; // 质押的GDC总量
    static const int64_t INVEST_ZOOM_UP = 1000 * 100000; // 投资比例放大倍数，为了精确
    static const int64_t MAX_WIN_LIMIT_RATE = 100; // 不超过奖池的1%

    CONTRACT myeosgdicek : public contract {
    public:

        struct transfer_args {
            name  from;
            name  to;
            asset         quantity;
            string        memo;
         };

        // @abi table player i64
        TABLE player {
            name player;
            uint64_t betcount;
            uint64_t wincount;;
            uint64_t losscount;
            vector<asset> bet_asset;
            vector<asset> win_asset;

            std::vector<asset> investamount;
            std::vector<asset> divestamount;
            std::vector<asset> investpercent; // 投资占比，10000=1%

            uint64_t gdcamount = 0; // 质押的gdc

            uint64_t primary_key() const {
                return player.value;
            }
        };

        TABLE prizepool {
            uint64_t id;
            asset pool;
            uint64_t totalbet = 0;
            uint64_t betcount = 0;
            uint64_t wincount = 0;
            
            uint64_t minbet; // 最小投注额
            uint64_t minbank; // 最小坐庄额

            uint64_t investtotalpercent = 0;

            int64_t profit = 0; // 利润

            uint64_t primary_key() const {
                return id;
            }
        };
       
        
        // @abi table bet i64
        TABLE bet {
            uint64_t id;
            name player;
            int64_t bet_time;
            asset bet_asset;
            bool is_big;
            uint16_t bet_num;
            uint64_t seed_id;
            capi_checksum256 server_seed;
            capi_checksum256 client_seed;
            uint16_t lucky_number;
            asset win_asset;
            uint64_t lottery_t_micros;// 开奖时间 
            
            uint64_t primary_key() const { return id; }
        };       

        TABLE vardic {
            uint64_t id;
            uint16_t value;
            uint64_t primary_key() const {return id;}
            // EOSLIB_SERIALIZE(vardic, (id)(value))
        };

        // @abi table seed i64
        TABLE seed {
            uint64_t id;
            capi_checksum256 seed_hash;
            capi_checksum256 seed_value;
            // uint64_t expired;
            uint64_t primary_key() const {return id;}
            // EOSLIB_SERIALIZE(serverseed, (id)(seed_hash)(seed_value))
        };

        typedef multi_index<name("player"), player> player_index;
        typedef multi_index<name("bet"), bet> bet_index;
        typedef multi_index<name("vardic"), vardic> vardic_index;
        typedef multi_index<name("seed"), seed> seed_index;
        typedef multi_index<name("prizepool"), prizepool> prizepool_index;

        myeosgdicek(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds),
                                                                        players(receiver, receiver.value), 
                                                                        bets(receiver, receiver.value), 
                                                                        vardics(receiver, receiver.value), 
                                                                        prizepools(receiver, receiver.value),
                                                                        seeds(receiver, receiver.value) {}

        ACTION init(capi_checksum256& hash);
        ACTION upconfig(uint64_t id, uint8_t value);
        ACTION upseed(capi_checksum256& seed, capi_checksum256& hash);
        ACTION lottery(uint64_t bet_id);
        ACTION deleteall();

        ACTION divest(name sender, uint64_t poolid, int64_t amount);
        // ACTION updatestatus(uint64_t status);
        ACTION redeem(name sender, int64_t amount);

        static constexpr eosio::name token_account{"eosio.token"_n};
        static constexpr eosio::name active_permission{"active"_n};
        // static constexpr eosio::name myeosgdicek_account{"myeosgdicek_account"_n};

        void transfer(name from, name to, asset quantity, string memo);
        using init_action = action_wrapper<"init"_n, &myeosgdicek::init>;
        using lottery_action = action_wrapper<"lottery"_n, &myeosgdicek::lottery>;
        using upconfig_action = action_wrapper<"upconfig"_n, &myeosgdicek::upconfig>;
        using upseed_action = action_wrapper<"upseed"_n, &myeosgdicek::upseed>;
        using deleteall_action = action_wrapper<"deleteall"_n, &myeosgdicek::deleteall>;

        // using updatepool_action = action_wrapper<"updatepool"_n, &myeosgdicek::updatepool>;
        using divest_action = action_wrapper<"divest"_n, &myeosgdicek::divest>;
        using redeem_action = action_wrapper<"redeem"_n, &myeosgdicek::redeem>;

    private:

        void insertval(uint64_t id, uint64_t value);
        void insertseed(capi_checksum256& hash);
        void updateconfig(uint64_t id, uint16_t value);
        uint64_t findlastseedid();
        void bet(name sender, asset bet_asset, int16_t bet_num, bool is_big, capi_checksum256 client_seed);
        void win_asset_pay(name winner, asset win_asset);
        void withdraw_asset(name to, asset quantity, string memo);
        bool gamePaused();
        void invest(name sender, asset quantity);
        uint64_t getval(uint64_t id);
        int64_t calulatePayback(int64_t amount, uint16_t rate);
        void reward_transfer(asset bet_asset, name to);
        void increaseVar(uint64_t id, int64_t delta);
        void decreaseVar(uint64_t id, int64_t delta);
        void pledge(name sender, asset quantity);
        void updatepool(asset assetInfo, uint64_t minBet, uint64_t minBank);

        player_index        players;
        bet_index           bets;
        vardic_index        vardics;
        seed_index          seeds;
        prizepool_index     prizepools;
    };

    extern "C"
    {
        void apply(uint64_t receiver, uint64_t code, uint64_t action)
        {
            auto self = receiver;
            if (code == "eosio.token"_n.value)
            {
                myeosgdicek mgdice(eosio::name(receiver), eosio::name(code), datastream<const char *>(nullptr, 0));
                const auto t = unpack_action_data<myeosgdicek::transfer_args>();
                mgdice.transfer(t.from, t.to, t.quantity, t.memo);
            }
            else if (code == self || action == "onerror"_n.value)
            {
                switch (action)
                {
                    EOSIO_DISPATCH_HELPER(myeosgdicek, (init)(lottery)(upconfig)(upseed)(deleteall)(divest)(redeem));// updatestatus pledge updatepool
                    default:
                        eosio_assert(false, "it is not my action.");
                        break;
                }
            }
        }
    }

}