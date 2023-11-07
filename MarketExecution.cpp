#include "MarketExecution.h"

#include <algorithm>
#include <pqxx/pqxx>

MarketExecution::MarketExecution()
{
    bid_history_.reserve(10'000);
    current_buy_bids_.reserve(1'000);
    current_sell_bids_.reserve(1'000);
}

MarketExecution::MarketExecution(const std::string& db_connection_params) : MarketExecution()
{
    try {
        SQL_connection_ = pqxx::connection(db_connection_params);
        if (SQL_connection_.value().is_open()) {
            std::cerr << "Opened database successfully: " << SQL_connection_.value().dbname() << std::endl;

            pqxx::work W(SQL_connection_.value());
            W.exec("DROP TABLE IF EXISTS bid_history;");


            std::string sql_table_creation = "CREATE TABLE bid_history(" \
                "bid_id integer NOT NULL," \
                "owner integer NOT NULL," \
                "type varchar(5) NOT NULL," \
                "initial_price integer NOT NULL," \
                "volume integer NOT NULL," \
                "cur_status varchar(20) NOT NULL," \
                "filled integer NOT NULL," \
                "PRIMARY KEY (bid_id) );";

            W.exec(sql_table_creation);
            W.commit();

        } else {
            std::cerr << "Can't open database" << std::endl;\
                SQL_connection_ = std::nullopt;
        }
    } catch (const std::exception &e) {
        std::cerr << "DB connection exception: " << e.what() << std::endl;
        SQL_connection_ = std::nullopt;
    }
}

void MarketExecution::AddBid(Bid new_bid)
{
    int new_bid_id = bid_history_.size();
    bid_history_.push_back(new_bid);
    Bid* new_bid_ptr = &(bid_history_.back());

    bid_hist_per_user_[new_bid_ptr->owner_].push_back(new_bid_ptr);
    if (new_bid.type_ == BidType::BUY)
    {
        AddBuyBidToCurrent(new_bid_ptr);
    } else {
        AddSellBidToCurrent(new_bid_ptr);
    }

    if (SQL_connection_.has_value())
    {

        pqxx::work W(SQL_connection_.value());
        W.exec("INSERT INTO bid_history (bid_id, owner, type, initial_price, volume, cur_status, filled) VALUES (" +
               std::to_string(new_bid_id) + ", " + std::to_string(new_bid.owner_) + ", " + "\'" + GetStringBidType(new_bid.type_) + "\'"
               + ", "+ std::to_string(new_bid.price_) + ", " + std::to_string(new_bid.quantity_) + ", " + "\'" + GetStringStatus(new_bid.status_) + "\'"
               + ", " + std::to_string(new_bid.filled_) + ");");
        W.commit();
    }
}

bool MarketExecution::AbortBid(size_t bid_id_to_abort)
{
    if (bid_history_[bid_id_to_abort].status_ == MarketExecution::BidStatus::ACTUAL ||
            bid_history_[bid_id_to_abort].status_ == MarketExecution::BidStatus::PARTIALLY_FILLED)
    {
        int price = bid_history_[bid_id_to_abort].price_;
        if (bid_history_[bid_id_to_abort].type_ == MarketExecution::BidType::BUY)
        {
            auto price_level_it = std::find_if(current_buy_bids_.rbegin(), current_buy_bids_.rend(),
                                               [&price](const PriceLevel& lvl){
                                                            return lvl.price_ == price;
                                                                                            });

            if (price_level_it == current_buy_bids_.rend())
            {
                return false;
            }

            auto place_in_queue_it = std::find(price_level_it->bid_queue_.begin(), price_level_it->bid_queue_.end(),
                                               &bid_history_[bid_id_to_abort]);
            price_level_it->bid_queue_.erase(place_in_queue_it);
        } else {
            auto price_level_it = std::find_if(current_sell_bids_.rbegin(), current_sell_bids_.rend(),
                                               [&price](const PriceLevel& lvl){
                                                   return lvl.price_ == price;
                                               });

            if (price_level_it == current_sell_bids_.rend())
            {
                return false;
            }

            auto place_in_queue_it = std::find(price_level_it->bid_queue_.begin(), price_level_it->bid_queue_.end(),
                                               &bid_history_[bid_id_to_abort]);
            price_level_it->bid_queue_.erase(place_in_queue_it);
        }

        bid_history_[bid_id_to_abort].status_ = MarketExecution::BidStatus::ABORTED;

        if (SQL_connection_.has_value())
        {

            pqxx::work W(SQL_connection_.value());
            W.exec("UPDATE bid_history SET cur_status = \'ABORTED\' WHERE bid_id = " +  std::to_string(bid_id_to_abort) + ";");
            W.commit();
        }

        return true;
    }

    return false;
}


void MarketExecution::AddBuyBidToCurrent(Bid* new_bid_ptr)
{
    const auto lb = std::lower_bound(current_buy_bids_.begin(), current_buy_bids_.end(), new_bid_ptr->price_,
                                     [](PriceLevel lhs, int rhs)
                                     {
                                         return lhs.price_ < rhs;
                                     });
    const auto ub = std::upper_bound(current_buy_bids_.begin(), current_buy_bids_.end(), new_bid_ptr->price_,
                                     [](int lhs, PriceLevel rhs)
                                     {
                                         return lhs < rhs.price_;
                                     });

    if (lb == ub)
    {
        std::list<Bid*> new_price_queue;
        new_price_queue.push_back(new_bid_ptr);

        if (lb == current_buy_bids_.end())
        {
            current_buy_bids_.push_back({new_bid_ptr->price_, std::move(new_price_queue)});
        } else {
            current_buy_bids_.insert(lb, {new_bid_ptr->price_, std::move(new_price_queue)});
        }
    } else {
        lb->bid_queue_.push_back(new_bid_ptr);
    }
    TriggerMatching();

}

void MarketExecution::AddSellBidToCurrent(Bid* new_bid_ptr)
{
    const auto lb = std::lower_bound(current_sell_bids_.rbegin(), current_sell_bids_.rend(), new_bid_ptr->price_,
                                     [](PriceLevel lhs, int rhs)
                                     {
                                         return lhs.price_ < rhs;
                                     });

    const auto ub = std::upper_bound(current_sell_bids_.rbegin(), current_sell_bids_.rend(), new_bid_ptr->price_,
                                     [](int lhs, PriceLevel rhs)
                                     {
                                         return lhs < rhs.price_;
                                     });

    if (lb == ub)
    {
        std::list<Bid*> new_price_queue;
        new_price_queue.push_back(new_bid_ptr);

        if (lb == current_sell_bids_.rend())
        {
            current_sell_bids_.insert(current_sell_bids_.begin(),
                                      {new_bid_ptr->price_, std::move(new_price_queue)});
        } else {
            current_sell_bids_.insert(lb.base(), {new_bid_ptr->price_, std::move(new_price_queue)});
        }
    } else {
        lb->bid_queue_.push_back(new_bid_ptr);
    }
    TriggerMatching();
}

size_t MarketExecution::GetBidId(Bid* bid) const
{
    return bid - bid_history_.data();
}


void MarketExecution::TriggerMatching()
{
    while (!current_buy_bids_.empty() && !current_sell_bids_.empty() && current_buy_bids_.back().price_ >= current_sell_bids_.back().price_)
    {

            auto cur_buy_bid = current_buy_bids_.back().bid_queue_.front();
            auto cur_sell_bid = current_sell_bids_.back().bid_queue_.front();


            int buy_quantity_not_filled = cur_buy_bid->quantity_ - cur_buy_bid->filled_;
            int sell_quantity_not_filled = cur_sell_bid->quantity_ - cur_sell_bid->filled_;

            if (buy_quantity_not_filled > sell_quantity_not_filled)
            {
                ProcessBuyVolumeMoreThanSellVolume(cur_buy_bid, cur_sell_bid, buy_quantity_not_filled,
                                                   sell_quantity_not_filled);
            } else if (buy_quantity_not_filled == sell_quantity_not_filled) {
                ProcessBuyVolumeEqualsSellVolume(cur_buy_bid, cur_sell_bid, buy_quantity_not_filled,
                                                 sell_quantity_not_filled);
            } else {
                ProcessSellVolumeMoreThanBuyVolume(cur_buy_bid, cur_sell_bid, buy_quantity_not_filled,
                                                   sell_quantity_not_filled);
            }

    }
}
void MarketExecution::ProcessBuyVolumeMoreThanSellVolume(Bid* cur_buy_bid, Bid* cur_sell_bid, int buy_quantity_not_filled, int sell_quantity_not_filled)
{
    int price_of_current_deal = cur_buy_bid > cur_sell_bid ? cur_sell_bid->price_ : cur_buy_bid->price_;

    cur_buy_bid->filled_ += sell_quantity_not_filled;
    cur_sell_bid->filled_ += sell_quantity_not_filled;

    cur_buy_bid->status_ = BidStatus::PARTIALLY_FILLED;
    cur_sell_bid->status_ = BidStatus::FILLED;




    db_.ChangeBalance(cur_buy_bid->owner_, -price_of_current_deal * sell_quantity_not_filled, sell_quantity_not_filled);
    db_.ChangeBalance(cur_sell_bid->owner_, price_of_current_deal * sell_quantity_not_filled, -sell_quantity_not_filled);

    current_sell_bids_.back().bid_queue_.pop_front();
    if (current_sell_bids_.back().bid_queue_.empty())
    {
        current_sell_bids_.pop_back();
    }

    if (SQL_connection_.has_value())
    {

        pqxx::work refresh_sql(SQL_connection_.value());
        refresh_sql.exec("UPDATE bid_history SET cur_status = \'PARTIALLY_FILLED\', filled = " + std::to_string(sell_quantity_not_filled) +
        " WHERE bid_id = " +  std::to_string(GetBidId(cur_buy_bid)) + ";");
        refresh_sql.exec("UPDATE bid_history SET cur_status = \'FILLED\', filled = " + std::to_string(sell_quantity_not_filled) +
               " WHERE bid_id = " +  std::to_string(GetBidId(cur_sell_bid)) + ";");
        refresh_sql.commit();
    }


}

void MarketExecution::ProcessBuyVolumeEqualsSellVolume(Bid* cur_buy_bid, Bid* cur_sell_bid, int buy_quantity_not_filled, int sell_quantity_not_filled)
{
    int price_of_current_deal = cur_buy_bid > cur_sell_bid ? cur_sell_bid->price_ : cur_buy_bid->price_;

    cur_buy_bid->filled_ += sell_quantity_not_filled;
    cur_sell_bid->filled_ += sell_quantity_not_filled;

    cur_buy_bid->status_ = BidStatus::FILLED;
    cur_sell_bid->status_ = BidStatus::FILLED;


    db_.ChangeBalance(cur_buy_bid->owner_, -price_of_current_deal * sell_quantity_not_filled, sell_quantity_not_filled);
    db_.ChangeBalance(cur_sell_bid->owner_, price_of_current_deal * sell_quantity_not_filled, -sell_quantity_not_filled);

    current_sell_bids_.back().bid_queue_.pop_front();
    if (current_sell_bids_.back().bid_queue_.empty())
    {
        current_sell_bids_.pop_back();
    }

    current_buy_bids_.back().bid_queue_.pop_front();
    if (current_buy_bids_.back().bid_queue_.empty())
    {
        current_buy_bids_.pop_back();
    }

    if (SQL_connection_.has_value())
    {

        pqxx::work refresh_sql(SQL_connection_.value());
        refresh_sql.exec("UPDATE bid_history SET cur_status = \'FILLED\', filled = " + std::to_string(sell_quantity_not_filled) +
                         " WHERE bid_id = " +  std::to_string(GetBidId(cur_buy_bid)) + ";");

        refresh_sql.exec("UPDATE bid_history SET cur_status = \'FILLED\', filled = " + std::to_string(sell_quantity_not_filled) +
                          " WHERE bid_id = " +  std::to_string(GetBidId(cur_sell_bid)) + ";");

        refresh_sql.commit();
    }
}

void MarketExecution::ProcessSellVolumeMoreThanBuyVolume(Bid* cur_buy_bid, Bid* cur_sell_bid, int buy_quantity_not_filled, int sell_quantity_not_filled)
{
    int price_of_current_deal = cur_buy_bid > cur_sell_bid ? cur_sell_bid->price_ : cur_buy_bid->price_;
    cur_buy_bid->filled_ += buy_quantity_not_filled;
    cur_sell_bid->filled_ += buy_quantity_not_filled;

    cur_buy_bid->status_ = BidStatus::FILLED;
    cur_sell_bid->status_ = BidStatus::PARTIALLY_FILLED;


    db_.ChangeBalance(cur_buy_bid->owner_, -price_of_current_deal * buy_quantity_not_filled, buy_quantity_not_filled);
    db_.ChangeBalance(cur_sell_bid->owner_, price_of_current_deal * buy_quantity_not_filled, -buy_quantity_not_filled);


    current_buy_bids_.back().bid_queue_.pop_front();
    if (current_buy_bids_.back().bid_queue_.empty())
    {
        current_buy_bids_.pop_back();
    }

    if (SQL_connection_.has_value())
    {

        pqxx::work refresh_sql(SQL_connection_.value());
        refresh_sql.exec("UPDATE bid_history SET cur_status = \'FILLED\', filled = " + std::to_string(buy_quantity_not_filled) +
                         " WHERE bid_id = " +  std::to_string(GetBidId(cur_buy_bid)) + ";");
        refresh_sql.exec("UPDATE bid_history SET cur_status = \'PARTIALLY_FILLED\', filled = " + std::to_string(buy_quantity_not_filled) +
                          " WHERE bid_id = " +  std::to_string(GetBidId(cur_sell_bid)) + ";");
        refresh_sql.commit();
    }
}

const ServerDatabase& MarketExecution::GetServerDatabase() const
{
    return db_;
}
ServerDatabase& MarketExecution::GetServerDatabase()
{
    return db_;
}

std::vector<MarketExecution::Bid*> MarketExecution::GetUserBidHist(size_t id) const
{
    if (!bid_hist_per_user_.count(id))
    {
        return {};
    }
    return bid_hist_per_user_.at(id);
}

std::string GetStringBidType(MarketExecution::BidType type)
{
    if (type == MarketExecution::BidType::BUY)
    {
        return "BUY";
    } else {
        return "SELL";
    }
}


std::string GetStringStatus(MarketExecution::BidStatus status)
{
    switch (status) {
        case MarketExecution::BidStatus::ACTUAL:
            return "ACTUAL";
        case MarketExecution::BidStatus::FILLED:
            return "FILLED";
        case MarketExecution::BidStatus::PARTIALLY_FILLED:
            return "PARTIALLY FILLED";
        case MarketExecution::BidStatus::ABORTED:
            return "ABORTED";
        default:
            return "UNKNOWN";
    }
}


std::ostream& operator<<(std::ostream& out, MarketExecution::Bid bid)
{
    std::string status = GetStringStatus(bid.status_);
    std::string type = GetStringBidType(bid.type_);


    out << status << " | " << type << " " << bid.quantity_ << " $ at " << bid.price_ << " price. " <<
    bid.filled_ << "/" << bid.quantity_ << " filled." << std::endl;

    return out;
}

