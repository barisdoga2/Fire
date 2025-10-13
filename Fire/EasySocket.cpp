#include "EasySocket.hpp"

#include <ws2tcpip.h>

EasySocket::EasySocket() : m_isBlocking(true), m_socket(INVALID_SOCKET)
{
	
}

static sockaddr_in createAddress(std::uint32_t address, unsigned short port)
{
	auto addr = sockaddr_in();
	addr.sin_addr.s_addr = htonl(address);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	return addr;
}

static EasySocket::EStatus getErrorStatus()
{

	switch (WSAGetLastError())
	{
		case WSAEWOULDBLOCK:  return EasySocket::EStatus::NotReady;
		case WSAEALREADY:     return EasySocket::EStatus::NotReady;
		case WSAECONNABORTED: return EasySocket::EStatus::Disconnected;
		case WSAECONNRESET:   return EasySocket::EStatus::Disconnected;
		case WSAETIMEDOUT:    return EasySocket::EStatus::Disconnected;
		case WSAENETRESET:    return EasySocket::EStatus::Disconnected;
		case WSAENOTCONN:     return EasySocket::EStatus::Disconnected;
		case WSAEISCONN:      return EasySocket::EStatus::Done;
		default:              return EasySocket::EStatus::Error;
	}

}

EasySocket::SocketHandle EasySocket::invalidSocket()
{
	return INVALID_SOCKET;
}

EasySocket::EStatus EasySocket::bind(unsigned short port, EasyIpAddress ip)
{
	close();

	create();

	sockaddr_in addr = createAddress(ip.toInteger(), port);
	if (::bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1)
	{
		std::cout << "Failed to bind socket to port " << port << std::endl;
		return EStatus::Error;
	}

	return EStatus::Done;
}

void EasySocket::unbind()
{
	close();
}

EasySocket::EStatus EasySocket::send(const void* data, std::size_t size, EasyIpAddress remoteAddress, unsigned short remotePort)
{
	create();

	if (size > MaxDatagramSize)
	{
		std::cout << "Cannot send data over the network " << "(the number of bytes to send is greater than sf::UDPSocket::MaxDatagramSize)" << std::endl;
		return EStatus::Error;
	}

	sockaddr_in address = createAddress(remoteAddress.toInteger(), remotePort);

	const int sent = static_cast<int>(
		sendto(m_socket, static_cast<const char*>(data), static_cast<Size>(size), 0, reinterpret_cast<sockaddr*>(&address), sizeof(address)));

	if (sent < 0)
		return getErrorStatus();

	return EStatus::Done;
}

EasySocket::EStatus EasySocket::receive(void* data, std::size_t size, std::size_t& received, std::optional<EasyIpAddress>& remoteAddress, unsigned short& remotePort)
{
	received = 0;
	remoteAddress = std::nullopt;
	remotePort = 0;

	if (!data)
	{
		std::cout << "Cannot receive data from the network (the destination buffer is invalid)" << std::endl;
		return EStatus::Error;
	}

	sockaddr_in address = createAddress(INADDR_ANY, 0);

	AddrLength addressSize = sizeof(address);
	const int sizeReceived = static_cast<int>(recvfrom(m_socket, static_cast<char*>(data), static_cast<Size>(size), 0, reinterpret_cast<sockaddr*>(&address), &addressSize));

	if (sizeReceived < 0)
		return getErrorStatus();

	received = static_cast<std::size_t>(sizeReceived);
	remoteAddress = EasyIpAddress(ntohl(address.sin_addr.s_addr));
	remotePort = ntohs(address.sin_port);

	return EStatus::Done;
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