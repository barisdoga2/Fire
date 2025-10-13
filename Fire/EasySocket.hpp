#pragma once

#include <optional>

#include "EasyIpAddress.hpp"


class EasySocket {
public:
	enum EStatus
	{
		Done,
		NotReady,
		Partial,
		Disconnected,
		Error,

		MAX
	};

private:
	using AddrLength = int;
	using Size = int;
	typedef unsigned __int64 SocketHandle;

	static constexpr size_t MaxDatagramSize{ 65507 };

	SocketHandle m_socket;
	bool m_isBlocking;

public:
	EasySocket();

	EStatus bind(unsigned short port, EasyIpAddress ip);

	void unbind();

	EStatus send(const void* data, size_t size, EasyIpAddress remoteAddress, unsigned short remotePort);

	EStatus receive(void* data, size_t size, size_t& received, std::optional<EasyIpAddress>& remoteAddress, unsigned short& remotePort);

	void setBlocking(bool block);

	unsigned short getLocalPort();

	static SocketHandle invalidSocket();

private:
	void create();

	void close();

};
