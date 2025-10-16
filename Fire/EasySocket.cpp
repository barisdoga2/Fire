#include <ws2tcpip.h>

#include "EasySocket.hpp"
#include "EasyPeer.hpp"



EasySocket::EasySocket() : m_isBlocking(false), m_socket(INVALID_SOCKET)
{
	
}

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
	if (::bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
		return GetLastError();

	return WSAEISCONN;
}

void EasySocket::unbind()
{
	close();
}

uint64_t EasySocket::send(const void* data, const std::size_t& size, const EasyIpAddress& remoteAddress, const unsigned short& remotePort)
{
	create();

	sockaddr_in addr = makeSockAddr(remoteAddress, remotePort);
	const int sent = static_cast<int>(sendto(m_socket, static_cast<const char*>(data), static_cast<Size>(size), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));

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
	const int recv = static_cast<int>(recvfrom(m_socket, static_cast<char*>(data), static_cast<Size>(capacity), 0, reinterpret_cast<sockaddr*>(&sender), &addrLen));

	if (recv < 0)
		return getErrorStatus();

	received = static_cast<std::size_t>(recv);
	remoteAddress = EasyIpAddress(sender.sin_addr.s_addr);
	remotePort = ntohs(sender.sin_port);

	return WSAEISCONN;
}

uint64_t EasySocket::receive(void* data, const std::size_t& capacity, std::size_t& received, EasyPeer& peer)
{
	create();

	received = 0;

	int addrLen = static_cast<int>(peer.sockAddr.size());
	const int recv = static_cast<int>(recvfrom(m_socket, static_cast<char*>(data), static_cast<Size>(capacity), 0, reinterpret_cast<sockaddr*>(peer.sockAddr.data()), &addrLen));

	if (recv < 0)
		return getErrorStatus();

	sockaddr_in* addrIn = reinterpret_cast<sockaddr_in*>(peer.sockAddr.data());
	peer.addr = (addrIn->sin_addr.s_addr << 16U) | (addrIn->sin_port);
	received = static_cast<std::size_t>(recv);

	peer.ip = EasyIpAddress(addrIn->sin_addr.s_addr);
	peer.port = ntohs(addrIn->sin_port);

	return WSAEISCONN;
}

void EasySocket::setBlocking(bool block)
{
	if (m_socket != INVALID_SOCKET)
	{
		u_long blocking = block ? 0 : 1;
		ioctlsocket(m_socket, static_cast<long>(FIONBIO), &blocking);
		m_isBlocking = block;
	}
}

unsigned short EasySocket::getLocalPort()
{
	if (m_socket != INVALID_SOCKET)
	{
		sockaddr_in address{};
		AddrLength size = sizeof(address);
		if (getsockname(m_socket, reinterpret_cast<sockaddr*>(&address), &size) != -1)
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
		if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1)
		{
			std::cout << "Failed to enable broadcast on UDP socket" << std::endl;
		}
	}
}

void EasySocket::close()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}
}

struct SocketInitializer
{
	SocketInitializer()
	{
		WSADATA init;
		WSAStartup(MAKEWORD(2, 2), &init);
	}

	~SocketInitializer()
	{
		WSACleanup();
	}
};

SocketInitializer globalInitializer;