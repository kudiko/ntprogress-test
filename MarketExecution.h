#pragma once

#include <list>
#include <iostream>
#include <pqxx/pqxx>

#include "ServerDatabase.h"


class MarketExecution
{
public:
    MarketExecution();
    MarketExecution(const std::string& db_connection_params);
    ~MarketExecution() = default;
    enum class BidType
    {
        BUY,
        SELL
    };

    enum class BidStatus
    {
        ACTUAL,
        FILLED,
        PARTIALLY_FILLED,
        ABORTED
    };

    struct Bid
    {
        BidType type_;
        std::size_t owner_;
        int price_;
        int quantity_;
        BidStatus status_;

        int filled_;
    };

    struct PriceLevel
    {
        int price_;
        std::list<Bid*> bid_queue_;
    };

    void AddBid(Bid new_bid);
    bool AbortBid(size_t bid_id_to_abort);

    const ServerDatabase& GetServerDatabase() const;
    ServerDatabase& GetServerDatabase();

    std::vector<Bid*> GetUserBidHist(size_t id) const;
    size_t GetBidId(Bid* bid) const;



private:
    friend class Tests;
    std::vector<Bid> bid_history_;

    std::unordered_map<size_t, std::vector<Bid*>> bid_hist_per_user_;

    // back of vector -> the highest price buys
    std::vector<PriceLevel> current_buy_bids_;

    // back of vector -> the lowest sell buys
    std::vector<PriceLevel> current_sell_bids_;

    void AddBuyBidToCurrent(Bid* new_bid_ptr);

    void AddSellBidToCurrent(Bid* new_bid_ptr);

    void TriggerMatching();

    void ProcessBuyVolumeMoreThanSellVolume(Bid* cur_buy_bid, Bid* cur_sell_bid, int buy_quantity_not_filled, int sell_quantity_not_filled);

    void ProcessBuyVolumeEqualsSellVolume(Bid* cur_buy_bid, Bid* cur_sell_bid, int buy_quantity_not_filled, int sell_quantity_not_filled);

    void ProcessSellVolumeMoreThanBuyVolume(Bid* cur_buy_bid, Bid* cur_sell_bid, int buy_quantity_not_filled, int sell_quantity_not_filled);

    ServerDatabase db_;
    std::optional<pqxx::connection> SQL_connection_;
};

std::string GetStringBidType(MarketExecution::BidType);
std::string GetStringStatus(MarketExecution::BidStatus);
std::ostream& operator<<(std::ostream& out, MarketExecution::Bid bid);