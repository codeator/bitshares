#include <bts/blockchain/asset_operations.hpp>
#include <bts/blockchain/asset_record.hpp>
#include <bts/blockchain/chain_interface.hpp>

namespace bts { namespace blockchain {

    share_type asset_record::available_shares()const
    {
        return maximum_share_supply - current_share_supply;
    }

    bool asset_record::can_issue( const asset& amount )const
    {
        if( id != amount.asset_id ) return false;
        return can_issue( amount.amount );
    }

    bool asset_record::can_issue( const share_type amount )const
    {
        if( amount <= 0 ) return false;
        auto new_share_supply = current_share_supply + amount;
        // catch overflow conditions
        return (new_share_supply > current_share_supply) && (new_share_supply <= maximum_share_supply);
    }

    asset asset_record::asset_from_string( const string& amount )const
    {
       asset ugly_asset(0, id);

       // Multiply by the precision and truncate if there are extra digits.
       // example: 100.500019 becomes 10050001
       const auto decimal = amount.find(".");
       ugly_asset.amount += atoll(amount.substr(0, decimal).c_str()) * precision;

       if( decimal != string::npos )
       {
          string fraction_string = amount.substr(decimal+1);
          share_type fraction = atoll(fraction_string.c_str());

          if( !fraction_string.empty() && fraction > 0 )
          {
             while( fraction < precision )
                fraction *= 10;
             while( fraction >= precision )
                fraction /= 10;
             while( fraction_string.size() && fraction_string[0] == '0')
             {
                fraction /= 10;
                fraction_string.erase(0, 1);
             }

             if( ugly_asset.amount >= 0 )
                ugly_asset.amount += fraction;
             else
                ugly_asset.amount -= fraction;
          }
       }

       return ugly_asset;
    }

    string asset_record::amount_to_string( share_type amount, bool append_symbol )const
    {
       const share_type shares = ( amount >= 0 ) ? amount : -amount;
       string decimal = fc::to_string( precision + ( shares % precision ) );
       decimal[0] = '.';
       auto str = fc::to_pretty_string( shares / precision ) + decimal;
       if( append_symbol ) str += " " + symbol;
       if( amount < 0 ) return "-" + str;
       return str;
    }

    const asset_db_interface& asset_record::db_interface( const chain_interface& db )
    { try {
        return db._asset_db_interface;
    } FC_CAPTURE_AND_RETHROW() }

    void asset_record::sanity_check( const chain_interface& db )const
    { try {
        FC_ASSERT( id >= 0 );
        FC_ASSERT( !symbol.empty() );
        FC_ASSERT( !name.empty() );
        FC_ASSERT( id == 0 || issuer_account_id == market_issuer_id || db.lookup<account_record>( issuer_account_id ).valid() );
        FC_ASSERT( is_power_of_ten( precision ) );
        FC_ASSERT( maximum_share_supply >= 0 && maximum_share_supply <= BTS_BLOCKCHAIN_MAX_SHARES );
        FC_ASSERT( current_share_supply >= 0 && current_share_supply <= maximum_share_supply );
        FC_ASSERT( collected_fees >= 0 && collected_fees <= current_share_supply );
        FC_ASSERT( transaction_fee >= 0 && transaction_fee <= maximum_share_supply );
        FC_ASSERT( market_fee <= BTS_BLOCKCHAIN_MAX_UIA_MARKET_FEE );
        /* TODO
        FC_ASSERT( last_proposal_id == 0 || db.lookup<proposal_record>( last_proposal_id ).valid() );
        if( prediction.valid() )
        {
            FC_ASSERT( prediction->base_asset_id == 0 || db.lookup<asset_record>( prediction->base_asset_id ).valid() );
        }
        */
        FC_ASSERT( last_proposal_id == 0 );
        FC_ASSERT( !prediction.valid() );
    } FC_CAPTURE_AND_RETHROW( (*this) ) }

    oasset_record asset_db_interface::lookup( const asset_id_type id )const
    { try {
        return lookup_by_id( id );
    } FC_CAPTURE_AND_RETHROW( (id) ) }

    oasset_record asset_db_interface::lookup( const string& symbol )const
    { try {
        return lookup_by_symbol( symbol );
    } FC_CAPTURE_AND_RETHROW( (symbol) ) }

    void asset_db_interface::store( const asset_id_type id, const asset_record& record )const
    { try {
        const oasset_record prev_record = lookup( id );
        if( prev_record.valid() )
        {
            if( prev_record->symbol != record.symbol )
                erase_from_symbol_map( prev_record->symbol );
        }

        insert_into_id_map( id, record );
        insert_into_symbol_map( record.symbol, id );
    } FC_CAPTURE_AND_RETHROW( (id)(record) ) }

    void asset_db_interface::remove( const asset_id_type id )const
    { try {
        const oasset_record prev_record = lookup( id );
        if( prev_record.valid() )
        {
            erase_from_id_map( id );
            erase_from_symbol_map( prev_record->symbol );
        }
    } FC_CAPTURE_AND_RETHROW( (id) ) }

} } // bts::blockchain
