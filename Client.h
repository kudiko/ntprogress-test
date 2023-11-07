#pragma once

#include <iostream>
#include <boost/asio.hpp>

#include "Common.hpp"
#include "json.hpp"

using boost::asio::ip::tcp;

class Client
{
public:
    Client();
    void MenuFunction();



private:
    size_t my_id_;
    boost::asio::io_service io_service_;

    tcp::socket s_;




    // Отправка сообщения на сервер по шаблону.
    static void SendMessage(
            tcp::socket& aSocket,
            const std::string& aId,
            const std::string& aRequestType,
            const std::string& aMessage);


// Возвращает строку с ответом сервера на последний запрос.
    static std::string ReadMessage(tcp::socket& aSocket);


// "Создаём" пользователя, получаем его ID.
    static size_t ProcessRegistration(tcp::socket& aSocket);
};