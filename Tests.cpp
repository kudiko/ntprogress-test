#include "Tests.h"

#include "MarketExecution.h"

Tests::Tests()
{
    RUN_TEST(tr_, TestAddBuyBid);
    RUN_TEST(tr_, TestAddSellBid);
    RUN_TEST(tr_, TestMatching);

}


void Tests::TestAddBuyBid()
{
    MarketExecution exec;

    Assert(exec.current_buy_bids_.empty() && exec.current_sell_bids_.empty() && exec.bid_history_.empty(), "Empty exec test");

    MarketExecution::Bid first {
            MarketExecution::BidType::BUY,
            1,
            50,
            1000,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(first);

    Assert(!exec.bid_history_.empty() && exec.bid_history_.back().owner_ == 1 && exec.bid_history_.back().price_ == 50,
           "Buy bid added to queue");

    Assert(!exec.current_buy_bids_.empty() && exec.current_buy_bids_.back().price_ == 50 &&
    exec.current_buy_bids_.back().bid_queue_.back() == &exec.bid_history_.back(), "Buy bid added to current buy bids");


    MarketExecution::Bid second {
            MarketExecution::BidType::BUY,
            2,
            55,
            250,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(second);

    Assert( exec.bid_history_.front().owner_ == 1 && exec.bid_history_.back().owner_ == 2 && exec.bid_history_.back().price_ == 55,
           "The greatest buy bid added to queue");

    Assert( exec.current_buy_bids_.size() == 2 &&
            exec.current_buy_bids_.back().price_ == 55 && exec.current_buy_bids_.front().price_ == 50 &&
            exec.current_buy_bids_.front().bid_queue_.back() == &exec.bid_history_.front() &&
           exec.current_buy_bids_.back().bid_queue_.back() == &exec.bid_history_.back(),
           "Buy bid with the greatest price correctly added to current buy bids");

    MarketExecution::Bid third {
            MarketExecution::BidType::BUY,
            3,
            1,
            125,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(third);

    Assert( exec.bid_history_.front().owner_ == 1 && exec.bid_history_.back().owner_ == 3
    && exec.bid_history_[1].owner_ == 2 && exec.bid_history_.back().price_ == 1,
            "The least buy bid added to queue");


    Assert( exec.current_buy_bids_.size() == 3 &&
            exec.current_buy_bids_.back().price_ == 55 && exec.current_buy_bids_[1].price_ == 50 &&
            exec.current_buy_bids_.front().price_ == 1 &&
            exec.current_buy_bids_.front().bid_queue_.back() == &exec.bid_history_[2] &&
            exec.current_buy_bids_.back().bid_queue_.back() == &exec.bid_history_[1] &&
            exec.current_buy_bids_[1].bid_queue_.back() == &exec.bid_history_[0],
            "Buy bid with the least price correctly added to current buy bids");

    MarketExecution::Bid fourth {
            MarketExecution::BidType::BUY,
            4,
            50,
            125,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(fourth);

    Assert( exec.bid_history_.back().owner_ == 4 && exec.bid_history_.back().price_ == 50,
            "Buy bid with already existing price added to queue");

    Assert( exec.current_buy_bids_.size() == 3 &&
             exec.current_buy_bids_[1].price_ == 50 &&
            exec.current_buy_bids_[1].bid_queue_.back() == &exec.bid_history_[3] &&
            exec.current_buy_bids_[1].bid_queue_.front() == &exec.bid_history_[0],
            "Buy bid with the least price correctly added to current buy bids");

}

void Tests::TestAddSellBid()
{
    MarketExecution exec;

    MarketExecution::Bid first {
            MarketExecution::BidType::SELL,
            1,
            50,
            1000,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(first);

    Assert(!exec.bid_history_.empty() && exec.bid_history_.back().owner_ == 1 && exec.bid_history_.back().price_ == 50 &&
    exec.current_buy_bids_.empty(),"Sell bid added to queue");

    Assert(!exec.current_sell_bids_.empty() && exec.current_sell_bids_.back().price_ == 50 &&
           exec.current_sell_bids_.back().bid_queue_.back() == &exec.bid_history_.back(), "Buy bid added to current sell bids");


    MarketExecution::Bid second {
            MarketExecution::BidType::SELL,
            2,
            55,
            250,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(second);

    Assert( exec.bid_history_.front().owner_ == 1 && exec.bid_history_.back().owner_ == 2 && exec.bid_history_.back().price_ == 55,
            "The greatest sell bid added to queue");

    Assert( exec.current_sell_bids_.size() == 2 &&
            exec.current_sell_bids_.back().price_ == 50 && exec.current_sell_bids_.front().price_ == 55 &&
            exec.current_sell_bids_.front().bid_queue_.back() == &exec.bid_history_.back() &&
            exec.current_sell_bids_.back().bid_queue_.back() == &exec.bid_history_.front(),
            "Sell bid with the greatest price correctly added to current sell bids");

    MarketExecution::Bid third {
            MarketExecution::BidType::SELL,
            3,
            1,
            125,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(third);

    Assert( exec.bid_history_.front().owner_ == 1 && exec.bid_history_.back().owner_ == 3
            && exec.bid_history_[1].owner_ == 2 && exec.bid_history_.back().price_ == 1,
            "The least sell bid added to queue");


    Assert( exec.current_sell_bids_.size() == 3 &&
            exec.current_sell_bids_.back().price_ == 1 && exec.current_sell_bids_[1].price_ == 50 &&
            exec.current_sell_bids_.front().price_ == 55 &&
            exec.current_sell_bids_.back().bid_queue_.back() == &exec.bid_history_[2] &&
            exec.current_sell_bids_.front().bid_queue_.back() == &exec.bid_history_[1] &&
            exec.current_sell_bids_[1].bid_queue_.back() == &exec.bid_history_[0],
            "Sell bid with the least price correctly added to current sell bids");

    MarketExecution::Bid fourth {
            MarketExecution::BidType::SELL,
            4,
            1,
            125,
            MarketExecution::BidStatus::ACTUAL,
            0
    };

    exec.AddBid(fourth);

    Assert( exec.bid_history_.back().owner_ == 4,
            "Sell bid with already existing price added to queue");

    Assert( exec.current_sell_bids_.size() == 3 &&
            exec.current_sell_bids_.back().price_ == 1 &&
            exec.current_sell_bids_.back().bid_queue_.front() == &exec.bid_history_[2] &&
            exec.current_sell_bids_.back().bid_queue_.back() == &exec.bid_history_[3],
            "Sell bid with existing price correctly added to current sell bids");
}

void Tests::TestMatching()
{
    // Buy volume = sell volume
    {

        MarketExecution exec;
        ServerDatabase& DB = exec.GetServerDatabase();
        DB.RegisterNewUser("FirstUser");
        DB.RegisterNewUser("SecondUser");

        MarketExecution::Bid first {
                MarketExecution::BidType::BUY,
                0,
                50,
                100,
                MarketExecution::BidStatus::ACTUAL,
                0
        };

        MarketExecution::Bid second {
                MarketExecution::BidType::SELL,
                1,
                49,
                100,
                MarketExecution::BidStatus::ACTUAL,
                0
        };

        exec.AddBid(first);
        exec.AddBid(second);

        Assert( exec.bid_history_.front().owner_ == 0 && exec.bid_history_.back().owner_ == 1
                && exec.bid_history_.front().status_ == MarketExecution::BidStatus::FILLED
                && exec.bid_history_.back().status_ == MarketExecution::BidStatus::FILLED,
                "Bids with same quantity matched and processed correctly");

        Assert(exec.current_buy_bids_.empty() && exec.current_sell_bids_.empty(), "Prices with no bids were deleted");

        Assert(exec.GetServerDatabase().GetUserBalance(first.owner_).value().RUB == -5000 &&
               exec.GetServerDatabase().GetUserBalance(first.owner_).value().USD == 100,
               "First person has right balances after buy = sell volume test");

        Assert(exec.GetServerDatabase().GetUserBalance(second.owner_).value().RUB == 5000 &&
               exec.GetServerDatabase().GetUserBalance(second.owner_).value().USD == -100,
               "Second person has right balances after buy   sell volume test");

    }

    // Test buy volume more than sell volume
    {
        MarketExecution exec;
        ServerDatabase& DB = exec.GetServerDatabase();
        DB.RegisterNewUser("FirstUser");
        DB.RegisterNewUser("SecondUser");

        MarketExecution::Bid first {
                MarketExecution::BidType::BUY,
                0,
                62,
                100,
                MarketExecution::BidStatus::ACTUAL,
                0
        };
        exec.AddBid(first);

        MarketExecution::Bid second {
                MarketExecution::BidType::SELL,
                1,
                61,
                30,
                MarketExecution::BidStatus::ACTUAL,
                0
        };
        exec.AddBid(second);

        Assert( exec.bid_history_.front().owner_ == 0 &&
                exec.bid_history_.front().status_ == MarketExecution::BidStatus::PARTIALLY_FILLED &&
                exec.bid_history_.front().filled_ == second.quantity_ &&

                exec.bid_history_[1].owner_ == 1 &&
                exec.bid_history_[1].status_ == MarketExecution::BidStatus::FILLED &&
                exec.bid_history_[1].filled_ == exec.bid_history_[1].quantity_ ,

                "Bids from test buy > sell have been correctly matched");

        Assert(!exec.current_buy_bids_.empty() && exec.current_sell_bids_.empty(), "Correctly deleted prices with no bids during buy > sell test");

        Assert(exec.GetServerDatabase().GetUserBalance(first.owner_).value().RUB == -1860 &&
               exec.GetServerDatabase().GetUserBalance(first.owner_).value().USD == 30,
               "First person has right balances after buy > sell test");

        Assert(exec.GetServerDatabase().GetUserBalance(second.owner_).value().RUB == 1860 &&
               exec.GetServerDatabase().GetUserBalance(second.owner_).value().USD == -30,
               "Second person has right balances after buy > sell test");
    }

    // Test cell volume more than buy volume
    {
        MarketExecution exec;
        ServerDatabase& DB = exec.GetServerDatabase();
        DB.RegisterNewUser("FirstUser");
        DB.RegisterNewUser("SecondUser");

        MarketExecution::Bid first {
                MarketExecution::BidType::SELL,
                0,
                60,
                100,
                MarketExecution::BidStatus::ACTUAL,
                0
        };
        exec.AddBid(first);

        MarketExecution::Bid second {
                MarketExecution::BidType::BUY,
                1,
                61,
                30,
                MarketExecution::BidStatus::ACTUAL,
                0
        };
        exec.AddBid(second);

        Assert( exec.bid_history_.front().owner_ == 0 &&
                exec.bid_history_.front().status_ == MarketExecution::BidStatus::PARTIALLY_FILLED &&
                exec.bid_history_.front().filled_ == second.quantity_ &&

                exec.bid_history_[1].owner_ == 1 &&
                exec.bid_history_[1].status_ == MarketExecution::BidStatus::FILLED &&
                exec.bid_history_[1].filled_ == exec.bid_history_[1].quantity_ ,

                "Bids from test sell > buy have been correctly matched");

        Assert(exec.current_buy_bids_.empty() && !exec.current_sell_bids_.empty(), "Correctly deleted prices with no bids during sell > buy test");

        Assert(exec.GetServerDatabase().GetUserBalance(first.owner_).value().RUB == 1800 &&
               exec.GetServerDatabase().GetUserBalance(first.owner_).value().USD == -30,
               "First person has right balances after sell > buy test");

        Assert(exec.GetServerDatabase().GetUserBalance(second.owner_).value().RUB == -1800 &&
               exec.GetServerDatabase().GetUserBalance(second.owner_).value().USD == 30,
               "Second person has right balances after sell > buy test");
    }

    // Test from the task
    {
        MarketExecution exec;
        ServerDatabase& DB = exec.GetServerDatabase();
        DB.RegisterNewUser("FirstUser");
        DB.RegisterNewUser("SecondUser");
        DB.RegisterNewUser("ThirdUser");

        MarketExecution::Bid first {
                MarketExecution::BidType::BUY,
                0,
                62,
                10,
                MarketExecution::BidStatus::ACTUAL,
                0
        };
        exec.AddBid(first);

        MarketExecution::Bid second {
                MarketExecution::BidType::BUY,
                1,
                63,
                20,
                MarketExecution::BidStatus::ACTUAL,
                0
        };
        exec.AddBid(second);

        MarketExecution::Bid third {
                MarketExecution::BidType::SELL,
                2,
                61,
                50,
                MarketExecution::BidStatus::ACTUAL,
                0
        };
        exec.AddBid(third);

        Assert( exec.bid_history_.front().owner_ == 0 &&
        exec.bid_history_.front().status_ == MarketExecution::BidStatus::FILLED &&
        exec.bid_history_.front().filled_ == exec.bid_history_.front().quantity_ &&

        exec.bid_history_[1].owner_ == 1 &&
        exec.bid_history_[1].status_ == MarketExecution::BidStatus::FILLED &&
        exec.bid_history_[1].filled_ == exec.bid_history_[1].quantity_ &&

        exec.bid_history_[2].owner_ == 2 &&
        exec.bid_history_[2].status_ == MarketExecution::BidStatus::PARTIALLY_FILLED &&
        exec.bid_history_[2].filled_ == exec.bid_history_.front().quantity_ + exec.bid_history_[1].quantity_,
        "Bids from task test have been correctly matched");

        Assert(exec.current_buy_bids_.empty() && !exec.current_sell_bids_.empty(), "Correctly deleted prices with no bids during task test");

        Assert(exec.GetServerDatabase().GetUserBalance(first.owner_).value().RUB == -620 &&
                       exec.GetServerDatabase().GetUserBalance(first.owner_).value().USD == 10,
                       "First person has right balances after task test");

        Assert(exec.GetServerDatabase().GetUserBalance(second.owner_).value().RUB == -1260 &&
               exec.GetServerDatabase().GetUserBalance(second.owner_).value().USD == 20,
               "Second person has right balances after task test");

        Assert(exec.GetServerDatabase().GetUserBalance(third.owner_).value().RUB == 1880 &&
               exec.GetServerDatabase().GetUserBalance(third.owner_).value().USD == -30,
               "Third person has right balances after task test");
    }



}