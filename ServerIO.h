#pragma once

#include <iostream>
#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>

#include "MarketExecution.h"

using boost::asio::ip::tcp;


class Session
{
public:
    Session(boost::asio::io_service& io_service, MarketExecution& exec);

    tcp::socket& socket();

    void start();

    // Обработка полученного сообщения.
    void handle_read(const boost::system::error_code& error,
                     size_t bytes_transferred);

    void handle_write(const boost::system::error_code& error);

private:
    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    MarketExecution& exec_;
};


class ServerIO
{
public:
    ServerIO(boost::asio::io_service& io_service);
    ServerIO(boost::asio::io_service& io_service, std::string DB_connection_params);

    void handle_accept(Session* new_session,
                       const boost::system::error_code& error);


private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;

    MarketExecution exec_;
};