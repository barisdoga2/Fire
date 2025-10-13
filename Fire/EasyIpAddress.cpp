#include "EasyIpAddress.hpp"

#include <ws2tcpip.h>


EasyIpAddress::EasyIpAddress(std::uint8_t byte0, std::uint8_t byte1, std::uint8_t byte2, std::uint8_t byte3) : m_address(htonl(static_cast<std::uint32_t>((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3)))
{}

EasyIpAddress::EasyIpAddress(std::uint32_t address) : m_address(htonl(address))
{}

std::uint32_t EasyIpAddress::toInteger() const
{
	return ntohl(m_address);
}

std::optional<EasyIpAddress> EasyIpAddress::resolve(std::string_view address)
{
	static const EasyIpAddress Any(0, 0, 0, 0);
	static const EasyIpAddress LocalHost(127, 0, 0, 1);
	static const EasyIpAddress Broadcast(255, 255, 255, 255);

	using namespace std::string_view_literals;

	if (address.empty())
		return std::nullopt;

	if (address == "255.255.255.255"sv)
	{
		return Broadcast;
	}

	if (address == "0.0.0.0"sv)
		return Any;

	std::uint32_t ip;
	inet_pton(AF_INET, address.data(), &ip);
	if (ip != INADDR_NONE)
		return EasyIpAddress(ntohl(ip));


	addrinfo hints{};
	hints.ai_family = AF_INET;

	addrinfo* result = nullptr;
	if (getaddrinfo(address.data(), nullptr, &hints, &result) == 0 && result != nullptr)
	{
		sockaddr_in sin{};
		std::memcpy(&sin, result->ai_addr, sizeof(*result->ai_addr));

		const std::uint32_t ip = sin.sin_addr.s_addr;
		freeaddrinfo(result);

		return EasyIpAddress(ntohl(ip));
	}

	return std::nullopt;
}