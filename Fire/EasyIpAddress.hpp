#pragma once

#include <iostream>
#include <optional>



class EasyIpAddress {
private:
	std::uint32_t m_address;
public:
	EasyIpAddress(std::uint8_t byte0, std::uint8_t byte1, std::uint8_t byte2, std::uint8_t byte3);

	explicit EasyIpAddress(std::uint32_t address);

	std::uint32_t toInteger() const;

	static std::optional<EasyIpAddress> resolve(std::string_view address);

};