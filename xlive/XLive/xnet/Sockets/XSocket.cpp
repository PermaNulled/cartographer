#include "stdafx.h"

#include "XSocket.h"
#include "XLive/xnet/upnp.h"
#include "XLive/xnet/IpManagement/XnIp.h"
#include "H2MOD/Modules/Shell/Config.h"

#include "XLive/xnet/net_utils.h"

XSocketManager g_XSockMgr;

// #5310: XOnlineStartup
int WINAPI XOnlineStartup()
{
	LOG_TRACE_NETWORK("XOnlineStartup()");
	return ERROR_SUCCESS;
}

// #1: XWSAStartup
int WINAPI XWSAStartup(WORD wVersionRequested, LPWSADATA lpWsaData)
{
	LOG_TRACE_NETWORK("XWSAStartup({:x}, {:p})", wVersionRequested, (void*)lpWsaData);
	int result = WSAStartup(wVersionRequested, lpWsaData);
	return result;
}

// #2: XWSACleanup
int WINAPI XWSACleanup()
{
	LOG_TRACE_NETWORK("XWSACleanup()");
	return WSACleanup();
}

// #3: XSocketCreate
SOCKET WINAPI XSocketCreate(int af, int type, int protocol)
{
	LOG_TRACE_NETWORK("XSocketCreate() - af = {0}, type = {1}, protocol = {2}", af, type, protocol);

	if (af != AF_INET)
		return SOCKET_ERROR;

	// TODO: add TCP support
	if (protocol != IPPROTO_UDP
		&& protocol != IPPROTO_VDP)
		return SOCKET_ERROR;

	bool isVoice = false;
	if (protocol == IPPROTO_VDP)
	{
		isVoice = true;
		protocol = IPPROTO_UDP; // We can't support VDP (Voice / Data Protocol) it's some encrypted crap which isn't standard.
	}

	// TODO: use an array of sockets rather than allocating the memory on heap
	XVirtualSocket* newXSocket = new XVirtualSocket(protocol, isVoice);

#if XSOCK_USING_STANDARD_APIS
	SOCKET ret = socket(af, type, protocol);
#else
	SOCKET ret = WSASocket(af, type, protocol, 0, 0, 0);
#endif

	if (ret == INVALID_SOCKET)
	{
		LOG_ERROR_NETWORK("XSocketCreate() - Invalid socket, last error: {}", WSAGetLastError());
		delete newXSocket;
		return INVALID_SOCKET;
	}

	LOG_TRACE_NETWORK("XSocketCreate() - Socket created with descriptor: {}.", ret);
	newXSocket->systemSocketHandle = ret;

	if (newXSocket->isVoiceProtocol)
	{
		LOG_TRACE_NETWORK("XSocketCreate() - Socket: {} is VDP protocol", ret);
	}

	// disable SIO_UDP_CONNRESET
	// TODO re-enable this if the issue https://github.com/pnill/cartographer/issues/320 is not caused by this

	/*DWORD ioctlSetting = 0;
	DWORD cbBytesReturned;
	if (WSAIoctl(newXSocket->systemSocketHandle, SIO_UDP_CONNRESET, &ioctlSetting, 4u, 0, 0, &cbBytesReturned, 0, 0) == SOCKET_ERROR)
	{
		LOG_ERROR_NETWORK("XSocketCreate() - couldn't disable SIO_UDP_CONNRESET", ret);
	}
	else
	{
		LOG_TRACE_NETWORK("XSocketCreate() - disabled SIO_UDP_CONNRESET");
	}*/

	g_XSockMgr.sockets.push_back(newXSocket);

	/*newXSocket->SetBufferSize(SO_SNDBUF, gXnIpMgr.GetMinSockSendBufferSizeInBytes());
	newXSocket->SetBufferSize(SO_RCVBUF, gXnIpMgr.GetMinSockRecvBufferSizeInBytes());*/

	return (SOCKET)newXSocket;
}

// #5: XSocketShutdown
int WINAPI XSocketShutdown(SOCKET s, int how)
{
	LOG_TRACE_NETWORK("XSocketShutdown");
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	return shutdown(xsocket->systemSocketHandle, how);
}

// #6: XSocketIOCTLSocket
int WINAPI XSocketIOCTLSocket(SOCKET s, long cmd, u_long* argp)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;

	LOG_TRACE_NETWORK("XSocketIOCTLSocket() - cmd: {}", IOCTLSocket_cmd_string(cmd).c_str());
	int ret = ioctlsocket(xsocket->systemSocketHandle, cmd, argp);

	if (ret == NO_ERROR)
	{
		if (cmd == FIONBIO
			&& *argp == TRUE)
		{
			LOG_TRACE_NETWORK("XSocketIOCTLSocket() - setting default buffer size for non-blocking socket.");
			// set socket send/recv buffers size, but only if the socket isn't blocking
			xsocket->SetBufferSize(SO_SNDBUF, gXnIpMgr.GetMinSockSendBufferSizeInBytes());
			xsocket->SetBufferSize(SO_RCVBUF, gXnIpMgr.GetMinSockRecvBufferSizeInBytes());

			// remove last error even if we didn't successfully increased the recv/send buffer size
			WSASetLastError(0);
		}
	}

	return ret;
}

// #7: XSocketSetSockOpt
int WINAPI XSocketSetSockOpt(SOCKET s, int level, int optname, const char* optval, int optlen)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;

	LOG_TRACE_NETWORK("XSocketSetSockOpt  (socket = {:x}, level = {}, optname = {},  optlen = {})",
		xsocket->systemSocketHandle, level, sockOpt_string(optname), optlen);

	if (optname == SO_SNDBUF
		|| optname == SO_RCVBUF)
	{
		int bufferSize = *(int*)(optval);
		return xsocket->SetBufferSize(optname, bufferSize);
	}

	if (optname == SO_BROADCAST)
	{
		xsocket->isBroadcast = *(bool*)optval;
		return 0;
	}

	int ret = setsockopt(xsocket->systemSocketHandle, level, optname, optval, optlen);
	if (ret == SOCKET_ERROR)
	{
		LOG_TRACE_NETWORK("XSocketSetSockOpt() - error: {}", WSAGetLastError());
	}

	LOG_TRACE_NETWORK("- ret = {:x}", ret);
	return ret;
}

// #8: XSocketGetSockOpt
int WINAPI XSocketGetSockOpt(SOCKET s, int level, int optname, char* optval, int* optlen)
{
	LOG_TRACE_NETWORK("XSocketGetSockOpt()");
	XVirtualSocket* xsocket = (XVirtualSocket*)s;

	if (optname == SO_BROADCAST)
	{
		*(bool*)(optval) = xsocket->IsBroadcast();
		return 0;
	}

	return getsockopt(xsocket->systemSocketHandle, level, optname, optval, optlen);
}

// #9: XSocketGetSockName
int WINAPI XSocketGetSockName(SOCKET s, struct sockaddr* name, int* namelen)
{
	LOG_TRACE_NETWORK("XSocketGetSockName()");
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	return getsockname(xsocket->systemSocketHandle, name, namelen);
}

// #10
int WINAPI XSocketGetPeerName(SOCKET s, struct sockaddr* name, int* namelen)
{
	LOG_TRACE_NETWORK("XSocketGetPeerName()");
	XVirtualSocket* xsocket = (XVirtualSocket*)s;

	if (xsocket->IsTCP())
		return getpeername(xsocket->systemSocketHandle, name, namelen);
	else
		return WSAENOTCONN;
}


// #12: XSocketConnect
int WINAPI XSocketConnect(SOCKET s, const struct sockaddr* name, int namelen)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	LOG_TRACE_NETWORK("XSocketConnect  (socket = {0:x}, name = {1:p}, namelen = {2})",
		xsocket->systemSocketHandle, (void*)name, namelen);

	return connect(xsocket->systemSocketHandle, name, namelen);
}


// #13: XSocketListen
int WINAPI XSocketListen(SOCKET s, int backlog)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	LOG_TRACE_NETWORK("XSocketListen  (socket = {0:x}, backlog = {1:x})",
		xsocket->systemSocketHandle, backlog);

	return listen(xsocket->systemSocketHandle, backlog);
}


// #14: XSocketAccept
SOCKET WINAPI XSocketAccept(SOCKET s, struct sockaddr* addr, int* addrlen)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;

	LIMITED_LOG(35, LOG_TRACE_NETWORK, "XSocketAccept  (socket = {0:x}, addr = {1:p}, addrlen = {2})",
		xsocket->systemSocketHandle, (void*)addr, *addrlen);

	return accept(xsocket->systemSocketHandle, addr, addrlen);
}


// #15: XSocketSelect
int WINAPI XSocketSelect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout)
{
	LIMITED_LOG(35, LOG_TRACE_NETWORK, "XSocketSelect()");

	return select(nfds, readfds, writefds, exceptfds, timeout);
}

// #16
BOOL WINAPI XSocketWSAGetOverlappedResult(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	LOG_TRACE_NETWORK("XSocketWSAGetOverlappedResult  (socket = {0:x}, lpWSAOverlapped = {1:p}, lpcbTransfer = {2:p}, fWait = {3}, lpdwFlags = {4:p})",
		xsocket->systemSocketHandle, (void*)lpOverlapped, (void*)lpcbTransfer, fWait, (void*)lpdwFlags);

	return WSAGetOverlappedResult(xsocket->systemSocketHandle, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
}

// #17
BOOL WINAPI XSocketWSACancelOverlappedIO(HANDLE hFile)
{
	LOG_TRACE_NETWORK("XSocketWSACancelOverlappedIO");
	return CancelIo(hFile);
}

static int read_system_socket(
	SOCKET s,
	LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesRecvd,
	LPDWORD lpFlags,
	struct sockaddr* lpFrom,
	LPINT lpFromlen,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
	int result;
#if XSOCK_USING_STANDARD_APIS
	result = ::recvfrom(s, lpBuffers->buf, lpBuffers->len, *lpFlags, lpFrom, lpFromlen);
	*lpNumberOfBytesRecvd = result;
#else
	result = WSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
#endif
	return result;
}

int XVirtualSocket::read_socket(
	LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesRecvd,
	LPDWORD lpFlags,
	struct sockaddr* lpFrom,
	LPINT lpFromlen,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	bool* outWinApiError)
{
	if (outWinApiError)
		*outWinApiError = false;

	if (this->IsTCP()
		|| lpFrom == NULL)
	{
		return WSARecv(
			this->systemSocketHandle,
			lpBuffers,
			dwBufferCount,
			lpNumberOfBytesRecvd,
			lpFlags,
			lpOverlapped,
			lpCompletionRoutine);
	}

	// ### TODO FIXME this might be better to be executed asynchronously, in another network thread, handling connections
	// but for now just leverage the current Halo 2's behaviour of polling all network sockets, and read our network data here in case 

	// read the main xnet socket
	int result;
	
	/*result = read_system_socket(
		g_XSockMgr.GetMainUdpSocketSystemHandle(),
		lpBuffers,
		dwBufferCount,
		lpNumberOfBytesRecvd,
		lpFlags,
		lpFrom,
		lpFromlen,
		lpOverlapped,
		lpCompletionRoutine);

	if (result != SOCKET_ERROR)
	{
		gXnIpMgr.HandleRecvdPacket(this, (sockaddr_in*)lpFrom, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd);
	}*/

	result = read_system_socket(systemSocketHandle, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
	bool mainSocketEmpty = result == SOCKET_ERROR
		&& WSAGetLastError() == WSAEWOULDBLOCK;

	if (mainSocketEmpty)
	{
		if (this->IsBroadcast())
		{
			// check if there's an error
			// and if that error is WSAEWOULDBLOCK
			// prioritize other type of data over broadcast
			// read the broadcast data, if system-link is properly initialized
			if (g_XSockMgr.SystemLinkAvailable())
			{
				for (int i = 0; i < 2; i++)
				{
					if (g_XSockMgr.SystemLinkGetSystemSockHandle(i) != INVALID_SOCKET)
					{
						result = read_system_socket(
							g_XSockMgr.SystemLinkGetSystemSockHandle(i), 
							lpBuffers,
							dwBufferCount,
							lpNumberOfBytesRecvd, 
							lpFlags,
							lpFrom,
							lpFromlen, 
							lpOverlapped, 
							lpCompletionRoutine);

						if (result != SOCKET_ERROR)
						{
							mainSocketEmpty = false;
							break;
						}
					}
				}
			}
		}
	}

	if (result == SOCKET_ERROR)
	{
		*lpNumberOfBytesRecvd = 0;
		if (outWinApiError)
			*outWinApiError = true;
		return SOCKET_ERROR;
	}
	
	result = gXnIpMgr.HandleRecvdPacket(this, (sockaddr_in*)lpFrom, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd);
	return result;
}

int XVirtualSocket::sock_read(LPWSABUF lpBuffers,
	DWORD dwBufferCount,
	LPDWORD lpNumberOfBytesRecvd,
	LPDWORD lpFlags,
	struct sockaddr* lpFrom,
	LPINT lpFromlen,
	LPWSAOVERLAPPED lpOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	// loop SOCK_MAX_RECV_PAYLOADS_TO_READ_PER_CALL times until we get a valid packet
	// unless Winsock API returns an error
	for (int i = 0; i < SOCK_MAX_RECV_PAYLOADS_TO_READ_PER_CALL; i++)
	{
		bool errorByAPI = false;

		int result = read_socket(lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine, &errorByAPI);

		if (result == SOCKET_ERROR)
		{
			if (errorByAPI)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
					LOG_ERROR_NETWORK("{} - socket error: {}", __FUNCTION__, WSAGetLastError());

				return result;
			}
			else
			{
				// continue reading the socket, if error wasn't by winsock api
				continue;
			}
		}
		else
		{
			// if there's no error, return the packet
			return result;
		}
	}

	// if we got this far, it means there were no game packets inbound, just XNet packets
	XSocketWSASetLastError(WSAEWOULDBLOCK);
	return SOCKET_ERROR;
}

// #21
int WINAPI XSocketWSARecvFrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	XVirtualSocket* socket = (XVirtualSocket*)s;
	return socket->sock_read(lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine);
}

// #25
int WINAPI XSocketWSASendTo(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, sockaddr* lpTo, int iTolen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	sockaddr_in* inTo = (sockaddr_in*)lpTo;

	if (xsocket->IsTCP() || inTo == NULL)
	{
		return WSASend(
			xsocket->systemSocketHandle,
			lpBuffers,
			dwBufferCount,
			lpNumberOfBytesSent,
			dwFlags,
			lpOverlapped,
			lpCompletionRoutine);
	}

	// check if the title attempts to send broadcast/systemlink data
	if ((inTo->sin_addr.s_addr == htonl(INADDR_BROADCAST)
		|| inTo->sin_addr.s_addr == htonl(INADDR_ANY))
		&& xsocket->IsBroadcast())
	{
		if (g_XSockMgr.SystemLinkAvailable())
		{
			XBroadcastPacket* packet = (XBroadcastPacket*)_alloca(sizeof(XBroadcastPacket) + lpBuffers->len);
			new (packet) XBroadcastPacket();

			memcpy((char*)packet + sizeof(XBroadcastPacket), lpBuffers->buf, lpBuffers->len);

			sockaddr_in broadcastAddresses[2];
			memset(broadcastAddresses, 0, sizeof(broadcastAddresses));

			broadcastAddresses[0].sin_family = AF_INET;
			broadcastAddresses[0].sin_addr.s_addr = htonl(INADDR_BROADCAST);
			if (H2Config_ip_broadcast_override != htonl(INADDR_ANY))
				broadcastAddresses[0].sin_addr.s_addr = H2Config_ip_broadcast_override;
			broadcastAddresses[0].sin_port = g_XSockMgr.SystemLinkGetPort();

			// also send message to localhost multicast group
			broadcastAddresses[1].sin_family = AF_INET;
			broadcastAddresses[1].sin_addr.s_addr = htonl(XSOCK_MUTICAST_ADDR);
			broadcastAddresses[1].sin_port = htons(XSOCK_MULTICAST_PORT);

			int result = SOCKET_ERROR;
			WSASetLastError(WSAEINVAL);

			for (int i = 0; i < 2; i++)
			{
				if (g_XSockMgr.SystemLinkGetSystemSockHandle(i) != INVALID_SOCKET)
				{
					// send LAN broadcast packet
					result = sendto(g_XSockMgr.SystemLinkGetSystemSockHandle(i),
						(const char*)packet,
						sizeof(XBroadcastPacket) + lpBuffers->len,
						dwFlags,
						(const sockaddr*)&broadcastAddresses[i], sizeof(sockaddr_in));
				}
			}

			packet->~XBroadcastPacket();
			return result;
		}
		else
		{
			WSASetLastError(WSAEINVAL);
			return SOCKET_ERROR;
		}
	}

	/*
		Create new SOCKADDR_IN structure,
		If we overwrite the original the game's security functions know it's not a secure address any longer.
		Worst case if this is found to cause performance issues we can handle the send and re-update to secure before return.
	*/

	XnIp* xnIp = gXnIpMgr.GetConnection(inTo->sin_addr);
	if (xnIp != nullptr
		&& !xnIp->ConnectStatusLost())
	{
		sockaddr_in sendToAddr;
		ZeroMemory(&sendToAddr, sizeof(sendToAddr));
		sendToAddr.sin_family = AF_INET;
		sendToAddr.sin_addr = xnIp->GetOnlineIpAddr();

		// check if the online ip address is the same as the local one
		// and if the online ip address of the connection is 0, fall back to LAN address
		// to allow packets to be sent even if an account is logged in locally or online (local Xbox profile or Online profile)
		if (xnIp->GetOnlineIpAddr().s_addr == gXnIpMgr.GetLocalUserXn()->GetOnlineIpAddr().s_addr
			|| xnIp->GetOnlineIpAddr().s_addr == 0)
		{
			sendToAddr.sin_addr = xnIp->GetLanIpAddr();
		}

		//sendToAddr.sin_port = xnIp->m_xnaddr.wPortOnline;
		sendToAddr.sin_port = htons(ntohs(xnIp->m_xnaddr.wPortOnline) + (ntohs(inTo->sin_port) % 1000));
		if (xnIp->PortMappingAvailable(inTo->sin_port))
		{
			const sockaddr_in* mapping = xnIp->GetPortMapping(inTo->sin_port);
			// if port map is available, use it
			if (!xsocket->SockAddrInInvalid(mapping))
			{
				sendToAddr = *mapping;

				LOG_TRACE_NETWORK("{} - using available port mapping for connection: {:X}, ip address: {:X} port: {}, local virtual: {}",
					__FUNCTION__,
					xnIp->GetConnectionId().s_addr,
					ntohl(xnIp->GetOnlineIpAddr().s_addr),
					ntohs(sendToAddr.sin_port),
					xsocket->GetHostOrderSocketVirtualPort()
					);
			}
		}
		else
		{
			LOG_TRACE_NETWORK("{} - using default port mapping, for connection: {:X}, ip address: {:X} port: {}, local virtual: {}", 
				__FUNCTION__, 
				xnIp->GetConnectionId().s_addr, 
				ntohl(xnIp->GetOnlineIpAddr().s_addr), 
				ntohs(sendToAddr.sin_port),
				xsocket->GetHostOrderSocketVirtualPort()
				);
		}

		int result = SOCKET_ERROR;
		DWORD pckSent = 0;
		DWORD dwNumberOfBytesSent = 0;

#if XSOCK_USING_STANDARD_APIS
		for (DWORD i = 0ul; i < dwBufferCount; i++)
		{
			result = sendto(xsocket->systemSocketHandle, lpBuffers[i].buf, lpBuffers[i].len, dwFlags, (const sockaddr*)&sendToAddr, sizeof(sendToAddr));
			if (result == SOCKET_ERROR)
				break;

			LOG_TRACE_NETWORK("{} - sent packet to connection: {:X}, ip address: {:X} port: {}, local virtual: {}",
				__FUNCTION__,
				xnIp->GetConnectionId().s_addr,
				ntohl(xnIp->GetOnlineIpAddr().s_addr),
				ntohs(sendToAddr.sin_port),
				xsocket->GetHostOrderSocketVirtualPort()
				);

			pckSent++;
			dwNumberOfBytesSent += result;
		}
#else
		result = WSASendTo(xsocket->systemSocketHandle, lpBuffers, dwBufferCount, &dwNumberOfBytesSent, dwFlags, (const sockaddr*)&sendToAddr, sizeof(sendToAddr), lpOverlapped, lpCompletionRoutine);
#endif // if XSOCK_USING_STANDARD_APIS

		if (result == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
				LOG_ERROR_NETWORK("XSocketSendTo() - socket error: {}", WSAGetLastError());

			if (lpNumberOfBytesSent)
				*lpNumberOfBytesSent = 0;
			return SOCKET_ERROR;
		}
		else
		{
#if !XSOCK_USING_STANDARD_APIS
			pckSent = dwBufferCount;
#endif
			xnIp->m_pckStats.PckSendStatsUpdate(pckSent, dwNumberOfBytesSent);
			gXnIpMgr.GetLocalUserXn()->m_pckStats.PckSendStatsUpdate(pckSent, dwNumberOfBytesSent);
			if (lpNumberOfBytesSent)
				*lpNumberOfBytesSent = dwNumberOfBytesSent;

			return 0;
		}

		return 0;
	}
	else
	{
		LOG_TRACE_NETWORK("XSocketSendTo() - Tried to send packet to unknown connection, connection index: {}, connection identifier: {:x}",
			XnIp::GetConnectionIndex(inTo->sin_addr), inTo->sin_addr.s_addr);
		XSocketWSASetLastError(WSAEHOSTUNREACH);
		return SOCKET_ERROR;
	}
}

// #26: XSocketInet_Addr
LONG WINAPI XSocketInet_Addr(const char* cp)
{
	LOG_TRACE_NETWORK("XSocketInet_Addr()");
	return inet_addr(cp);
}

// #27: XWSAGetLastError
INT WINAPI XSocketWSAGetLastError()
{
	static int print = 0;

	int ret = WSAGetLastError();

	if (print < 15)
	{
		//LOG_TRACE_NETWORK("XSocketWSAGetLastError() - {}", ret);

		print++;
	}

	return ret;
}

// #28
void WINAPI XSocketWSASetLastError(int iError)
{
	LOG_TRACE_NETWORK("XSocketWSASetLastError()");
	WSASetLastError(iError);
}

// #29
HANDLE WINAPI XSocketWSACreateEvent()
{
	LOG_TRACE_NETWORK("XSocketWSACreateEvent()");
	return WSACreateEvent();
}

// #30
BOOL WINAPI XSocketWSACloseEvent(HANDLE hEvent)
{
	LOG_TRACE_NETWORK("XSocketWSACloseEvent()");
	return WSACloseEvent(hEvent);
}

// #31
BOOL WINAPI XSocketWSASetEvent(HANDLE hEvent)
{
	LOG_TRACE_NETWORK("XSocketWSASetEvent()");
	return WSASetEvent(hEvent);
}

// #32
BOOL WINAPI XSocketWSAResetEvent(HANDLE hEvent)
{
	LOG_TRACE_NETWORK("XSocketWSAResetEvent()");
	return WSAResetEvent(hEvent);
}

// #33
DWORD WINAPI XSocketWSAWaitForMultipleEvents(DWORD cEvents, HANDLE* lphEvents, BOOL fWaitAll, DWORD dwTimeout, BOOL fAlertable)
{
	LOG_TRACE_NETWORK("XSocketWSAWaitForMultipleEvents()");
	return WSAWaitForMultipleEvents(cEvents, lphEvents, fWaitAll, dwTimeout, fAlertable);
}

// #34
int WINAPI XSocketWSAFDIsSet(SOCKET fd, fd_set* a2)
{
	LOG_TRACE_NETWORK("XSocketWSAFDIsSet()");
	return __WSAFDIsSet(fd, a2);
}

// #35
int WINAPI XSocketWSAEventSelect(SOCKET s, HANDLE hEventObject, __int32 lNetworkEvents)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	LOG_TRACE_NETWORK("XSocketWSAEventSelect()");
	return WSAEventSelect(xsocket->systemSocketHandle, hEventObject, lNetworkEvents);
}

// #4
int WINAPI XSocketClose(SOCKET s)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;
	LOG_TRACE_NETWORK("XSocketClose() - socket: {}", xsocket->systemSocketHandle);

	int ret = closesocket(xsocket->systemSocketHandle);

	for (auto i = g_XSockMgr.sockets.begin(); i != g_XSockMgr.sockets.end(); ++i)
	{
		if (*i == xsocket)
		{
			g_XSockMgr.sockets.erase(i);
			break;
		}
	}

	delete xsocket;

	return ret;
}

// #11: XSocketBind
SOCKET WINAPI XSocketBind(SOCKET s, const struct sockaddr* name, int namelen)
{
	XVirtualSocket* xsocket = (XVirtualSocket*)s;

	// copy socket ip/port
	memcpy(&xsocket->name, name, sizeof(sockaddr_in));
	u_short virtual_port = ((struct sockaddr_in*)name)->sin_port;

	sockaddr sockBind;
	memset(&sockBind, 0, sizeof(sockBind));

	sockaddr_in* sockBindInAddr = (sockaddr_in*)&sockBind;
	sockBindInAddr->sin_family = AF_INET;
	// ### TODO FIXME add configurable network adapter
	sockBindInAddr->sin_addr.s_addr = htonl(INADDR_ANY);
	sockBindInAddr->sin_port = htons(0);

	switch (ntohs(virtual_port))
	{
	case 1000:
	case 1001:
		sockBindInAddr->sin_port = htons(H2Config_base_port + (ntohs(virtual_port) % 1000));
		break;
	default:
		break;
	}

	LOG_TRACE_NETWORK("XSocketBind() - virtual socket port - {}", ntohs(virtual_port));

	int ret = bind(xsocket->systemSocketHandle, &sockBind, namelen);

	if (ret == SOCKET_ERROR)
		LOG_TRACE_NETWORK("{}() - SOCKET_ERROR", __FUNCTION__);

	if (ret != SOCKET_ERROR)
	{
		sockaddr assignedAddr;
		int assignedAddrSize = sizeof(assignedAddr);

		getsockname(xsocket->systemSocketHandle, &assignedAddr, &assignedAddrSize);

		sockaddr_in* assignedAddrIn = (sockaddr_in*)&assignedAddr;

		LOG_TRACE_NETWORK("{}() - socket with virtual port: {} assigned system socket port: {}",
			__FUNCTION__,
			ntohs(virtual_port),
			ntohs(assignedAddrIn->sin_port)
		);
	}

	return ret;
}

// #23
int WINAPI XSocketWSASend(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	return XSocketWSASendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, NULL, 0, lpOverlapped, lpCompletionRoutine);
}

// #19
int WINAPI XSocketWSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	return XSocketWSARecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, NULL, 0, lpOverlapped, lpCompletionRoutine);
}

// #22: XSocketSend
int WINAPI XSocketSend(SOCKET s, const char* buf, int len, int flags)
{
	WSABUF wsaBuf;
	wsaBuf.len = len;
	wsaBuf.buf = (CHAR*)buf;
	DWORD numberOfBytesSent = 0;

	if (XSocketWSASendTo(s, &wsaBuf, 1, &numberOfBytesSent, flags, NULL, NULL, NULL, NULL) == 0)
		return numberOfBytesSent;
	else
		return SOCKET_ERROR;
}

// #24: XSocketSendTo
int WINAPI XSocketSendTo(SOCKET s, const char* buf, int len, int flags, sockaddr* to, int tolen)
{
	WSABUF wsaBuf;
	wsaBuf.len = len;
	wsaBuf.buf = (CHAR*)buf;
	DWORD numberOfBytesSent = 0;

	if (XSocketWSASendTo(s, &wsaBuf, 1, &numberOfBytesSent, flags, to, tolen, NULL, NULL) == 0)
		return numberOfBytesSent;
	else
		return SOCKET_ERROR;
}

// #18: XSocketRecv
int WINAPI XSocketRecv(SOCKET s, char* buf, int len, int flags)
{
	WSABUF wsaBuf[1];
	wsaBuf[0].len = len;
	wsaBuf[0].buf = (CHAR*)buf;
	DWORD numberOfbytesRecvd;

	if (XSocketWSARecvFrom(s, wsaBuf, ARRAYSIZE(wsaBuf), &numberOfbytesRecvd, (LPDWORD)&flags, NULL, NULL, NULL, NULL) == 0)
		return numberOfbytesRecvd;
	else
		return SOCKET_ERROR;
}

// #20
int WINAPI XSocketRecvFrom(SOCKET s, char* buf, int len, int flags, sockaddr* from, int* fromlen)
{
	WSABUF wsaBuf[1];
	wsaBuf[0].len = len;
	wsaBuf[0].buf = (CHAR*)buf;
	DWORD numberOfbytesRecvd;

	if (XSocketWSARecvFrom(s, wsaBuf, ARRAYSIZE(wsaBuf), &numberOfbytesRecvd, (LPDWORD)&flags, from, fromlen, NULL, NULL) == 0)
		return numberOfbytesRecvd;
	else
		return SOCKET_ERROR;
}

// #37: XSocketHTONL
u_long WINAPI XSocketHTONL(u_long hostlong)
{
	return htonl(hostlong);
}

// #38: XSocketNTOHS
u_short WINAPI XSocketNTOHS(u_short netshort)
{
	return ntohs(netshort);
}

// #39: XSocketNTOHL
u_long WINAPI XSocketNTOHL(u_long netlong)
{
	return ntohl(netlong);
}

// #40: XSocketHTONS
u_short WINAPI XSocketHTONS(u_short hostshort)
{
	return htons(hostshort);
}

int XVirtualSocket::SetBufferSize(int optname, INT bufSize)
{
	if (optname != SO_SNDBUF && optname != SO_RCVBUF)
	{
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}

	if (!this->IsUDP())
	{
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}

	int bufOpt, bufOptSize;
	bufOptSize = sizeof(bufOpt);

	if (getsockopt(this->systemSocketHandle, SOL_SOCKET, optname, (char*)&bufOpt, &bufOptSize) == SOCKET_ERROR)
	{
		LOG_ERROR_NETWORK("{} - getsockopt() failed, last error : {}, cannot increase UDP nonblocking buffer size!", __FUNCTION__, WSAGetLastError());
		return SOCKET_ERROR;
	}

	LOG_TRACE_NETWORK("{} - getsockopt() - {}: {} - {}", __FUNCTION__, sockOpt_string(optname), bufOpt, bufSize);

	// this may only affect Windows 7/Server 2008 R2 and below, as Windows 10 uses an 64K buffer already
	if (bufOpt < bufSize)
	{
		bufOpt = bufSize; // set the recvbuf to needed size
		// increase socket recv buffer
		if (setsockopt(this->systemSocketHandle, SOL_SOCKET, optname, (char*)&bufOpt, sizeof(bufOpt)) == SOCKET_ERROR) // then attempt to increase the buffer
		{
			LOG_ERROR_NETWORK("{} - setsockopt() failed, last error: {}", __FUNCTION__, WSAGetLastError());
			return SOCKET_ERROR;
		}
	}

	WSASetLastError(0);
	return 0;
}

int XVirtualSocket::UdpSend(const char* buf, int len, int flags, sockaddr* to, int tolen)
{
	return XSocketSendTo((SOCKET)this, buf, len, flags, to, tolen);
}

void XSocketManager::SocketsDisposeAll()
{
	for (auto xsocket : sockets)
	{
		XSocketClose((SOCKET)xsocket);
	}
	sockets.clear();

	SystemLinkDispose();
	MainLinkDispose();
}

void XSocketManager::Initialize()
{
}

void XSocketManager::Dispose()
{
	SocketsDisposeAll();
}

bool XSocketManager::CreateSocketUDP(XInternalSocket* sock, unsigned long interfaceAddress, WORD port, bool multicast)
{
	bool success = false;

	IN_ADDR interfaceAddr; 

	// ### TODO FIXME: allow choosing the network interface
	interfaceAddr.s_addr = interfaceAddress;
	if (multicast)
	{
		interfaceAddr.s_addr = htonl(INADDR_LOOPBACK);
	}

	SOCKET s;
	do
	{
		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == INVALID_SOCKET)
		{
			break;
		}

		unsigned long mode = 1;
		if (ioctlsocket(s, FIONBIO, &mode) == SOCKET_ERROR)
		{
			break;
		}

		int optval_broadcast = 1;
		if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char*)&optval_broadcast, sizeof(optval_broadcast)) == SOCKET_ERROR)
		{
			break;
		}

		if (multicast)
		{
			// allow same addr to be used
			int optval_reuseaddr = 1;
			if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&optval_reuseaddr, sizeof(optval_reuseaddr)) == SOCKET_ERROR)
			{
				break;
			}
		}

		sockaddr_in name;
		int name_len = sizeof(name);
		memset(&name, 0, sizeof(sockaddr_in));
		name.sin_family = AF_INET;
		name.sin_addr = interfaceAddr;
		name.sin_port = port;

		if (bind(s, (const sockaddr*)&name, sizeof(name)) == SOCKET_ERROR)
		{
			break;
		}

		if (multicast)
		{
			ip_mreq mreq;

			// localhost only
			mreq.imr_interface.s_addr = htonl(INADDR_LOOPBACK);
			mreq.imr_multiaddr.s_addr = htonl(XSOCK_MUTICAST_ADDR);

			if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)))
			{
				break;
			}
		}

		if (getsockname(s, (struct sockaddr*)&name, &name_len) == SOCKET_ERROR)
		{
			break;
		}

		if (name.sin_port != port)
		{
			// failed to use the configured port, likely was already in USE!
			break;
		}

		success = true;

	} while (0);

	if (success)
	{
		sock->m_systemSockHandle = s;
		sock->m_port = port;
		sock->m_multicast = multicast;
	}
	else
	{
		sock->m_systemSockHandle = INVALID_SOCKET;
		sock->m_port = 0;
		sock->m_multicast = false;
	}

	return success;
}

bool XSocketManager::SystemLinkSocketInitialize(WORD port)
{
	bool success = CreateSocketUDP(&m_broadcastLANSock, htonl(INADDR_ANY), port, false);
	success |= CreateSocketUDP(&m_broadcastLocalhostSock, htonl(INADDR_ANY), htons(XSOCK_MULTICAST_PORT), true);

	// ### FIXME return error code??
	return success;
}

bool XSocketManager::SystemLinkSocketReset(WORD port)
{
	SystemLinkDispose();
	return SystemLinkSocketInitialize(port);
}

void XSocketManager::SystemLinkDispose()
{
	if (SystemLinkAvailable())
	{
		m_broadcastLANSock.Dispose();
		m_broadcastLocalhostSock.Dispose();
	}
}

bool XSocketManager::MainLinkSocketInitialize(WORD port) 
{
	bool success = CreateSocketUDP(&m_mainUdpSocket, htonl(INADDR_ANY), port, false);
	return success;
}

bool XSocketManager::MainLinkSocketReset(WORD port) 
{
	MainLinkDispose();
	return MainLinkSocketInitialize(port);
}

void XSocketManager::MainLinkDispose() 
{
	m_mainUdpSocket.Dispose();
}