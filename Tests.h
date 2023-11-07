#pragma once
#include "test_framework.h"

class Tests
{
public:
    Tests();


private:
    static void TestAddBuyBid();
    static void TestAddSellBid();
    static void TestMatching();

    TestRunner tr_;

};