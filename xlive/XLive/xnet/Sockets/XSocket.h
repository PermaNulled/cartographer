#pragma once

#define IPPROTO_VDP 254

#define SOCK_K_UNIT 1024

#define SOCK_UDP_MIN_SEND_BUFFER_K_UNITS 64 // this by default XLive is 16K
#define SOCK_UDP_MIN_RECV_BUFFER_K_UNITS 64 // this by default XLive is 16K

#define SOCK_UDP_MIN_SEND_BUFFER (SOCK_UDP_MIN_SEND_BUFFER_K_UNITS * SOCK_K_UNIT)
#define SOCK_UDP_MIN_RECV_BUFFER (SOCK_UDP_MIN_RECV_BUFFER_K_UNITS * SOCK_K_UNIT)

#define XSOCK_USING_STANDARD_APIS 1

#define SOCK_MAX_RECV_PAYLOADS_TO_READ_PER_CALL 20

enum
{
	EXSOCK_IDENTIFIER = 'XSoC'
};

// 239.255.0.1
#define XSOCK_MUTICAST_ADDR  0xefff0001
#define XSOCK_MULTICAST_PORT 56011

struct XVirtualSocket;

class XSocketManager
{
	bool m_initialized = false;

	class XInternalSocket
	{
		friend class XSocketManager;

		WORD m_port = 0;
		SOCKET m_systemSockHandle = INVALID_SOCKET;
		bool m_multicast;

		void Dispose()
		{
			if (m_systemSockHandle != INVALID_SOCKET)
			{
				if (m_multicast)
				{
					ip_mreq mreq;

					// localhost only
					mreq.imr_interface.s_addr = htonl(INADDR_LOOPBACK);
					mreq.imr_multiaddr.s_addr = htonl(XSOCK_MUTICAST_ADDR);

					setsockopt(
						m_systemSockHandle, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq, sizeof(mreq)
					);
				}

				closesocket(m_systemSockHandle);

			}
			m_systemSockHandle = INVALID_SOCKET;
			m_port = 0;
		}
	};

	XInternalSocket m_mainUdpSocket;
	XInternalSocket m_broadcastLANSock;
	XInternalSocket m_broadcastLocalhostSock;

public:
	void Initialize();
	void Dispose();

	bool SystemLinkSocketInitialize(WORD port);
	bool SystemLinkSocketReset(WORD port);
	void SystemLinkDispose();

	bool MainLinkSocketInitialize(WORD port);
	bool MainLinkSocketReset(WORD port);
	void MainLinkDispose();

	SOCKET GetMainUdpSocketSystemHandle() const
	{
		return m_mainUdpSocket.m_systemSockHandle;
	}

	bool CreateSocketUDP(XInternalSocket* sock, unsigned long interfaceAddress, WORD port, bool multicast);

	WORD SystemLinkGetPort() const
	{
		return m_broadcastLANSock.m_port;
	}

	SOCKET SystemLinkGetLANSystemSockHandle() const
	{
		return m_broadcastLANSock.m_systemSockHandle;
	}

	SOCKET SystemLinkGetSystemSockHandle(int index) const
	{
		switch (index)
		{
		case 0:
			return m_broadcastLANSock.m_systemSockHandle;
		case 1:
			return m_broadcastLocalhostSock.m_systemSockHandle;
		}

		return INVALID_SOCKET;
	}

	bool SystemLinkAvailable() const
	{
		for (int i = 0; i < 2; i++)
		{
			if (SystemLinkGetSystemSockHandle(i) != INVALID_SOCKET)
			{
				return true;
			}
		}
		return false;
	}

	void SocketsDisposeAll();
	std::vector<XVirtualSocket*> sockets;
};

struct XVirtualSocket
{
	SOCKET systemSocketHandle;
	int identifier;
	int protocol;
	sockaddr_in name;
	bool isBroadcast;
	bool isVoiceProtocol;

	XVirtualSocket(int _protocol, bool _isVoiceSocket)
	{
		identifier = EXSOCK_IDENTIFIER;
		protocol = _protocol;
		isVoiceProtocol = _isVoiceSocket;
		systemSocketHandle = INVALID_SOCKET;
		isBroadcast = false;
		memset(&name, 0, sizeof(name));
	}

	bool IsTCP() const { return protocol == IPPROTO_TCP; }
	bool IsUDP() const { return protocol == IPPROTO_UDP; }

	bool IsBroadcast() const { return isBroadcast; }

	/* VDP uses UDP, and some encryption (done at network transport layer, not game layer) */
	bool IsVDP() const { return protocol == IPPROTO_UDP; }

	bool IsValid() const { return identifier == EXSOCK_IDENTIFIER; }

	// all fields in sockaddr_in are in network byte order
	// some helpers for conversion for each case needed
	/* get the port, in host byte order, in this case little-endian */
	u_short GetHostOrderSocketVirtualPort() const { return ntohs(name.sin_port); }

	/* get the port, in network byte order, in this case big-endian */
	u_short GetNetworkOrderSocketVirtualPort() const { return name.sin_port; }

	/* sets the socket send/recv buffer size */
	int SetBufferSize(int optName, INT bufSize);

	static bool SockAddrInEqual(const sockaddr_in* a1, const sockaddr_in* a2)
	{
		return (a1->sin_addr.s_addr == a2->sin_addr.s_addr && a1->sin_port == a2->sin_port);
	}

	static bool SockAddrInInvalid(const sockaddr_in* a1)
	{
		return a1->sin_addr.s_addr == 0 || a1->sin_port == 0;
	}

	int UdpSend(const char* buf, int len, int flags, sockaddr *to, int tolen);

	// dwBufferCount > 1 is unsupported/unimplemented
	int sock_read(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

private:
	int read_socket(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, struct sockaddr* lpFrom, LPINT lpFromlen, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, bool* winApiError);
};

void WINAPI XSocketWSASetLastError(int iError);
SOCKET WINAPI XSocketCreate(int af, int type, int protocol);
int WINAPI XSocketSendTo(SOCKET s, const char* buf, int len, int flags, sockaddr *to, int tolen);

extern XSocketManager g_XSockMgr;