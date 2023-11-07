#include "ServerDatabase.h"

#include <sstream>
#include <iostream>

size_t ServerDatabase::RegisterNewUser(const std::string& aUserName)
{
    if (user_login_to_id_.count(aUserName))
    {
        return user_login_to_id_[aUserName];
    }
    size_t newUserId = user_id_to_login_.size();
    user_logins_.push_back(aUserName);

    user_id_to_login_[newUserId] = user_logins_.back();
    user_login_to_id_[user_logins_.back()] = newUserId;
    balances_[newUserId] = {0, 0};

    return newUserId;
}

std::string ServerDatabase::GetUserName(std::size_t aUserId) const
{
    if (user_id_to_login_.count(aUserId))
    {
        return std::string(user_id_to_login_.at(aUserId));
    }
    else {
        return "Error! Unknown User";
    }
}

std::optional<ServerDatabase::Currencies> ServerDatabase::GetUserBalance(std::size_t aUserId) const
{
    if (balances_.count(aUserId))
    {
        return balances_.at(aUserId);
    }
    return std::nullopt;
}

std::optional<ServerDatabase::Currencies> ServerDatabase::ChangeBalance(std::size_t aUserId,
                                                                        int rub_change, int usd_change)
{
    if (!balances_.count(aUserId))
    {
        return std::nullopt;
    }

    balances_[aUserId].RUB += rub_change;
    balances_[aUserId].USD += usd_change;

    return {Currencies{rub_change, usd_change}};
}