#include "ServerIO.h"
#include "json.hpp"
#include "Common.hpp"
#include "MarketExecution.h"
#include <pqxx/pqxx>
#include <fstream>

#include "Tests.h"

using boost::asio::ip::tcp;

Session::Session(boost::asio::io_service& io_service, MarketExecution& exec) : socket_(io_service), exec_(exec) {}

tcp::socket& Session::socket()
{
    return socket_;
}

void Session::start()
{
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
                            boost::bind(&Session::handle_read, this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void Session::handle_read(const boost::system::error_code& error,
                 size_t bytes_transferred)
{
    if (!error)
    {
        data_[bytes_transferred] = '\0';

        // Парсим json, который пришёл нам в сообщении.
        auto j = nlohmann::json::parse(data_);
        auto reqType = j["ReqType"];

        std::string reply = "Error! Unknown request type";

        if (reqType == Requests::Registration)
        {
            // Это реквест на регистрацию пользователя.
            // Добавляем нового пользователя и возвращаем его ID.
            reply = std::to_string(exec_.GetServerDatabase().RegisterNewUser(j["Message"]));
        }
        else if (reqType == Requests::ChangeBal)
        {
            size_t user_id = std::stoi(std::string(j["UserId"]));
            std::stringstream ss;
            ss << std::string(j["Message"]);

            int rub_change, usd_change;
            ss >> rub_change >> usd_change;

            auto get_bal_change_result = exec_.GetServerDatabase().ChangeBalance(user_id, rub_change, usd_change);
            if (get_bal_change_result.has_value())
            {
                reply = "Your balance was changed by  " + std::to_string(get_bal_change_result.value().RUB) +
                        " RUB, " + std::to_string(get_bal_change_result.value().USD) + " USD.\n";
            } else {
                reply = "Error! User unknown\n";
            }

        }
        else if (reqType == Requests::GetBal)
        {
            auto get_bal_result = exec_.GetServerDatabase().GetUserBalance(std::stoi(std::string(j["UserId"])));
            if (get_bal_result.has_value())
            {
                reply = "Your balance is " + std::to_string(get_bal_result.value().RUB) + " RUB, " +
                        std::to_string(get_bal_result.value().USD) + " USD.\n";
            } else {
                reply = "Error! User unknown\n";
            }
        }
        else if (reqType == Requests::Buy)
        {
            std::stringstream ss;
            ss << std::string(j["UserId"]) << " " << std::string(j["Message"]);
            size_t user_id;
            int price, quantity;
            ss >> user_id >> price >> quantity;

            MarketExecution::Bid new_bid {
                    MarketExecution::BidType::BUY,
                    user_id,
                    price,
                    quantity,
                    MarketExecution::BidStatus::ACTUAL,
                    0};

            exec_.AddBid(new_bid);
            std::stringstream answer;

            answer << "Buy bid for " << quantity << " dollars @ " << price << " accepted!\n";
            reply = answer.str();

        }
        else if (reqType == Requests::Sell)
        {
            std::stringstream ss;
            ss << std::string(j["UserId"]) << " " << std::string(j["Message"]);
            size_t user_id;
            int price, quantity;
            ss >> user_id >> price >> quantity;

            MarketExecution::Bid new_bid {
                    MarketExecution::BidType::SELL,
                    user_id,
                    price,
                    quantity,
                    MarketExecution::BidStatus::ACTUAL,
                    0};

            exec_.AddBid(new_bid);
            std::stringstream answer;

            answer << "Sell bid for " << quantity << " dollars @ " << price << " accepted!\n";
            reply = answer.str();
        }
        else if (reqType == Requests::BidsHist)
        {
            auto bids = exec_.GetUserBidHist(std::stoi(std::string(j["UserId"])));
            std::stringstream actual;
            std::stringstream history;

            for (auto it = bids.rbegin(); it != bids.rend(); ++it)
            {
                size_t cur_bid_id = exec_.GetBidId(*it);
                if ((*it)->status_ == (MarketExecution::BidStatus::ACTUAL) ||
                        (*it)->status_ == (MarketExecution::BidStatus::PARTIALLY_FILLED))
                {
                    actual << "ID: " << cur_bid_id << " | " << *(*it);
                } else {
                    history << "ID: " << cur_bid_id << " | " << *(*it);
                }
            }

            reply = "==========ACTUAL BIDS==========\n" + actual.str() + "\n==========BID HISTORY==========\n" + history.str();
        } else if (reqType == Requests::Abort)
        {
            size_t bid_to_abort = std::stoi(std::string(j["Message"]));
            bool success = exec_.AbortBid(bid_to_abort);

            if (success)
            {
                reply = "Bid " + std::to_string(bid_to_abort) + " successfully aborted.\n";
            } else {
                reply = "Error in abortion of bid " + std::to_string(bid_to_abort) + ".\n";
            }

        }

        boost::asio::async_write(socket_,
                                 boost::asio::buffer(reply, reply.size()),
                                 boost::bind(&Session::handle_write, this,
                                             boost::asio::placeholders::error));
    }
    else
    {
        delete this;
    }
}

void Session::handle_write(const boost::system::error_code& error)
{
    if (!error)
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                boost::bind(&Session::handle_read, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
    else
    {
        delete this;
    }
}

ServerIO::ServerIO(boost::asio::io_service& io_service)
: io_service_(io_service),
acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
{
    std::cout << "ServerIO started! Listen " << port << " port" << std::endl;

    Session* new_session = new Session(io_service_, exec_);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&ServerIO::handle_accept, this, new_session,
                                       boost::asio::placeholders::error));


}

ServerIO::ServerIO(boost::asio::io_service& io_service, std::string DB_connection_params) : io_service_(io_service),
acceptor_(io_service, tcp::endpoint(tcp::v4(), port)), exec_(DB_connection_params)
{
    std::cout << "ServerIO started! Listen " << port << " port" << std::endl;

    Session* new_session = new Session(io_service_, exec_);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&ServerIO::handle_accept, this, new_session,
                                       boost::asio::placeholders::error));
}

void ServerIO::handle_accept(Session* new_session,
                             const boost::system::error_code& error)
{
    if (!error)
    {
        new_session->start();
        new_session = new Session(io_service_, exec_);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&ServerIO::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }
    else
    {
        delete new_session;
    }
}


int main()
{
    {
        Tests met;
    }
    try
    {
        boost::asio::io_service io_service;

        ServerIO s(io_service, "dbname = stock-market user = postgres password = postgres hostaddr = 127.0.0.1 port = 5432");
    //  ServerIO s(io_service);


        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}

