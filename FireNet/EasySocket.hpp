#pragma once
#include <optional>
#include <vector>

#include "EasyNet.hpp"
#include "EasyIpAddress.hpp"

#ifndef WSAEWOULDBLOCK
#define WSABASEERR              10000
#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)
#define WSAEDESTADDRREQ         (WSABASEERR+39)
#define WSAEMSGSIZE             (WSABASEERR+40)
#define WSAEPROTOTYPE           (WSABASEERR+41)
#define WSAENOPROTOOPT          (WSABASEERR+42)
#define WSAEPROTONOSUPPORT      (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT      (WSABASEERR+44)
#define WSAEOPNOTSUPP           (WSABASEERR+45)
#define WSAEPFNOSUPPORT         (WSABASEERR+46)
#define WSAEAFNOSUPPORT         (WSABASEERR+47)
#define WSAEADDRINUSE           (WSABASEERR+48)
#define WSAEADDRNOTAVAIL        (WSABASEERR+49)
#define WSAENETDOWN             (WSABASEERR+50)
#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAENETRESET            (WSABASEERR+52)
#define WSAECONNABORTED         (WSABASEERR+53)
#define WSAECONNRESET           (WSABASEERR+54)
#define WSAENOBUFS              (WSABASEERR+55)
#define WSAEISCONN              (WSABASEERR+56)
#define WSAENOTCONN             (WSABASEERR+57)
#define WSAESHUTDOWN            (WSABASEERR+58)
#define WSAETOOMANYREFS         (WSABASEERR+59)
#define WSAETIMEDOUT            (WSABASEERR+60)
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAELOOP                (WSABASEERR+62)
#define WSAENAMETOOLONG         (WSABASEERR+63)
#define WSAEHOSTDOWN            (WSABASEERR+64)
#define WSAEHOSTUNREACH         (WSABASEERR+65)
#define WSAENOTEMPTY            (WSABASEERR+66)
#define WSAEPROCLIM             (WSABASEERR+67)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)
#define WSANO_DATA                       11004L
#endif



class EasyPeer;
class PeerInfo;
class EasySocket {
private:
	using AddrLength = int;
	using Size = int;
	typedef unsigned __int64 SocketHandle;

	static constexpr size_t MaxDatagramSize{ 65507 };

	SocketHandle m_socket;
	bool m_isBlocking;

public:
	
	EasySocket();

	uint64_t bind(unsigned short port, const EasyIpAddress& ip = EasyIpAddress::Any);

	void unbind();


	uint64_t send(const void* data, const size_t size, const EasyIpAddress& remoteAddress, const unsigned short& remotePort);
	uint64_t send(const void* data, const size_t size, uint64_t addrU64); // Server use only

	uint64_t receive(void* data, const size_t& size, size_t& received, EasyIpAddress& remoteAddress, unsigned short& remotePort);
	uint64_t receive(void* data, const std::size_t& capacity, std::size_t& received, PeerSocketInfo& info); // Server use only

	void setBlocking(bool block);

	unsigned short getLocalPort();

	static SocketHandle invalidSocket();

private:
	void create();

	void close();

};
