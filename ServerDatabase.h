#pragma once

#include <optional>
#include <deque>
#include <unordered_map>
#include <map>
#include <string>



class ServerDatabase
{
public:
    struct Currencies
    {
        int RUB = 0;
        int USD = 0;
    };

    ServerDatabase() = default;
    // "Регистрирует" нового пользователя и возвращает его ID.
    size_t RegisterNewUser(const std::string& aUserName);

    // Запрос имени клиента по ID
    std::string GetUserName(std::size_t aUserId) const;

    std::optional<Currencies> GetUserBalance(std::size_t aUserId) const;

    std::optional<Currencies> ChangeBalance(std::size_t aUserId, int rub_change, int usd_change);

    ~ServerDatabase() = default;

private:

    std::deque<std::string> user_logins_;
    std::unordered_map<size_t, std::string_view> user_id_to_login_;
    std::unordered_map<std::string_view, size_t> user_login_to_id_;
    std::unordered_map<size_t, Currencies> balances_;

};