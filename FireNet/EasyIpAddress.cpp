#include "pch.h"

#include <ws2tcpip.h>

#include "EasyIpAddress.hpp"


const EasyIpAddress EasyIpAddress::Any(0, 0, 0, 0);
const EasyIpAddress EasyIpAddress::LocalHost(127, 0, 0, 1);
const EasyIpAddress EasyIpAddress::Broadcast(255, 255, 255, 255);

EasyIpAddress::EasyIpAddress() : m_address(htonl(0)) {}

EasyIpAddress::EasyIpAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    uint32_t addrHost = (static_cast<uint32_t>(b0) << 24) |
        (static_cast<uint32_t>(b1) << 16) |
        (static_cast<uint32_t>(b2) << 8) |
        static_cast<uint32_t>(b3);
    m_address = htonl(addrHost); // convert to network order
}

EasyIpAddress::EasyIpAddress(uint32_t addressNetworkOrder)
    : m_address(addressNetworkOrder) {
}

uint32_t EasyIpAddress::toInteger() const
{
    return ntohl(m_address); // convert to host order
}

uint32_t EasyIpAddress::asNetworkInteger() const
{
    return m_address;
}

std::string EasyIpAddress::toString() const
{
    char buffer[INET_ADDRSTRLEN] = {};
    in_addr addr{};
    addr.s_addr = m_address; // already network order
    if (inet_ntop(AF_INET, &addr, buffer, sizeof(buffer)) == nullptr)
        return {};
    return std::string(buffer);
}

EasyIpAddress EasyIpAddress::resolve(const std::string& address)
{
    if (address.empty())
        return Any;

    in_addr addr{};
    if (inet_pton(AF_INET, address.c_str(), &addr) == 1)
        return EasyIpAddress(addr.s_addr); // already network order

    addrinfo hints{};
    hints.ai_family = AF_INET;

    addrinfo* result = nullptr;
    if (getaddrinfo(address.c_str(), nullptr, &hints, &result) == 0 && result)
    {
        sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(result->ai_addr);
        uint32_t net = sin->sin_addr.s_addr;
        freeaddrinfo(result);
        return EasyIpAddress(net);
    }

    return Any;
}
