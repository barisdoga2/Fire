#include "pch.h"

#include <ws2tcpip.h>

#include "EasySocket.hpp"
#include "EasyIpAddress.hpp"

static sockaddr_in makeSockAddr(const EasyIpAddress& ip, unsigned short port)
{
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ip.toInteger() == 0 ? INADDR_ANY : ip.toInteger();
	addr.sin_addr.s_addr = ip.toInteger() ? ip.toInteger() : INADDR_ANY;
	addr.sin_addr.s_addr = ip.toInteger() ? htonl(ip.toInteger()) : INADDR_ANY;
	addr.sin_port = htons(port);
	return addr;
}



EasySocket::EasySocket() : m_isBlocking(false), m_socket(INVALID_SOCKET)
{
	
}

std::string EasySocket::AddrToString(const Addr_t& addr)
{
	// Extract parts
	uint32_t ip_net = static_cast<uint32_t>(addr >> 16);
	uint16_t port_net = static_cast<uint16_t>(addr & 0xFFFF);

	// Convert to host order
	uint32_t ip_host = ntohl(ip_net);
	uint16_t port_host = ntohs(port_net);

	in_addr ipAddr{};
	ipAddr.s_addr = htonl(ip_host);

	char ipStr[INET_ADDRSTRLEN]{};
	inet_ntop(AF_INET, &ipAddr, ipStr, sizeof(ipStr));

	return std::string(ipStr) + ":" + std::to_string(port_host);
}

static uint64_t getErrorStatus()
{
	return WSAGetLastError();
}

EasySocket::SocketHandle EasySocket::invalidSocket()
{
	return INVALID_SOCKET;
}

uint64_t EasySocket::bind(unsigned short port, const EasyIpAddress& ip)
{
	close();

	create();

	sockaddr_in addr = makeSockAddr(ip, port);
	if (::bind((SOCKET)m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
		return GetLastError();

	return WSAEISCONN;
}

void EasySocket::unbind()
{
	close();
}

uint64_t EasySocket::send(const void* data, const std::size_t size, const EasyIpAddress& remoteAddress, const unsigned short& remotePort)
{
	create();

	sockaddr_in addr = makeSockAddr(remoteAddress, remotePort);
	const int sent = static_cast<int>(sendto((SOCKET)m_socket, static_cast<const char*>(data), static_cast<Size>(size), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));

	if (sent < 0)
		return getErrorStatus();

	return WSAEISCONN;
}

uint64_t EasySocket::send(const void* data, const std::size_t size, uint64_t addrU64) // Server use only
{
	create();

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = static_cast<uint16_t>(addrU64 & 0xFFFF);
	addr.sin_addr.s_addr = static_cast<uint32_t>(addrU64 >> 16);

	const int sent = static_cast<int>(sendto((SOCKET)m_socket, static_cast<const char*>(data), static_cast<Size>(size), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));

	if (sent < 0)
		return getErrorStatus();

	return WSAEISCONN;
}

uint64_t EasySocket::receive(void* data, const std::size_t& capacity, std::size_t& received, EasyIpAddress& remoteAddress, unsigned short& remotePort)
{
	create();

	received = 0;
	remotePort = 0;
	remoteAddress = EasyIpAddress::Any;

	sockaddr_in sender{};
	int addrLen = sizeof(sender);
	const int recv = static_cast<int>(recvfrom((SOCKET)m_socket, static_cast<char*>(data), static_cast<Size>(capacity), 0, reinterpret_cast<sockaddr*>(&sender), &addrLen));

	if (recv < 0)
		return getErrorStatus();

	received = static_cast<std::size_t>(recv);
	remoteAddress = EasyIpAddress(sender.sin_addr.s_addr);
	remotePort = ntohs(sender.sin_port);

	return WSAEISCONN;
}

uint64_t EasySocket::receive(void* data, const std::size_t& capacity, std::size_t& received, Addr_t& addr_in)  // Server use only
{
	create();

	received = 0;

	sockaddr_in addr{};
	int addr_size = sizeof(sockaddr_in);
	const int recv = static_cast<int>(recvfrom((SOCKET)m_socket, static_cast<char*>(data), static_cast<Size>(capacity), 0, reinterpret_cast<sockaddr*>(&addr), &addr_size));

	if (recv < 0)
		return getErrorStatus();

	received = static_cast<std::size_t>(recv);

	addr_in = (static_cast<uint64_t>(addr.sin_addr.s_addr) << 16) | addr.sin_port;

	return WSAEISCONN;
}

void EasySocket::setBlocking(bool block)
{
	if (m_socket != INVALID_SOCKET)
	{
		u_long blocking = block ? 0 : 1;
		ioctlsocket((SOCKET)m_socket, static_cast<long>(FIONBIO), &blocking);
		m_isBlocking = block;
	}
}

unsigned short EasySocket::getLocalPort()
{
	if (m_socket != INVALID_SOCKET)
	{
		sockaddr_in address{};
		AddrLength size = sizeof(address);
		if (getsockname((SOCKET)m_socket, reinterpret_cast<sockaddr*>(&address), &size) != -1)
		{
			return ntohs(address.sin_port);
		}
	}
	return 0;
}

void EasySocket::create()
{
	if (m_socket == INVALID_SOCKET)
	{
		const SocketHandle handle = socket(PF_INET, SOCK_DGRAM, 0);

		if (handle == INVALID_SOCKET)
		{
			std::cout << "Failed to create socket" << std::endl;
			return;
		}

		m_socket = handle;

		setBlocking(m_isBlocking);

		int yes = 1;
		if (setsockopt((SOCKET)m_socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1)
		{
			std::cout << "Failed to enable broadcast on UDP socket" << std::endl;
		}
	}
}

void EasySocket::close()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket((SOCKET)m_socket);
		m_socket = INVALID_SOCKET;
	}
}

struct SocketInitializer
{
	SocketInitializer()
	{
		WSADATA init;
		(void)WSAStartup(MAKEWORD(2, 2), &init);
	}

	~SocketInitializer()
	{
		//WSACleanup();
	}
};

SocketInitializer globalInitializer;