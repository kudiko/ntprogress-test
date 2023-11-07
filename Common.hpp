#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555;

namespace Requests
{
    static std::string Registration = "Reg";
    static std::string GetBal = "Bal";
    static std::string ChangeBal = "ChBal";
    static std::string Buy = "Buy";
    static std::string Sell = "Sell";
    static std::string BidsHist = "BidsHist";
    static std::string Abort = "Abort";
}

#endif //CLIENSERVERECN_COMMON_HPP
