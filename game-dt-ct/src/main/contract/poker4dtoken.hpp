/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   CONTRACT poker4dtoken : public contract {
      public:
         using contract::contract;

         ACTION create( name   issuer,
                      asset  maximum_supply);

         ACTION issue( name to, asset quantity, string memo );

         ACTION retire( asset quantity, string memo );

         ACTION transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo );

         ACTION open( name owner, const symbol& symbol, name ram_payer );

         ACTION close( name owner, const symbol& symbol );

         static asset get_supply( name token_contract_account, symbol_code sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         static asset get_balance( name token_contract_account, name owner, symbol_code sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

         using create_action = action_wrapper<"create"_n, &poker4dtoken::create>;
         using issue_action = action_wrapper<"issue"_n, &poker4dtoken::issue>;
         using retire_action = action_wrapper<"retire"_n, &poker4dtoken::retire>;
         using transfer_action = action_wrapper<"transfer"_n, &poker4dtoken::transfer>;
         using open_action = action_wrapper<"open"_n, &poker4dtoken::open>;
         using close_action = action_wrapper<"close"_n, &poker4dtoken::close>;

      private:
         TABLE account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         TABLE currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;

         void sub_balance( name owner, asset value );
         void add_balance( name owner, asset value, name ram_payer );
   };

} 
