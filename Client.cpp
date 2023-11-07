#include <iostream>
#include <boost/asio.hpp>

#include "Common.hpp"
#include "json.hpp"

#include "Client.h"

using boost::asio::ip::tcp;


Client::Client() : io_service_(), s_(io_service_)
{
    try
    {
        tcp::resolver resolver(io_service_);
        tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(port));
        tcp::resolver::iterator iterator = resolver.resolve(query);

        s_.connect(*iterator);

        // Мы предполагаем, что для идентификации пользователя будет использоваться ID.
        // Тут мы "регистрируем" пользователя - отправляем на сервер имя, а сервер возвращает нам ID.
        // Этот ID далее используется при отправке запросов.
        my_id_ = ProcessRegistration(s_);

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}

void Client::MenuFunction() {
        try {
            while (true)
            {
                // Тут реализовано "бесконечное" меню.
                std::cout << "Menu:\n"
                             "1) Get my balance \n"
                             "2) Deposit/withdraw funds \n"
                             "3) Buy USD \n"
                             "4) Sell USD \n"
                             "5) Get trade history \n"
                             "6) Abort bids \n"
                             "7) Exit \n"
                          << std::endl;

                short menu_option_num;
                std::cin >> menu_option_num;

                switch (menu_option_num) {
                    case 1: {
                        SendMessage(s_, std::to_string(my_id_), Requests::GetBal, "");
                        std::cout << ReadMessage(s_);
                        break;
                    }
                    case 2: {
                        int rub_change, usd_change;
                        std::cout << "Enter balance changes in form of \"RUB_change USD_change\" " << std::endl;
                        std::cin >> rub_change >> usd_change;
                        SendMessage(s_, std::to_string(my_id_), Requests::ChangeBal, std::to_string(rub_change) + " " + std::to_string(usd_change));
                        std::cout << ReadMessage(s_);
                        break;
                    }
                    case 3:
                    {
                        int price, quantity;
                        std::cout << "Enter price: " << std::endl;
                        std::cin >> price;
                        std::cout << "Enter quantity: " << std::endl;
                        std::cin >> quantity;
                        SendMessage(s_, std::to_string(my_id_), Requests::Buy, std::to_string(price) + " " +  std::to_string(quantity));
                        std::cout << ReadMessage(s_);
                        break;
                    }
                    case 4:
                    {
                        int price, quantity;
                        std::cout << "Enter price: " << std::endl;
                        std::cin >> price;
                        std::cout << "Enter quantity: " << std::endl;
                        std::cin >> quantity;
                        SendMessage(s_, std::to_string(my_id_), Requests::Sell, std::to_string(price) + " " + std::to_string(quantity));
                        std::cout << ReadMessage(s_);
                        break;
                    }
                    case 5:
                    {
                        SendMessage(s_, std::to_string(my_id_), Requests::BidsHist, "");
                        std::cout << ReadMessage(s_);
                        break;
                    }
                    case 6:
                    {
                        int id_to_abort;
                        std::cout << "Enter bid id to be aborted: " << std::endl;
                        std::cin >> id_to_abort;
                        SendMessage(s_, std::to_string(my_id_), Requests::Abort, std::to_string(id_to_abort));
                        std::cout << ReadMessage(s_);
                        break;
                    }
                    case 7:
                    {
                        exit(0);
                        break;
                    }
                    default:
                    {
                        std::cout << "Unknown menu option\n" << std::endl;
                    }
                }
            }
        } catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
}

void Client::SendMessage(tcp::socket &aSocket, const std::string &aId, const std::string &aRequestType,
                                const std::string &aMessage)
{
    nlohmann::json req;
    req["UserId"] = aId;
    req["ReqType"] = aRequestType;
    req["Message"] = aMessage;

    std::string request = req.dump();
    boost::asio::write(aSocket, boost::asio::buffer(request, request.size()));
}

std::string Client::ReadMessage(tcp::socket &aSocket)
{
    boost::asio::streambuf b;
    boost::asio::read_until(aSocket, b, "\0");
    std::istream is(&b);
    std::string line(std::istreambuf_iterator<char>(is), {});

    return line;
}



size_t Client::ProcessRegistration(tcp::socket& aSocket)
{
    std::string name;
    std::cout << "Hello! Enter your name: ";
    std::cin >> name;

    // Для регистрации Id не нужен, заполним его нулём
    SendMessage(aSocket, "0", Requests::Registration, name);
    return std::stoi(ReadMessage(aSocket));
}




int main()
{
    Client client_instance;
    client_instance.MenuFunction();

    return 0;
}