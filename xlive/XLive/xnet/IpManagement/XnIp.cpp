#include "stdafx.h"

#include "XnIp.h"

#include "H2MOD/Utils/Utils.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/Shell/Startup/Startup.h"

#include "XLive/Cryptography/Rc4.h"
#include "XLive/xnet/NIC.h"
#include "XLive/xnet/net_utils.h"

XnIpManager gXnIpMgr;

// local user xbox network address
XnIp XnIpManager::m_ipLocal;

XECRYPT_RC4_STATE gRc4StateRand;

void XnIpManager::Initialize(const XNetStartupParams* netStartupParams)
{
	memcpy(&m_startupParams, netStartupParams, sizeof(XNetStartupParams));

	if (m_startupParams.cfgSecRegMax == 0)
		m_startupParams.cfgSecRegMax = 32; // default 32 XNet connections

	if (m_XnIPs != nullptr)
		delete[] m_XnIPs;

	m_XnIPs = new XnIp[m_startupParams.cfgSecRegMax];
	ZeroMemory(m_XnIPs, m_startupParams.cfgSecRegMax * sizeof(*m_XnIPs));

	if (m_startupParams.cfgKeyRegMax == 0)
		m_startupParams.cfgKeyRegMax = 4; // default 4 key pairs

	if (m_XnKeyPairs != nullptr)
		delete[] m_XnKeyPairs;

	m_XnKeyPairs = new XnKeyPair[m_startupParams.cfgKeyRegMax];
	ZeroMemory(m_XnKeyPairs, m_startupParams.cfgKeyRegMax * sizeof(*m_XnKeyPairs));

	if (m_startupParams.cfgQosDataLimitDiv4 == 0)
		m_startupParams.cfgQosDataLimitDiv4 = 256 / 4; // 256 bytes as default

	if (m_startupParams.cfgSockDefaultRecvBufsizeInK <= 0)
		m_startupParams.cfgSockDefaultRecvBufsizeInK = SOCK_UDP_MIN_RECV_BUFFER_K_UNITS;

	if (m_startupParams.cfgSockDefaultSendBufsizeInK <= 0)
		m_startupParams.cfgSockDefaultSendBufsizeInK = SOCK_UDP_MIN_SEND_BUFFER_K_UNITS;

	XNetRandom(NULL, 0);
}

void XnIpManager::Dispose()
{
	g_XSockMgr.Dispose();
}

XnIp* XnIpManager::GetConnection(const IN_ADDR ina) const
{
	XnIp* xnIp = &m_XnIPs[XnIp::GetConnectionIndex(ina)];
	return xnIp->IsValid(ina) ? xnIp : nullptr;
}

void XnIpManager::UpdatePacketReceivedCounters(IN_ADDR ipIdentifier, unsigned int bytesRecvdCount)
{
	XnIp* xnIp = GetConnection(ipIdentifier);
	if (xnIp != nullptr)
	{
		xnIp->UpdateInteractionTimeHappened();
		xnIp->m_pckStats.PckRecvdStatsUpdate(1, bytesRecvdCount);
		GetLocalUserXn()->m_pckStats.PckRecvdStatsUpdate(1, bytesRecvdCount);
	}
}

void XnIpManager::LogConnectionsToConsole(TextOutputCb* outputCb) const
{
	if (!(GetRegisteredKeyCount() > 0))
	{
		const char* err_message = "cannot log XNet connections when no keys are registered (you need to host/be in a game)";
		LOG_CRITICAL_NETWORK(err_message);
		if (outputCb)
			outputCb(StringFlag_None, "# %s", err_message);
		return;
	}

	const char* xnet_connections_str = "XNet connections: ";
	LOG_CRITICAL_NETWORK(xnet_connections_str);
	if (outputCb)
		outputCb(StringFlag_None, "# %s", xnet_connections_str);

	for (int i = 0; i < GetMaxXnConnections(); i++)
	{
		std::string logString;
		XnIp* xnIp = &m_XnIPs[i];
		if (xnIp->m_valid)
		{
			const XnIpPckTransportStats* pckStats;
			xnIp->PckGetStats(&pckStats);

			logString +=
				"		Index: " + std::to_string(i) + " " +
				"Packets sent: " + std::to_string(pckStats->pckBytesSent) + " " +
				"Packets received: " + std::to_string(pckStats->pckBytesRecvd) + " " +
				"Connect status: " + std::to_string(xnIp->GetConnectStatus()) + " " +
				"Connection initiator: " + (xnIp->InitiatedConnectRequest() ? "yes" : "no") + " " +
				"Time since last interaction: " + std::to_string((float)(timeGetTime() - xnIp->m_lastConnectionInteractionTime) / 1000.f) + " " +
				"Time since last packet received: " + std::to_string((float)(timeGetTime() - pckStats->lastPacketReceivedTime) / 1000.f);

			LOG_CRITICAL_NETWORK(logString);
			if (outputCb)
				outputCb(StringFlag_None, "# %s", logString.c_str());
		}
	}

	LOG_CRITICAL_NETWORK("available XnIp connection slots: {}", GetMaxXnConnections());
	if (outputCb)
		outputCb(StringFlag_None, "# available XnIp connection slots: %d", GetMaxXnConnections());
}

void XnIpManager::LogConnectionsErrorDetails(const sockaddr_in* address, int errorCode, const XNKID* receivedKey) const
{
	LOG_CRITICAL_NETWORK("{} - tried to add XNADDR in the system, caused error: {}", __FUNCTION__, errorCode);

	if (address != nullptr)
		LOG_CRITICAL_NETWORK("{} - received connection request from {}:{} we can't fulfil.", __FUNCTION__, inet_ntoa(address->sin_addr), ntohs(address->sin_port));

	int keysRegisteredCount = GetRegisteredKeyCount();
	if (keysRegisteredCount > 0)
	{
		for (int i = 0; i < keysRegisteredCount; i++)
		{
			if (m_XnKeyPairs[i].m_valid)
			{
				LOG_CRITICAL_NETWORK("{} - registered session ID key: {}",
					__FUNCTION__,
					ByteToHexStr(m_XnKeyPairs[i].m_xnkid.ab, sizeof(m_XnKeyPairs[i].m_xnkid.ab)).c_str());
			}
		}

		XnKeyPair* matchingKey = KeyPairLookup(receivedKey);
		if (matchingKey == nullptr)
		{
			LOG_CRITICAL_NETWORK("{} - received key does not match any registered key!!", __FUNCTION__);
			return;
		}

		LOG_CRITICAL_NETWORK("{} - registered key count: {}", __FUNCTION__, keysRegisteredCount);
		for (int i = 0; i < GetMaxXnConnections(); i++)
		{
			XnIp* xnIp = &m_XnIPs[i];
			if (xnIp->m_valid)
			{
				const XnIpPckTransportStats* pckStats;
				xnIp->PckGetStats(&pckStats);

				float connectionLastInteractionSeconds = (float)(timeGetTime() - xnIp->m_lastConnectionInteractionTime) / 1000.f;
				float connectionLastPacketReceivedSeconds = (float)(timeGetTime() - xnIp->m_lastConnectionInteractionTime) / 1000.f;
				LOG_CRITICAL_NETWORK("{} - connection index: {}, packets sent: {}, packets received: {}, time since last interaction: {:.4f} seconds, time since last packet receive: {:.4f} seconds",
					__FUNCTION__,
					i,
					pckStats->pckSent,
					pckStats->pckRecvd,
					connectionLastInteractionSeconds,
					connectionLastPacketReceivedSeconds);
			}
			else
			{
				// we shouldn't ever get at this point
				LOG_CRITICAL_NETWORK("{} - hold up pendejo, wtf - connection index: {}, we have free connection registry and couldn't use it.", __FUNCTION__, i);
			}
		}
	}
	else
	{
		LOG_CRITICAL_NETWORK("{} - no keys are registered, cannot create connections with no registered keys!!", __FUNCTION__);
	}
}

int XnIpManager::HandleRecvdPacket(XVirtualSocket* xsocket, sockaddr_in* lpFrom, WSABUF* lpBuffers, DWORD dwBufferCount, LPDWORD lpBytesRecvdCount)
{
	// check first if the packet received has the size bigger or equal to the XNet packet header first
	if (*lpBytesRecvdCount >= sizeof(XNetPacketHeader))
	{
		XNetPacketHeader* packet = reinterpret_cast<XNetPacketHeader*>(lpBuffers[0].buf);
		switch (packet->signature)
		{
		case EXNIP_PACKET_SIGNATURE_XNET_BROADCAST:
		{
			XBroadcastPacket* broadcastPck = reinterpret_cast<XBroadcastPacket*>(lpBuffers[0].buf);
			if (*lpBytesRecvdCount >= sizeof(XBroadcastPacket)
				&& strncmp(broadcastPck->pckHeader.signatureString, XNIP_BROADCAST_HEADER_STR, XNIP_MAX_PCK_STR_HDR_LEN) == 0
				&& broadcastPck->data.name.sin_addr.s_addr == htonl(INADDR_BROADCAST))
			{
				if (*lpBytesRecvdCount > sizeof(XBroadcastPacket))
				{
					*lpBytesRecvdCount = *lpBytesRecvdCount - sizeof(XBroadcastPacket);
					memmove(lpBuffers[0].buf, lpBuffers[0].buf + sizeof(XBroadcastPacket), *lpBytesRecvdCount);
					lpFrom->sin_addr = broadcastPck->data.name.sin_addr;
					lpFrom->sin_port = xsocket->GetNetworkOrderSocketVirtualPort();
					return 0;
				}
			}
		}
		break;

		case EXNIP_PACKET_SIGNATURE_XNET_REQUEST:
		{
			XNetRequestPacket* requestPacket = reinterpret_cast<XNetRequestPacket*>(lpBuffers[0].buf);
			if (*lpBytesRecvdCount == sizeof(XNetRequestPacket)
				&& strncmp(requestPacket->pckHeader.signatureString, XNIP_REQUEST_HEADER_STR, XNIP_MAX_PCK_STR_HDR_LEN) == 0)
			{
				LOG_INFO_NETWORK("{} - Received XNetRequest type: {} from ip address {:X}, port: {}, virtual: {}",
					__FUNCTION__,
					(int)requestPacket->data.reqType,
					ntohl(lpFrom->sin_addr.s_addr),
					ntohs(lpFrom->sin_port),
					ntohs(requestPacket->data.senderVirtualPort)
					);

				HandleXNetRequestPacket(xsocket, requestPacket, lpFrom, lpBytesRecvdCount); // save NAT info and send back a connection packet

				// set the bytes received count to 0
				// should signal the callee to poll another packet, so the game is kept fed with data if any
				*lpBytesRecvdCount = 0;
				return SOCKET_ERROR;
			}
		}
		break;

		default:
			break;
		} // switch (packet->signature)
	}

	IN_ADDR ipIdentifier;
	// let the game know the packet received came from an unkown source
	if (!GetEstablishedConnectionIdentifierByRecvAddr(xsocket, lpFrom, &ipIdentifier))
	{
		lpFrom->sin_addr = ipIdentifier;
		lpFrom->sin_port = xsocket->GetNetworkOrderSocketVirtualPort(); // Virtual socket ID, not the actual port
		UpdatePacketReceivedCounters(ipIdentifier, *lpBytesRecvdCount);
		return 0;
	}
	else
	{
		// set the bytes received count to 0 and recv address/identifier
		// when the packet comes from an unknown source
		LOG_ERROR_NETWORK("{} - discarding packet with size: {}", __FUNCTION__, *lpBytesRecvdCount);
		*lpBytesRecvdCount = 0;
		ZeroMemory(lpFrom, sizeof(*lpFrom));
		return SOCKET_ERROR;
	}
}

// Outputs the connection identifier
// Returns error if connection is not found or connected
int XnIpManager::GetEstablishedConnectionIdentifierByRecvAddr(XVirtualSocket* xsocket, const sockaddr_in* fromAddr, IN_ADDR* outConnectionIdentifier) const
{
	outConnectionIdentifier->s_addr = 0;

	for (int i = 0; i < GetMaxXnConnections(); i++)
	{
		XnIp* xnIp = &m_XnIPs[i];
		if (xnIp->m_valid
			&& xnIp->ConnectStatusConnected())
		{
			const sockaddr_in* mapping = xnIp->GetPortMapping(xsocket->GetNetworkOrderSocketVirtualPort());

			if (mapping == nullptr)
			{
				LOG_ERROR_NETWORK("{} - no port mapping for ip address: {}:{}", __FUNCTION__, inet_ntoa(fromAddr->sin_addr), ntohs(fromAddr->sin_port));
				return WSAEINVAL;
			}

			if (xsocket->SockAddrInEqual(fromAddr, mapping))
			{
				*outConnectionIdentifier = xnIp->GetConnectionId();
				return 0;
			}
		}
	}

	LOG_ERROR_NETWORK("{} - received packet from unknown/unregistered source, ip address: {}:{}", __FUNCTION__, inet_ntoa(fromAddr->sin_addr), ntohs(fromAddr->sin_port));
	return WSAEINVAL;
}

void XnIpManager::SetupLocalConnectionInfo(unsigned long xnaddr, unsigned long lanaddr, unsigned short baseport, const char* machineUID, const char* abOnline)
{
	XnIpManager::UnregisterLocalConnectionInfo();

	m_ipLocal.m_xnaddr.ina = CNic::GetBestInterfaceRouteAddressFromIp("0.0.0.0"); // pass INADDR_ANY

	if (Ipv4AddressIsReservedOrLocalhost(m_ipLocal.m_xnaddr.ina))
	{
		LOG_TRACE_NETWORK("{} - IP address is private - {}", __FUNCTION__, inet_ntoa(m_ipLocal.m_xnaddr.ina));
	}
	else
	{
		// fall back to localhost if what GetBestIpToIpRoute found is not a private/local host ip address (maybe we are not under a NAT gateway)
		LOG_TRACE_NETWORK("{} - GetBestIpToIpRoute() returned public IP address, likely not under a NAT, falling back to localhost IP address!", __FUNCTION__);
		m_ipLocal.m_xnaddr.ina.s_addr = inet_addr("127.0.0.1");
	}

	// override what GetBestIpToIpRoute found if lanaddr is different than INADDR_ANY
	if (lanaddr != htonl(INADDR_NONE)
		&& lanaddr != htonl(INADDR_ANY))
	{
		m_ipLocal.m_xnaddr.ina.s_addr = lanaddr; // check if the lanaddr is specified already
		LOG_TRACE_NETWORK("{} - lanaddr already specified: {}", __FUNCTION__, inet_ntoa(m_ipLocal.m_xnaddr.ina));
	}

	m_ipLocal.m_xnaddr.inaOnline.s_addr = xnaddr;
	// create rc4 state from machine id key to build the abEnet
	XECRYPT_RC4_STATE rc4_engine_state;
	XeCryptRc4Key(&rc4_engine_state, (BYTE*)machineUID, sizeof(XNADDR::abEnet) * 2);

	m_ipLocal.m_xnaddr.wPortOnline = htons(baseport);
	XeCryptRc4Ecb(&rc4_engine_state, m_ipLocal.m_xnaddr.abEnet, sizeof(m_ipLocal.m_xnaddr.abEnet));
	//HexStrToBytes(std::string(machineUID, sizeof(XNADDR::abEnet) * 2), m_ipLocal.m_xnaddr.abEnet, sizeof(XNADDR::abEnet));
	HexStrToBytes(std::string(abOnline, sizeof(XNADDR::abOnline) * 2), m_ipLocal.m_xnaddr.abOnline, sizeof(XNADDR::abOnline));
	m_ipLocal.m_pckStats.PckDataSampleUpdate();

	//m_ipLocal.m_valid = g_XSockMgr.MainLinkSocketInitialize(htons(baseport));
	m_ipLocal.m_valid = true;
}

void XnIpManager::UnregisterLocalConnectionInfo()
{
	g_XSockMgr.MainLinkDispose();
	m_ipLocal.DiscardPortMappings();
	ZeroMemory(&m_ipLocal, sizeof(m_ipLocal));
}

void XnIpManager::ClearLostConnections()
{
	int lostConnectionsCount = 0;
	for (int i = 0; i < GetMaxXnConnections(); i++)
	{
		XnIp* xnIp = &m_XnIPs[i];
		if (xnIp->m_valid
			&& xnIp->ConnectionTimedOut())
		{
			lostConnectionsCount++;
			UnregisterXnIpIdentifier(xnIp->GetConnectionId());
		}
	}
	if (lostConnectionsCount > 0)
		LOG_CRITICAL_NETWORK("{} - lost {} connection(s)!", __FUNCTION__, lostConnectionsCount);
}

XnIp* XnIpManager::GetLocalUserXn()
{
	return &m_ipLocal;
}

void XnIpManager::HandleXNetRequestPacket(XVirtualSocket* xsocket, const XNetRequestPacket* reqPacket, const sockaddr_in* recvAddr, LPDWORD lpBytesRecvdCount)
{
	IN_ADDR connectionIdentifier;
	connectionIdentifier.s_addr = 0;

	int ret = CreateOrGetXnIpIdentifierFromPacket(&reqPacket->data.xnaddr, &reqPacket->data.xnkid, reqPacket, &connectionIdentifier);

	// if CreateOrGetXnIpIdentifierFromPacket is successful, another connection spot has been created
	// or an existing one is present
	if (ret == 0)
	{
		// TODO: get rid of H2v only sockets

		if (connectionIdentifier.s_addr == htonl(INADDR_LOOPBACK))
			return;

		XnIp* xnIp = GetConnection(connectionIdentifier);
		switch (reqPacket->data.reqType)
		{
		case EXNIP_CONNECTION_PORT_MAPPING_UPDATE:
		case EXNIP_CONNECTION_FINISH_ESTABLISH_SECURE_CHANNEL:
		case EXNIP_CONNECTION_ACKNOWLEDGE_CONNECTED_SECURE_CHANNEL:
		case EXNIP_CONNECTION_CLOSE_SECURE:
			xnIp->HandleConnectionPacket(xsocket, reqPacket, recvAddr, lpBytesRecvdCount);
			break;

			// TODO:
		case EXNIP_CONNECTION_PING:
		case EXNIP_CONNECTION_PONG:
			LOG_CRITICAL_NETWORK("{} - unimplemented request type: {}", __FUNCTION__, (int)reqPacket->data.reqType);
			break;

		default:
			break;
		} // switch (reqPacket->data.reqType)

		// increase packets received count
		int bytesReceivedCount = 0;
		if (lpBytesRecvdCount)
			bytesReceivedCount = *lpBytesRecvdCount;

		UpdatePacketReceivedCounters(connectionIdentifier, bytesReceivedCount);
	}
	else
	{
		LOG_TRACE_NETWORK("{} - secure connection cannot be established!", __FUNCTION__);
		LogConnectionsErrorDetails(recvAddr, ret, &reqPacket->data.xnkid);
		// TODO: send back the connection cannot be established
	}
}

XnIp* XnIpManager::XnIpLookup(const XNADDR* pxna, const XNKID* xnkid) const
{
	XnKeyPair* keyPair = KeyPairLookup(xnkid);
	if (keyPair == nullptr)
	{
		return nullptr;
	}

	// check if the user is already in the system, and get the first 
	for (int i = 0; i < GetMaxXnConnections(); i++)
	{
		// compare current connection index with what we got passed to us
		if (
			m_XnIPs[i].m_valid
			&& !memcmp(m_XnIPs[i].m_xnaddr.abEnet, pxna->abEnet, sizeof(XNADDR::abEnet)) // check the MAC address
			// && !memcmp(m_XnIPs[i].xnaddr.abOnline, pxna->abOnline, sizeof(XNADDR::abOnline))
			&& !memcmp(m_XnIPs[i].m_keyPair->m_xnkid.ab, keyPair->m_xnkid.ab, sizeof(XNKID::ab))
			)
		{
			// if it is already in the system, return
			return &m_XnIPs[i];
		}
	}

	return nullptr;
}

int XnIpManager::RegisterNewXnIp(const XNADDR* pxna, const XNKID* pxnkid, IN_ADDR* outIpIdentifier)
{
	/*
		Update: 1/31/2020
		- Creates an identifier to be used by WinSock calls from a XNADDR address
		- It gets created when we received a XNet request connection packet, or when the game calls XNetXnAddrToInAddr
		- If XNetInAddrToXnAddr passes an invalid identifier, it will return WSAEINVAL
	*/

	XnKeyPair* keyPair = KeyPairLookup(pxnkid);
	if (keyPair == nullptr)
	{
		// session key is invalid 
		return WSAEINVAL;
	}

	for (int i = 0; i < GetMaxXnConnections(); i++)
	{
		if (!m_XnIPs[i].m_valid)
		{
			XnIp* newXnIp = &m_XnIPs[i];
			ZeroMemory(newXnIp, sizeof(*newXnIp));
			memcpy(&newXnIp->m_xnaddr, pxna, sizeof(*pxna));

			int randIdentifier = (rand() % 0xFF) + 1; // 0 to 254 + 1
			randIdentifier <<= 8;

			XNetRandom(newXnIp->m_nonce, sizeof(XnIp::m_nonce));
			newXnIp->m_keyPair = keyPair;
			newXnIp->m_connectionId.s_addr = htonl(i | randIdentifier);
			newXnIp->m_valid = true;
			LOG_INFO_NETWORK("{} - registered new connection, index {}, identifier {:X}, n0nce key: {}",
				__FUNCTION__,
				i,
				htonl(i | randIdentifier),
				ByteToHexStr(newXnIp->m_nonce, sizeof(XnIp::m_nonce)).c_str()
			);

			// update the state
			newXnIp->UpdateInteractionTimeHappened();
			newXnIp->SetConnectStatus(XNET_CONNECT_STATUS_IDLE);
			newXnIp->m_pckStats.PckDataSampleUpdate();

			if (outIpIdentifier)
				*outIpIdentifier = newXnIp->GetConnectionId();

			for (auto socket : g_XSockMgr.sockets)
			{
				WORD virtualPort = socket->GetHostOrderSocketVirtualPort();
				if (virtualPort != 1000
					&& virtualPort != 1001)
					continue;

				PortMapping map;
				ZeroMemory(&map, sizeof(map));
				map.virtualPort = socket->GetNetworkOrderSocketVirtualPort();
				map.state = PortMapping::PortMapState::XNIP_NET_ADDRESS_MAPPINGS_UNAVAILABLE;
				newXnIp->InsertPortMapping(&map);
			}

			return 0;
		}
	}

	// if we get this far, no more connection spots available
	return WSAENOMORE;
}

void XnIp::UpdateNonceKeyFromPacket(const XNetRequestPacket* reqPacket)
{
	// if we got a request to create the new connection
	// update accordingly with the data received
	if (m_endpointNonceValid)
	{
		LOG_WARNING_NETWORK("{} - n0nce key already updated for connection id: {:X}",
			__FUNCTION__,
			GetConnectionId().s_addr
		);
		return;
	}

	memcpy(m_endpointNonce, reqPacket->data.nonceKey, sizeof(XnIp::m_endpointNonce));
	m_endpointNonceValid = true;

	LOG_TRACE_NETWORK("{} - updated noOnce key for connection id: {:X}, n0nceKey: {}",
		__FUNCTION__,
		GetConnectionId().s_addr,
		ByteToHexStr(m_endpointNonce, 8).c_str()
	);
}

// TODO: add separate function that doesn't create XnIp identification from packet
// for now if reqPacket is null it means this is not called after a connection packet is received
// basically reqPacket = nullptr, XNetXnAddrToInAddr() context, reqPacket != nullptr, HandleXNetRequestPacket() context
int XnIpManager::CreateOrGetXnIpIdentifierFromPacket(const XNADDR* pxna, const XNKID* pxnkid, const XNetRequestPacket* reqPacket, IN_ADDR* outIpIdentifier)
{
	const XnIp* localConnectionInfo = GetLocalUserXn();
	if (!localConnectionInfo->m_valid)
	{
		LOG_TRACE_NETWORK("{} - XNADDR information is not populated!", __FUNCTION__);
		return WSAEADDRNOTAVAIL;
	}

	if (!memcmp(localConnectionInfo->m_xnaddr.abEnet, pxna->abEnet, sizeof(XNADDR::abEnet)))
	{
		// some retarded games use the MAC address as the unique player identifier
		// but in 2020 we want to connect from the same PC multiple game instances, and this has become a unique account identifier
		// but even then, we cannot allow xbox addresses with the same abEnet identifier
		LOG_CRITICAL_NETWORK("{} - the specified XNADDR is the same with the local one, aborting connection.", __FUNCTION__);
		LOG_CRITICAL_NETWORK("{} - local abEnet: {} == provided abEnet: {}",
			__FUNCTION__,
			ByteToHexStr(localConnectionInfo->m_xnaddr.abEnet, 6),
			ByteToHexStr(pxna->abEnet, 6));
		outIpIdentifier->s_addr = htonl(INADDR_LOOPBACK);
		return 0;
	}

	// HACK: clear lost connections (if any) before attempting to create another one
	ClearLostConnections();

	XnIp* existingConnection = XnIpLookup(pxna, pxnkid);

	bool fromPacket = reqPacket != nullptr;
	bool isAlreadyRegistered = existingConnection != nullptr;
	bool reconnectAttempt = false;

	if (isAlreadyRegistered
		&& fromPacket)
	{
		// test if endppoint's connection session key is different from the received packet
		bool connectionEstablishKeyMismatch = existingConnection->m_endpointNonceValid
			&& memcmp(existingConnection->m_endpointNonce, reqPacket->data.nonceKey, sizeof(XnIp::m_nonce)) != 0;

		if (connectionEstablishKeyMismatch)
		{
			// if so recreate the connection
			UnregisterXnIpIdentifier(existingConnection->GetConnectionId());
			existingConnection = nullptr;
			isAlreadyRegistered = false;
			reconnectAttempt = true;
		}
	}

	const auto registerConnection = [this](
		const XNADDR* pxna,
		const XNKID* pxnkid,
		IN_ADDR* outIpId,
		bool fromPacket,
		bool reconnectAttempt,
		const XNetRequestPacket* reqPacket) -> int
	{
		int result = RegisterNewXnIp(pxna, pxnkid, outIpId);
		// first check if RegisterNewXnIp was successful
		if (result == 0)
		{
			XnIp* newConnection = GetConnection(*outIpId);
			// if we received and created this identification after we received a packet, the other side tries to initiate a connection
			newConnection->m_connectionInitiator = fromPacket ? true : false;
			if (fromPacket)
			{
				newConnection->UpdateNonceKeyFromPacket(reqPacket);
				if (reconnectAttempt) {
					XNIP_SET_BIT(newConnection->m_flags, XnIp::XnIp_ReconnectionAttempt, true);
					LOG_WARNING_NETWORK("{} - reconnection attempt for connection id: {:X}",
						__FUNCTION__,
						newConnection->GetConnectionId().s_addr);
				}
			}
		}

		return result;
	}; /* registerConnection() lambda end */

	if (!isAlreadyRegistered)
	{
		return registerConnection(pxna, pxnkid, outIpIdentifier, fromPacket, reconnectAttempt, reqPacket);
	}
	else
	{
		if (fromPacket
			&& !existingConnection->m_endpointNonceValid)
		{
			// update the n0nce key from the packet received
			// this should be the case where 
			existingConnection->UpdateNonceKeyFromPacket(reqPacket);
		}

		// if we just received a request from a known connection
		// or XNetXnAddrToInAddr tries to register the connection
		// just output the identifier
		if (outIpIdentifier)
		{
			*outIpIdentifier = existingConnection->GetConnectionId();
			LOG_INFO_NETWORK("{} - existing connection id: {:X}", __FUNCTION__, existingConnection->GetConnectionId().s_addr);
		}

		return 0;
	}
}

void XnIpManager::UnregisterXnIpIdentifier(const IN_ADDR ina)
{
	// TODO: let the other connection end know that the connection has to be closed
	XnIp* xnIp = GetConnection(ina);
	if (xnIp != nullptr)
	{
		const XnIpPckTransportStats* pckStats;
		xnIp->PckGetStats(&pckStats);
		LOG_INFO_NETWORK("{} - packets sent: {}, packets recv'd {}", __FUNCTION__,
			pckStats->pckSent,
			pckStats->pckRecvd);
		LOG_INFO_NETWORK("{} - Unregistered connection index: {}, identifier: {:X}",
			__FUNCTION__,
			XnIp::GetConnectionIndex(ina),
			xnIp->GetConnectionId().s_addr);

		xnIp->DiscardPortMappings();
		SecureZeroMemory(xnIp, sizeof(*xnIp));
	}
}

int XnIpManager::RegisterKey(XNKID* pxnkid, XNKEY* pxnkey)
{
	if (KeyPairLookup(pxnkid) != nullptr)
	{
		LOG_TRACE_NETWORK("{} - XnKeyPair: xnkid {}, xnkey: {} already registered!",
			__FUNCTION__,
			ByteToHexStr(pxnkid->ab, sizeof(pxnkid->ab)).c_str(),
			ByteToHexStr(pxnkey->ab, sizeof(pxnkey->ab)).c_str()
		);
		return WSAEALREADY;
	}

	for (int i = 0; i < GetMaxXnKeyPairs(); i++)
	{
		XnKeyPair* keyPair = &m_XnKeyPairs[i];
		if (keyPair->m_valid == false)
		{
			keyPair->m_valid = true;
			memcpy(keyPair->m_xnkid.ab, pxnkid->ab, sizeof(XNKID::ab));
			memcpy(keyPair->m_xnkey.ab, pxnkey->ab, sizeof(XNKEY::ab));
			LOG_TRACE_NETWORK("{} - registered key: xnkid {}, xnkey: {}",
				__FUNCTION__,
				ByteToHexStr(keyPair->m_xnkid.ab, sizeof(XNKID::ab)).c_str(),
				ByteToHexStr(keyPair->m_xnkey.ab, sizeof(XNKEY::ab)).c_str()
			);
			return 0;
		}
	}

	LOG_CRITICAL_NETWORK("{} - reached max key registrations!", __FUNCTION__);
	return WSAENOMORE;
}

void XnIpManager::UnregisterKey(const XNKID* pxnkid)
{
	for (int i = 0; i < GetMaxXnKeyPairs(); i++)
	{
		XnKeyPair* keyPair = &m_XnKeyPairs[i];
		if (!memcmp(keyPair->m_xnkid.ab, pxnkid->ab, sizeof(XNKID::ab)))
		{
			// close any connections present on this key
			//for (int j = 0; j < GetMaxXnConnections(); j++)
			//{
			//	if (XnIPs[j].bValid
			//		&& XnIPs[j].keyPair == &XnKeyPairs[i])
			//		UnregisterXnIpIdentifier(XnIPs[j].connectionIdentifier);
			//}

			if (keyPair->m_valid)
			{
				SecureZeroMemory(keyPair, sizeof(*keyPair));
				return;
			}
		}
	}

	LOG_CRITICAL_NETWORK("{} - unknown session key id - {}", __FUNCTION__, ByteToHexStr(pxnkid->ab, sizeof(pxnkid->ab)));
}

XnKeyPair* XnIpManager::KeyPairLookup(const XNKID* pxnkid) const
{
	for (int i = 0; i < GetMaxXnKeyPairs(); i++)
	{
		XnKeyPair* keyPair = &m_XnKeyPairs[i];
		if (keyPair->m_valid
			&& !memcmp(keyPair->m_xnkid.ab, pxnkid->ab, sizeof(XNKID::ab))
			)
		{
			return keyPair;
		}
	}

	LOG_WARNING_NETWORK("{} - unknown session key id - {}", __FUNCTION__, ByteToHexStr(pxnkid->ab, sizeof(pxnkid->ab)));
	return nullptr;
}

// gets the actual connection index from a connection identifier
int XnIp::GetConnectionIndex(IN_ADDR connectionId)
{
	return (int)(connectionId.s_addr >> 24);
}

void XnIp::SavePortMapping(XVirtualSocket* xsocket, WORD virtualPort, const sockaddr_in* addr)
{
	LOG_TRACE_NETWORK("{} - socket: {}, connection index: {}, identifier: {:X}", __FUNCTION__,
		xsocket->systemSocketHandle, XnIp::GetConnectionIndex(GetConnectionId()), GetConnectionId().s_addr);

	switch (ntohs(virtualPort))
	{
	case 1000:
	case 1001:
		UpdatePortMapping(virtualPort, addr);
		break;
	default:
		break;
	}
}

void XnIp::SendXNetRequestAllSockets(eXnip_ConnectRequestType reqType)
{
	// send for each UDP socket, the other side may not have the NAT data
	for (auto sockIt : g_XSockMgr.sockets)
	{
		// connect only UDP sockets
		if (sockIt->IsUDP())
		{
			if (sockIt->GetHostOrderSocketVirtualPort() != 1000
				&& sockIt->GetHostOrderSocketVirtualPort() != 1001)
				continue;

			SendXNetRequest(sockIt, reqType);
		}
	}
	return;
}

void XnIp::SendXNetRequest(XVirtualSocket* xsocket, eXnip_ConnectRequestType reqType)
{
	sockaddr_in sendToAddr;
	ZeroMemory(&sendToAddr, sizeof(sockaddr_in));

	sendToAddr.sin_family = AF_INET;
	sendToAddr.sin_addr = GetConnectionId();
	sendToAddr.sin_port = xsocket->GetNetworkOrderSocketVirtualPort();

	XNetRequestPacket reqPacket;

	reqPacket.data.reqType = reqType;
	memcpy(reqPacket.data.xnkid.ab, m_keyPair->m_xnkid.ab, sizeof(XNKID::ab));
	memcpy(reqPacket.data.nonceKey, m_nonce, sizeof(XnIp::m_nonce));
	XNetGetTitleXnAddr(&reqPacket.data.xnaddr);

	if (PortMappingsAvailable())
		XNIP_SET_BIT(reqPacket.data.flags, XnIp_HasPortMappingsUpdated, true);
	reqPacket.data.connectionInitiator = !InitiatedConnectRequest();
	reqPacket.data.senderVirtualPort = xsocket->GetNetworkOrderSocketVirtualPort();

	switch (reqType)
	{
	case EXNIP_CONNECTION_PORT_MAPPING_UPDATE:
	case EXNIP_CONNECTION_FINISH_ESTABLISH_SECURE_CHANNEL:
		break;

	case EXNIP_CONNECTION_ACKNOWLEDGE_CONNECTED_SECURE_CHANNEL:
		break;

	case EXNIP_CONNECTION_CLOSE_SECURE:
	case EXNIP_CONNECTION_PONG:
	case EXNIP_CONNECTION_PING:
		LOG_CRITICAL_NETWORK("{} - unimplemented requests!", __FUNCTION__);
		break;

	default:
		// this shouldn't be executed ever
		LOG_ERROR_NETWORK("{} - invalid request type", __FUNCTION__);
		break;
	}

	m_connectionPacketsSentCount++;

	m_requestContext = true;
	int udp_send_result = xsocket->UdpSend((const char*)&reqPacket, sizeof(XNetRequestPacket), 0, (sockaddr*)&sendToAddr, sizeof(sendToAddr));
	LOG_INFO_NETWORK("{} - request sent, socket handle: {}, connection index: {}, connection id: {:x}, n0nceKey: {}",
		__FUNCTION__,
		xsocket->systemSocketHandle,
		XnIp::GetConnectionIndex(GetConnectionId()),
		GetConnectionId().s_addr,
		ByteToHexStr(reqPacket.data.nonceKey, 8).c_str()
	);
}

void XnIp::HandleConnectionPacket(XVirtualSocket* xsocket, const XNetRequestPacket* reqPacket, const sockaddr_in* recvAddr, LPDWORD lpBytesRecvdCount)
{
	switch (reqPacket->data.reqType)
	{
	case EXNIP_CONNECTION_PORT_MAPPING_UPDATE:
		if (GetConnectStatus() > XNET_CONNECT_STATUS_PENDING)
		{
			break;
		}

		SavePortMapping(xsocket, reqPacket->data.senderVirtualPort, recvAddr);

		// check if port mappings are available, after saving them
		if (PortMappingsAvailable())
		{
			if (XNIP_TEST_BIT(reqPacket->data.flags, XnIp_HasPortMappingsUpdated)) // don't send back if the other end has our port mappings already
			{
				SendXNetRequest(xsocket, EXNIP_CONNECTION_FINISH_ESTABLISH_SECURE_CHANNEL); // send EstablishSecure after we saved all NAT
			}
			else
			{
				SendXNetRequestAllSockets(EXNIP_CONNECTION_PORT_MAPPING_UPDATE);
			}
			SetConnectStatus(XNET_CONNECT_STATUS_PENDING);
		}
		break;

	case EXNIP_CONNECTION_FINISH_ESTABLISH_SECURE_CHANNEL:
		if (!ConnectStatusPending())
		{
			break;
		}
		// when 2 peers try to connect to each other at the same time
		// determine which peer should handle the connecting by who has a bigger MAC address
		// or if the other side doesn't consider itself a connection initiator, just send
		// if the other connection cannot receive packets (ports are not forwarded), connectReqPkt->data.connectionInitiator will always be true
		if (!XNIP_TEST_BIT(m_flags, XnIp::XnIp_ConnectDeclareConnectedRequestSent))
		{
			// prevent this being sent twice, this will happen if the game Calls XNetConnect at the same time on each endpoint
			// and both ends have the ports open
			if (!reqPacket->data.connectionInitiator
				|| memcmp(gXnIpMgr.GetLocalUserXn()->m_xnaddr.abEnet, reqPacket->data.xnaddr.abEnet, sizeof(XNADDR::abEnet)) > 0
				)
			{
				SendXNetRequest(xsocket, EXNIP_CONNECTION_ACKNOWLEDGE_CONNECTED_SECURE_CHANNEL);
			}
			else
			{
				SendXNetRequest(xsocket, EXNIP_CONNECTION_FINISH_ESTABLISH_SECURE_CHANNEL);
			}
			XNIP_SET_BIT(m_flags, XnIp::XnIp_ConnectDeclareConnectedRequestSent, true);
		}
		break;

		// this will prevent the connection to send packets before we know we can send them
	case EXNIP_CONNECTION_ACKNOWLEDGE_CONNECTED_SECURE_CHANNEL:
		if (!ConnectStatusPending())
		{
			break;
		}
		switch (GetConnectStatus())
		{
		case XNET_CONNECT_STATUS_PENDING:
			LOG_TRACE_NETWORK("{} - connection id: {:X} successfully connected", __FUNCTION__, GetConnectionId().s_addr);
			SendXNetRequest(xsocket, EXNIP_CONNECTION_ACKNOWLEDGE_CONNECTED_SECURE_CHANNEL);
			// reset the flag if we successfully re-connected
			XNIP_SET_BIT(m_flags, XnIp_ReconnectionAttempt, false);
			SetConnectStatus(XNET_CONNECT_STATUS_CONNECTED);
			break;

		default:
			LOG_ERROR_NETWORK("{} - connection id: {:X} declare connected during non-pending status", __FUNCTION__, GetConnectionId().s_addr);
			break;
		}
		break;

	case EXNIP_CONNECTION_CLOSE_SECURE:
		//HandleDisconnectPacket(xsocket, packet, lpFrom);
		//LOG_TRACE_NETWORK("{} - Received ConnectionCloseSecure request from ip address {:x}, port: {}", __FUNCTION__, htonl(lpFrom->sin_addr.s_addr), htons(lpFrom->sin_port));
		break;
	} // switch (reqPacket->data.reqType)
}

void XnIp::HandleDisconnectPacket(XVirtualSocket* xsocket, const XNetRequestPacket* disconnectReqPck, const sockaddr_in* recvAddr)
{
	// TODO: implement graceful connection disconnect
}

// ------------------------
#pragma region XNet APIs
// ------------------------

// #51: XNetStartup
int WINAPI XNetStartup(const XNetStartupParams* pxnsp)
{
	LOG_TRACE_NETWORK("XNetStartup()");
	gXnIpMgr.Initialize(pxnsp);
	return 0;
}

// #52: XNetCleanup
INT WINAPI XNetCleanup()
{
	LOG_TRACE_NETWORK("XNetCleanup()");

	gXnIpMgr.Dispose();

	return 0;
}

// #53: XNetRandom
INT WINAPI XNetRandom(BYTE* pb, UINT cb)
{
	static bool Rc4CryptInitialized = false;

	if (Rc4CryptInitialized == false)
	{
		LARGE_INTEGER key;
		QueryPerformanceCounter(&key);
		XeCryptRc4Key(&gRc4StateRand, (BYTE*)&key, sizeof(key));
		Rc4CryptInitialized = true;
	}

	XeCryptRc4Ecb(&gRc4StateRand, pb, cb);
	return 0;
}

// #54: XNetCreateKey
INT WINAPI XNetCreateKey(XNKID* pxnkid, XNKEY* pxnkey)
{
	LOG_TRACE_NETWORK("XNetCreateKey()");
	if (pxnkid && pxnkey)
	{
		XNetRandom(pxnkid->ab, sizeof(pxnkid->ab));
		XNetRandom(pxnkey->ab, sizeof(pxnkey->ab));
		pxnkid->ab[0] &= ~XNET_XNKID_MASK;
		pxnkid->ab[0] |= XNET_XNKID_SYSTEM_LINK_XPLAT;
	}

	return 0;
}

// #57: XNetXnAddrToInAddr
INT WINAPI XNetXnAddrToInAddr(const XNADDR* pxna, const XNKID* pxnkid, IN_ADDR* pina)
{
	if (pxna == nullptr
		|| pxnkid == nullptr
		|| pina == nullptr)
		return WSAEINVAL;

	int ret = gXnIpMgr.CreateOrGetXnIpIdentifierFromPacket(pxna, pxnkid, nullptr, pina);

	if (ret != 0)
	{
		gXnIpMgr.LogConnectionsErrorDetails(nullptr, ret, pxnkid);
		return ret;
	}

	LOG_TRACE_NETWORK("{} - XNADDR local-address: {:X}, online-address: {:X}", __FUNCTION__, ntohl(pxna->ina.s_addr), ntohl(pxna->inaOnline.s_addr));
	//LOG_TRACE_NETWORK("{} - XnIp online-address: {:X}", __FUNCTION__, ntohl(xnIp->GetOnlineIpAddr().s_addr));
	return ret;
}

// #60: XNetInAddrToXnAddr
INT WINAPI XNetInAddrToXnAddr(const IN_ADDR ina, XNADDR* pxna, XNKID* pxnkid)
{
	LIMITED_LOG(15, LOG_TRACE_NETWORK, "{} - connection index: {}, identifier {:x}", __FUNCTION__, XnIp::GetConnectionIndex(ina), ina.s_addr);

	if (pxna)
		ZeroMemory(pxna, sizeof(*pxna));

	if (pxnkid)
		ZeroMemory(pxnkid->ab, sizeof(pxnkid->ab));

	XnIp* xnIp = gXnIpMgr.GetConnection(ina);
	if (xnIp == nullptr)
	{
		LOG_CRITICAL_NETWORK("{} - connection index: {}, identifier: {:X} are invalid!", __FUNCTION__, XnIp::GetConnectionIndex(ina), ina.s_addr);
		return WSAEINVAL;
	}

	if (pxna)
		memcpy(pxna, &xnIp->m_xnaddr, sizeof(*pxna));

	if (pxnkid)
		memcpy(pxnkid->ab, xnIp->m_keyPair->m_xnkid.ab, sizeof(XNKID::ab));

	return 0;
}

// #63: XNetUnregisterInAddr
int WINAPI XNetUnregisterInAddr(const IN_ADDR ina)
{
	LOG_TRACE_NETWORK("{} - connection index {}, identifier: {:X}", __FUNCTION__, XnIp::GetConnectionIndex(ina), ina.s_addr);
	gXnIpMgr.UnregisterXnIpIdentifier(ina);
	return 0;
}

// #65: XNetConnect
int WINAPI XNetConnect(const IN_ADDR ina)
{
	LOG_TRACE_NETWORK("{} - connection index {}, identifier: {:X}", __FUNCTION__, XnIp::GetConnectionIndex(ina), ina.s_addr);

	XnIp* xnIp = gXnIpMgr.GetConnection(ina);
	if (xnIp == nullptr)
	{
		LOG_CRITICAL_NETWORK("{} - connection index: {}, identifier: {:X} is invalid!", __FUNCTION__, XnIp::GetConnectionIndex(ina), ina.s_addr);
		return WSAEINVAL;
	}

	// send connect packets only if the state is idle
	if (xnIp->GetConnectStatus() == XNET_CONNECT_STATUS_IDLE)
	{
		xnIp->SendXNetRequestAllSockets(EXNIP_CONNECTION_PORT_MAPPING_UPDATE);
		xnIp->SetConnectStatus(XNET_CONNECT_STATUS_PENDING);
	}

	return 0;
}

// #66: XNetGetConnectStatus
int WINAPI XNetGetConnectStatus(const IN_ADDR ina)
{
	LIMITED_LOG(15, LOG_TRACE_NETWORK, "{} : connection index {}, identifier: {:x}", __FUNCTION__, XnIp::GetConnectionIndex(ina), ina.s_addr);

	XnIp* xnIp = gXnIpMgr.GetConnection(ina);
	if (xnIp == nullptr)
	{
		LOG_CRITICAL_NETWORK("{} - connection index: {}, identifier: {:X} is invalid!", __FUNCTION__, XnIp::GetConnectionIndex(ina), ina.s_addr);
		return XNET_CONNECT_STATUS_LOST;
	}
	
	// ### TODO FIXME the case described below is disabled (needs more research or proper implementation)
	// check if we're either connected
	// or not attempting to reconnect and STATUS PENDING
	// this will prevent the game from recreating secure connections when the time out is reached
	// and the connection was never established, causing XNET_CONNECT_STATUS_LOST status to be issued
	// basically creating a connection/endpoint unreachable case (to note just for Halo 2)
	// while preventing connections from stalling after a failed attempt to recover connection
	// from a previous successful connection that has been discarded in the meantime due to various reasons
	if (xnIp->ConnectStatusConnected()
		//|| (xnIp->ConnectStatusPending() && !XNIP_TEST_BIT(xnIp->m_flags, XnIp::XnIp_ReconnectionAttempt))
		)
	{
		xnIp->UpdateInteractionTimeHappened();
	}
	
	if (xnIp->GetConnectStatus() < XNET_CONNECT_STATUS_CONNECTED
		&& xnIp->ConnectionTimedOut())
	{
		return XNET_CONNECT_STATUS_LOST;
	}

	return xnIp->GetConnectStatus();
}

// #73: XNetGetTitleXnAddr
DWORD WINAPI XNetGetTitleXnAddr(XNADDR* pxna)
{
	LIMITED_LOG(15, LOG_TRACE_NETWORK, "{}", __FUNCTION__);
	ZeroMemory(pxna, sizeof(*pxna));

	XnIp* localUserXnIp = gXnIpMgr.GetLocalUserXn();
	if (!localUserXnIp->m_valid)
		return XNET_GET_XNADDR_PENDING;

	memcpy(pxna, &localUserXnIp->m_xnaddr, sizeof(*pxna));
	return XNET_GET_XNADDR_ETHERNET;
}

// #55: XNetRegisterKey
int WINAPI XNetRegisterKey(XNKID* pxnkid, XNKEY* pxnkey)
{
	LOG_TRACE_NETWORK("XNetRegisterKey()");
	return gXnIpMgr.RegisterKey(pxnkid, pxnkey);
}

// #56: XNetUnregisterKey
int WINAPI XNetUnregisterKey(const XNKID* pxnkid)
{
	LOG_TRACE_NETWORK("XNetUnregisterKey()");
	gXnIpMgr.UnregisterKey(pxnkid);

	return 0;
}

// ------------------------
#pragma endregion
// ------------------------