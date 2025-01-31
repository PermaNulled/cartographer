#include "stdafx.h"
#include "XLive/xnet/XNetQoS.h"

#include <MSWSock.h>
#include <WS2tcpip.h>
#include "IpManagement/XnIp.h"
#include "H2MOD/Modules/Shell/Config.h"

using namespace std::chrono_literals;

CXNetQoS XNetQoS;

void ClientQoSLookUp(UINT cxna, XNADDR* pxna, UINT cProbes, IN_ADDR aina[], XNQOS** apxnqos, DWORD dwBitsPerSec, XNQOS* pqos)
{
	LIMITED_LOG(15, LOG_TRACE_NETWORK, "ClientQoSLookup( cxna: {}, cProbes: {})", cxna, cProbes);

	while (pqos->cxnqosPending > 0)
	{
		if (cProbes > 0)
		{
			XNADDR* xn = &pxna[pqos->cxnqosPending - 1];
			XNQOSINFO* xnqosinfo = &pqos->axnqosinfo[(pqos->cxnqosPending - 1)];

			//	LOG_TRACE_NETWORK_N("[XNetQoSLookup] Looping cxnas for probes pqos->xnqosPending: {}", pqos->cxnqosPending);

			int iResult;
			SOCKET connectSocket = INVALID_SOCKET;
			struct addrinfo* result = NULL, * ptr = NULL, hints;

			ZeroMemory(&hints, sizeof(hints));
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			std::string addr = inet_ntoa(xn->inaOnline);
			std::string port = std::to_string(ntohs(xn->wPortOnline) + 10);

			//	LOG_TRACE_NETWORK_N("[XNetQoSLookup] QoSLookup, addr={0}, port={1}", addr.c_str(), prt.c_str());

			// Resolve the server address and port
			iResult = getaddrinfo(addr.c_str(), port.c_str(), &hints, &result);
			if (iResult != 0) {
				//	LOG_TRACE_NETWORK_N("[XnetQoSLookup] getaddrinfo failed with error: {}\n", iResult);

				xnqosinfo->cProbesRecv = 0;
				xnqosinfo->cProbesXmit = 0;
				xnqosinfo->bFlags |= XNET_XNQOSINFO_TARGET_DISABLED;

				if (pqos->cxnqosPending > 0)
					pqos->cxnqosPending--;

				continue;
			}

			for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

				connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
				if (connectSocket == INVALID_SOCKET) {
					continue;
				}

				/* We're only allowing 500ms for a response from server, or connection to server to be established. */
				DWORD nTimeout = 500;
				setsockopt(connectSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeout, sizeof(DWORD));
				setsockopt(connectSocket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&nTimeout, sizeof(DWORD));

				// Connect to server.
				iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
				if (iResult == SOCKET_ERROR) {
					closesocket(connectSocket);
					continue;
				}

				break; // if we get here, we must have connected successfully
			}

			freeaddrinfo(result);

			if (connectSocket == INVALID_SOCKET || iResult == SOCKET_ERROR)
			{
				//	LOG_TRACE_NETWORK_N("[XNetQoSLookup] Unable to connect to server...");

				xnqosinfo->cProbesRecv = 0;
				xnqosinfo->cProbesXmit = 0;
				xnqosinfo->bFlags |= XNET_XNQOSINFO_TARGET_DISABLED;

				if (pqos->cxnqosPending > 0)
					pqos->cxnqosPending--;

				continue;
			}

			int recvResult = 0;
			int recvBufLen = gXnIpMgr.GetReqQoSBufferSize();
			BYTE* recvBuf = new BYTE[recvBufLen];
			ZeroMemory(recvBuf, recvBufLen);

			std::vector<long long> ping_storage;
			int probes = cProbes;
			do
			{
				//	LOG_TRACE_NETWORK_N("[XNetQoSLookup][Client] Sending Probe {}..",probes);

				char send_data[4];
				ZeroMemory(send_data, sizeof(send_data));
				*(DWORD*)&send_data = 0xAABBCCDD;

				auto started = std::chrono::high_resolution_clock::now();
				send(connectSocket, send_data, sizeof(send_data), 0);
				recvResult = recv(connectSocket, (char*)recvBuf, recvBufLen, 0);
				auto done = std::chrono::high_resolution_clock::now();

				//	LOG_TRACE_NETWORK_N("[XNetQosLookup][Client] Probe got {} bytes of data back", RecvLen);

				std::chrono::milliseconds d = std::chrono::duration_cast<std::chrono::milliseconds>(done - started);

				long long diff = d.count();

				//	LOG_TRACE_NETWORK_N("[XNetQoSLookup][Client] Probe {} finished ping: %lld", probes, diff);

				ping_storage.push_back(diff);

				probes--;
			} while (probes > 0);

			/*int WSAError = WSAGetLastError();
			if (RecvLen == SOCKET_ERROR
				|| WSAError == WSAETIMEDOUT
				|| WSAError == WSAECONNRESET
				|| WSAError == WSAESHUTDOWN
				|| WSAError == WSAEINVAL
				|| connectSocket == INVALID_SOCKET
				|| connectSocket == SOCKET_ERROR)
			{
				closesocket(connectSocket);
					LOG_TRACE_NETWORK_N("[XNetQoSLookup][Socket: %08X] Winsock Error Occured: %i - Socket Closed", connectSocket, WSAError);
			}*/

			closesocket(connectSocket);

			if (recvResult <= 0 || recvResult != recvBufLen)
			{
				//	LOG_TRACE_NETWORK_N("[XNetQoSLookup][Socket: %08X] Disconnected!",connectSocket);

				xnqosinfo->cProbesRecv = 0;
				xnqosinfo->cProbesXmit = 0;
				xnqosinfo->bFlags |= XNET_XNQOSINFO_TARGET_DISABLED;

				if (pqos->cxnqosPending > 0)
					pqos->cxnqosPending--;

				continue;
			}

			auto ping_result = std::minmax_element(ping_storage.begin(), ping_storage.end());
			long long min_ping = *ping_result.first;
			long long max_ping = *ping_result.second;
			long average = (long)(std::accumulate(ping_storage.begin(), ping_storage.end(), 0ll) / ping_storage.size());

			ping_storage.clear();

			//	LOG_TRACE_NETWORK_N("[XNetQoSLookup][Client] Finished Probes max_ping: {0}, min_ping: {1}, average: {2}", (WORD)max_ping, (WORD)min_ping,average);

			xnqosinfo->wRttMinInMsecs = (WORD)min_ping;
			xnqosinfo->wRttMedInMsecs = (WORD)average;
			xnqosinfo->cProbesRecv = 4;
			xnqosinfo->cProbesXmit = 4;
			xnqosinfo->pbData = recvBuf;
			xnqosinfo->cbData = recvBufLen;
			xnqosinfo->dwUpBitsPerSec = dwBitsPerSec;
			xnqosinfo->dwDnBitsPerSec = dwBitsPerSec;
			xnqosinfo->bFlags |= (XNET_XNQOSINFO_TARGET_CONTACTED | XNET_XNQOSINFO_COMPLETE | XNET_XNQOSINFO_DATA_RECEIVED);

			if (pqos->cxnqosPending > 0)
				pqos->cxnqosPending--;
		}
	}

	// XNADDR
	delete[] pxna;
}

bool CXNetQoS::IsListening()
{
	return m_listenerThreadRunning;
}

LPFN_ACCEPTEX lpfnAcceptEx = nullptr;

void CALLBACK CXNetQoS::HandleClient(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	LPSOCKET_INFORMATION acceptSockInfo = reinterpret_cast<LPSOCKET_INFORMATION>(lpOverlapped);
	//LOG_TRACE_NETWORK_N("{} - socket: {}", __FUNCTION__, acceptSockInfo->Socket);

	do
		// while (0) for the break functionality
		// break in this context is goto cleanup; equivalent
	{
		if (dwError != 0)
		{
			LOG_TRACE_NETWORK("{} - closing connection, dwError: {}", __FUNCTION__, dwError);
			break;
		}

		if (cbTransferred == 0)
		{
			//LOG_TRACE_NETWORK("{} - no bytes sent!", __FUNCTION__);
			break;
		}

		// delete memory allocated inside SendBack callback
		delete[] acceptSockInfo->DataBuf.buf;

		acceptSockInfo->DataBuf.len = 4;
		acceptSockInfo->DataBuf.buf = new char[acceptSockInfo->DataBuf.len];
		ZeroMemory(&acceptSockInfo->Overlapped, sizeof(acceptSockInfo->Overlapped));

		DWORD flags = 0;
		int recvResult = WSARecv(acceptSockInfo->Socket, &acceptSockInfo->DataBuf, 1, NULL, &flags, &acceptSockInfo->Overlapped, SendBack);
		if (recvResult == 0)
		{
			// the operation will be completed synchronously
			return;
		}
		else if (recvResult == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSA_IO_PENDING)
			{
				// the operation will be completed asynchronously, don't clear memory because it will be used
				return;
			}
			else
			{
				LOG_TRACE_NETWORK("{} - WSARecv operation failed with error: {}", __FUNCTION__, WSAGetLastError());
				break;
			}
		}
	} while (0);

	closesocket(acceptSockInfo->Socket);
	delete[] acceptSockInfo->DataBuf.buf;
	delete acceptSockInfo;
	return;
}

void CALLBACK CXNetQoS::SendBack(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	LPSOCKET_INFORMATION acceptSockInfo = reinterpret_cast<LPSOCKET_INFORMATION>(lpOverlapped);

	//LOG_TRACE_NETWORK("{} - callback -> socket: {}", __FUNCTION__, acceptSockInfo->Socket);

	do
		// while (0) for the break functionality
		// break in this context is goto cleanup; equivalent
	{
		if (dwError != (DWORD)0)
		{
			LOG_TRACE_NETWORK("{} - dwError: {}, cleaning up", __FUNCTION__, dwError);
			break;
		}

		if (cbTransferred == 0)
		{
			//LOG_TRACE_NETWORK("{} - no data received!", __FUNCTION__);
			break;
		}

		if (*(DWORD*)acceptSockInfo->DataBuf.buf == 0xAABBCCDD)
		{
			//LOG_TRACE_NETWORK("{} - magic is right, sending data back on socket {}", __FUNCTION__, acceptSockInfo->Socket);

			delete[] acceptSockInfo->DataBuf.buf;

			// copy the data passed by the game
			acceptSockInfo->DataBuf.len = gXnIpMgr.GetReqQoSBufferSize();
			acceptSockInfo->DataBuf.buf = new CHAR[gXnIpMgr.GetReqQoSBufferSize()];
			memcpy(acceptSockInfo->DataBuf.buf, XNetQoS.pbData, gXnIpMgr.GetReqQoSBufferSize());

			ZeroMemory(&acceptSockInfo->Overlapped, sizeof(acceptSockInfo->Overlapped));
			int sendResult = WSASend(acceptSockInfo->Socket, &acceptSockInfo->DataBuf, 1, NULL, 0, &acceptSockInfo->Overlapped, HandleClient);
			if (sendResult == 0)
			{
				// the operation will be completed synchronously
				return;
			}
			else if (sendResult == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSA_IO_PENDING)
				{
					// the operation will be completed asynchronously, don't clear memory because it will be used
					return;
				}
				else
				{
					// LOG_TRACE_NETWORK("{} - WSASend() failed with error {}", __FUNCTION__, WSAGetLastError());
					break;
				}
			}
		}
		else
		{
			// LOG_TRACE_NETWORK("{} - Unknown data received! (wtf)", __FUNCTION__);
			break;
		}

	} while (0);

	closesocket(acceptSockInfo->Socket);
	delete[] acceptSockInfo->DataBuf.buf;
	delete acceptSockInfo;
	return;
}

void CXNetQoS::Listener()
{
	do
		// while (0) for the break functionality
		// break in this context is goto cleanup; equivalent
	{
		m_listenerThreadRunning = true;
		LOG_TRACE_NETWORK("{} - QoS listener initializing", __FUNCTION__);

		sockaddr_in serverAddr;
		ZeroMemory(&serverAddr, sizeof(serverAddr));

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // anyone can connect
		serverAddr.sin_port = htons(H2Config_base_port + 10);

		DWORD dwBytes = 0;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;

		m_WsaEvents[0] = WSACreateEvent();
		m_WsaEvents[1] = WSACreateEvent();
		if (m_WsaEvents[0] == WSA_INVALID_EVENT
			|| m_WsaEvents[1] == WSA_INVALID_EVENT)
		{
			LOG_TRACE_NETWORK("{} - failed to create listener end event handle, error: {}", __FUNCTION__, WSAGetLastError());
			break;
		}

		m_ListenSocket = WSASocket(serverAddr.sin_family, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_ListenSocket == INVALID_SOCKET)
		{
			LOG_TRACE_NETWORK("{} - listener socket creation failed.", __FUNCTION__);
			break;
		}

		if (bind(m_ListenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		{
			LOG_TRACE_NETWORK("{} - unable to bind listener socket!", __FUNCTION__);
			break;
		}

		if (WSAIoctl(m_ListenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx, sizeof(GuidAcceptEx),
			&lpfnAcceptEx, sizeof(lpfnAcceptEx),
			&dwBytes, NULL, NULL) == SOCKET_ERROR)
		{
			LOG_TRACE_NETWORK("{} - failed to get AcceptEx function pointer, WSAIoctl() error: {}", __FUNCTION__, WSAGetLastError());
			break;
		}

		if (listen(m_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			LOG_TRACE_NETWORK("{} - listen() error: {}", __FUNCTION__, WSAGetLastError());
			break;
		}

		// if listen socket initialization went fine, wait for client connections
		while (true)
		{
			OVERLAPPED acceptOvelapped;
			ZeroMemory(&acceptOvelapped, sizeof(acceptOvelapped));

			acceptOvelapped.hEvent = m_WsaEvents[1];
			SOCKET acceptSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (acceptSocket == INVALID_SOCKET)
			{
				LOG_TRACE_NETWORK("{} - WSASocket failed with error: {}", __FUNCTION__, WSAGetLastError());
				continue;
			}

			char acceptBuffer[2 * (sizeof(sockaddr_in) + 16)];
			DWORD dwBytesReceived = 0;
			if (lpfnAcceptEx(m_ListenSocket, acceptSocket, (PVOID)acceptBuffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytesReceived, &acceptOvelapped) == FALSE)
			{
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					LOG_TRACE_NETWORK("{} - AcceptEx failed with error: {}", __FUNCTION__, WSAGetLastError());
					closesocket(acceptSocket);
					continue;
				}
			}

			bool bConnectionInbound = false;

			// Wait for AcceptEx events
			// We use the async version of AcceptEx because we want to have the abillity to exit the thread
			DWORD eventWaitRet;
			do 
				eventWaitRet = WSAWaitForMultipleEvents(ARRAYSIZE(m_WsaEvents), m_WsaEvents, FALSE, WSA_INFINITE, TRUE);
			while (eventWaitRet == WSA_WAIT_IO_COMPLETION);

			if (eventWaitRet == WSA_WAIT_FAILED)
			{
				LOG_TRACE_NETWORK("{} - WSAWaitForMultipleEvents() failed with error {}", __FUNCTION__, WSAGetLastError());
				closesocket(acceptSocket);
				continue;
			}
			else if (eventWaitRet - WSA_WAIT_EVENT_0 == 0u)
			{
				LOG_TRACE_NETWORK("{} - signaled to terminate thread, last WSAError - {}", __FUNCTION__, WSAGetLastError());
				closesocket(acceptSocket);
				break;
			}
			else if (eventWaitRet - WSA_WAIT_EVENT_0 == 1u)
			{
				// connection inbound, continue normally
				WSAResetEvent(m_WsaEvents[1]);
			}

			LPSOCKET_INFORMATION lpSockInfo = new SOCKET_INFORMATION;
			ZeroMemory(lpSockInfo, sizeof(SOCKET_INFORMATION));

			lpSockInfo->BytesSEND = 0;
			lpSockInfo->BytesRECV = 0;
			lpSockInfo->Socket = acceptSocket;

			// recv buffer prep
			lpSockInfo->DataBuf.len = 4;
			lpSockInfo->DataBuf.buf = new char[lpSockInfo->DataBuf.len];

			DWORD flags = 0;
			// LOG_TRACE_NETWORK("{} - WSARecv about to receive data on socket: {}", __FUNCTION__, acceptSocket);
			if (WSARecv(lpSockInfo->Socket, &lpSockInfo->DataBuf, 1, NULL, &flags, &lpSockInfo->Overlapped, SendBack) == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					LOG_TRACE_NETWORK("{} - WSARecv failed with error: {}", __FUNCTION__, WSAGetLastError());
					closesocket(acceptSocket);
					delete lpSockInfo;
				}
			}
		}

	} while (0);

	// cleanup
	if (m_ListenSocket != INVALID_SOCKET)
	{
		closesocket(m_ListenSocket);
		m_ListenSocket = INVALID_SOCKET;
	}

	for (int i = 0; i < ARRAYSIZE(m_WsaEvents); i++)
	{
		WSACloseEvent(m_WsaEvents[i]);
		m_WsaEvents[i] = WSA_INVALID_EVENT;
	}

	m_listenerThreadRunning = false;
	return;
}

// #69: XNetQosListen
DWORD WINAPI XNetQosListen(XNKID *pxnkid, PBYTE pb, UINT cb, DWORD dwBitsPerSec, DWORD dwFlags)
{
	LIMITED_LOG(15, LOG_TRACE_NETWORK, "XNetQosListen  (pb - cb = {}, dwbitsPerSec = {:X}, flags = {:X})", 
		cb, dwBitsPerSec, dwFlags);

	if ((dwFlags & XNET_QOS_LISTEN_ENABLE) && (dwFlags & XNET_QOS_LISTEN_DISABLE)) 
	{
		LOG_ERROR_NETWORK("{} - invalid dwFlags: cannot enable and disable.", __FUNCTION__);
		return WSAEINVAL;
	}

	if (XNetQoS.pbData == nullptr)
	{
		XNetQoS.pbData = new BYTE[gXnIpMgr.GetReqQoSBufferSize()];
		ZeroMemory(XNetQoS.pbData, gXnIpMgr.GetReqQoSBufferSize());
	}

	if (dwFlags & XNET_QOS_LISTEN_SET_DATA)
	{
		if (cb > 0)
		{
			if (cb > XNetQoS.cbData)
			{
				// cb shouldn't be higher than gXnIp.GetReqQoSBufferSize()
				XNetQoS.cbData = cb;
			}

			ZeroMemory(XNetQoS.pbData, gXnIpMgr.GetReqQoSBufferSize());
			memcpy(XNetQoS.pbData, pb, cb);
		}
	}

	if ((dwFlags & XNET_QOS_LISTEN_ENABLE) && XNetQoS.IsListening() == false)
	{
		std::thread(&CXNetQoS::Listener, &XNetQoS).detach();
	}

	if (dwFlags & XNET_QOS_LISTEN_DISABLE)
	{

	}

	if ((dwFlags & XNET_QOS_LISTEN_RELEASE) && XNetQoS.IsListening() != false)
	{
		if (XNetQoS.m_WsaEvents[0] != WSA_INVALID_EVENT)
		{
			// signal the thread it has to unblock, and based on m_bStopListening it will decide wheter terminate the thread or not
			WSASetEvent(XNetQoS.m_WsaEvents[0]);
			LOG_TRACE_NETWORK("{} - listener signaled to release resources!", __FUNCTION__);
		}
	}

	return 0;
}

// #70: XNetQosLookup
DWORD WINAPI XNetQosLookup(UINT cxna, XNADDR * apxna[], XNKID * apxnkid[], XNKEY * apxnkey[], UINT cina, IN_ADDR aina[], DWORD adwServiceId[], UINT cProbes, DWORD dwBitsPerSec, DWORD dwFlags, WSAEVENT hEvent, XNQOS** pxnqos)
{
	LIMITED_LOG(15, LOG_TRACE_NETWORK, "{} ( cxna: {}, cina: {}, cProbes: {}, dwBitsPerSec: {}, hEvent: {})",
		__FUNCTION__, cxna, cina, cProbes, dwBitsPerSec, hEvent);

	XNADDR* pxna = new XNADDR[cxna];
	for (DWORD i = 0; i < cxna; i++)
	{
		memcpy(&pxna[i], apxna[i], sizeof(XNADDR));
	}

	*pxnqos = (XNQOS*)new char[sizeof(XNQOS) + (sizeof(XNQOSINFO) * (cxna - 1))];

	XNQOS* pqos = *pxnqos;
	ZeroMemory(pqos, sizeof(XNQOS) + (sizeof(XNQOSINFO) * (cxna - 1)));

	pqos->cxnqos = cxna;
	pqos->cxnqosPending = cxna;

	/*
	This is gonna hit some CPUs hard when there's a lot of servers on the list, we'll probably want to queue this a bit and only allow X number of threads to run at a time.
	We want to abuse the CPU where possible considering more modern systems will have decent CPUs so we'll be able to force things to happen faster but still want to keep compatibility with older setups.
	*/

	std::thread(ClientQoSLookUp, cxna, pxna, cProbes, aina, pxnqos, dwBitsPerSec, pqos).detach();

	return 0;
}

// #71: XNetQosServiceLookup
INT WINAPI XNetQosServiceLookup(DWORD dwFlags, WSAEVENT hEvent, XNQOS** ppxnqos)
{
	LOG_TRACE_NETWORK("XNetQosServiceLookup()");

	// not connected to LIVE - abort now
	// - wants a3 return ASYNC struct
	return WSAEINVAL;
}

// #72: XNetQosRelease
INT WINAPI XNetQosRelease(XNQOS* pxnqos)
{
	LIMITED_LOG(15, LOG_TRACE_NETWORK, "XNetQosRelease()");
	for (unsigned int i = 0; i < pxnqos->cxnqos; i++)
	{
		// this may crash other games, doesn't for H2PC
		if (pxnqos->axnqosinfo[i].cbData > 0 && pxnqos->axnqosinfo[i].pbData)
			delete[] pxnqos->axnqosinfo[i].pbData;
	}

	// TODO: find a way to stop the tread writing to this memory block
	// TODO: there's stub Qos that might get passed to pxnqos, skip memory deallocation, check Networking.cpp for stub
	// delete[] pxnqos; 

	/* We need to clean-up all XNetQoSLookup data here, listener data should be cleaned up inside the Listen function Only. */
	return 0;
}

// #77
DWORD WINAPI XNetQosGetListenStats(XNKID *pxnKid, XNQOSLISTENSTATS *pQosListenStats)
{
	LOG_TRACE_NETWORK("XNetQosGetListenStats()");
	return 0;
}
