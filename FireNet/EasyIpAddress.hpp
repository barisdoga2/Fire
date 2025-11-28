#pragma once
#include <iostream>


class EasyIpAddress {
public:
    EasyIpAddress();                                           // 0.0.0.0
    EasyIpAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);
    explicit EasyIpAddress(uint32_t addressNetworkOrder);      // already network order

    static const EasyIpAddress Any;
    static const EasyIpAddress LocalHost;
    static const EasyIpAddress Broadcast;

    uint32_t toInteger() const;                  // host order integer
    uint32_t asNetworkInteger() const; 
    std::string toString() const;                // "A.B.C.D"
    static EasyIpAddress resolve(const std::string& address); // from string/hostname

private:
    uint32_t m_address; // stored in network byte order

};