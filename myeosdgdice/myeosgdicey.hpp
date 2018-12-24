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
    static const uint16_t game_status_var_id = 0;

    static const uint16_t game_status_lock = 1;
    static const uint16_t game_status_play = 2;

    static const uint16_t gt_min_num = 3; // 猜大最小3
    static const uint16_t gt_max_num = 99; // 猜大最大98
    static const uint16_t st_min_num = 1; // 猜小最小1
    static const uint16_t st_max_num = 97; // 猜小最大97
    static const int16_t platform_cut = 150; // 平台抽成
    static const uint64_t time_multiple = 1000000; // 1秒的毫秒数
    static const string default_seed_string = "default_seed_string";

    CONTRACT myeosgdicey : public contract {
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

            uint64_t primary_key() const {
                return player.value;
            }
            
            // EOSLIB_SERIALIZE(player, (owner)(betcount)(wincount)(winamount)(losscount)(bet_asset)(win_asset)(loss_asset))
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

            // EOSLIB_SERIALIZE(bet, (id)(player)(bet_time)(bet_asset)(is_big)(bet_num)(hahs_id)(server_seed)(client_seed)(lucky_number)(win_asset))
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

        myeosgdicey(name receiver, name code, datastream<const char *> ds) : contract(receiver, code, ds),
                                                                        players(receiver, receiver.value), 
                                                                        bets(receiver, receiver.value), 
                                                                        vardics(receiver, receiver.value), 
                                                                        seeds(receiver, receiver.value) {}

        ACTION init(capi_checksum256& hash);
        ACTION upconfig(uint64_t id, uint8_t value);
        ACTION upseed(capi_checksum256& seed, capi_checksum256& hash);
        ACTION lottery(uint64_t bet_id);
        ACTION deleteall();

        static constexpr eosio::name token_account{"eosio.token"_n};
        static constexpr eosio::name active_permission{"active"_n};
        // static constexpr eosio::name myeosgdicey_account{"myeosgdicey_account"_n};

        void transfer(name from, name to, asset quantity, string memo);
        using init_action = action_wrapper<"init"_n, &myeosgdicey::init>;
        using lottery_action = action_wrapper<"lottery"_n, &myeosgdicey::lottery>;
        using upconfig_action = action_wrapper<"upconfig"_n, &myeosgdicey::upconfig>;
        using upseed_action = action_wrapper<"upseed"_n, &myeosgdicey::upseed>;
        using deleteall_action = action_wrapper<"deleteall"_n, &myeosgdicey::deleteall>;

    private:
        void roll(uint64_t bet_id);
        // void win_asset_pay(name winner, asset win_asset);
        void insertval(uint64_t id, uint64_t value);
        void insertseed(capi_checksum256& hash);
        void updateconfig(uint64_t id, uint16_t value);
        uint64_t findlastseedid();
        void bet(name sender, asset bet_asset, int16_t bet_num, bool is_big, capi_checksum256 client_seed);
        void win_asset_pay(name winner, asset win_asset);

        player_index        players;
        bet_index           bets;
        vardic_index        vardics;
        seed_index          seeds;
    };

    extern "C"
    {
        void apply(uint64_t receiver, uint64_t code, uint64_t action)
        {
            auto self = receiver;
            if (code == "eosio.token"_n.value)
            {
                myeosgdicey mgdice(eosio::name(receiver), eosio::name(code), datastream<const char *>(nullptr, 0));
                const auto t = unpack_action_data<myeosgdicey::transfer_args>();
                mgdice.transfer(t.from, t.to, t.quantity, t.memo);
            }
            else if (code == self || action == "onerror"_n.value)
            {
                switch (action)
                {
                    EOSIO_DISPATCH_HELPER(myeosgdicey, (init)(lottery)(upconfig)(upseed)(deleteall));
                    default:
                        eosio_assert(false, "it is not my action.");
                        break;
                }
            }
        }
    }

} // namespace eosio