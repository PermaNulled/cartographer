#include "stdafx.h"
#include "upnp.h"

#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>

#include "H2MOD/Modules/Shell/Config.h"

/* Ripped from ED thanks guys - PermaNull*/
ModuleUPnP::ModuleUPnP()
{
	upnpDevice = upnpDiscover(2000, NULL, NULL, 0, 0, 2, &upnpDiscoverError);
}

Utils::UPnPResult ModuleUPnP::UPnPForwardPort(bool tcp, int externalport, int internalport, const std::string & ruleName)
{
	struct UPNPUrls urls;
	struct IGDdatas data;
	char lanaddr[16];

	if (upnpDiscoverError != UPNPDISCOVER_SUCCESS || upnpDevice == nullptr)
		return Utils::UPnPResult(Utils::UPnPErrorType::DiscoveryError, upnpDiscoverError);

	int ret = UPNP_GetValidIGD(upnpDevice, &urls, &data, lanaddr, sizeof(lanaddr), NULL, 0);

	if (ret != Utils::UPNP_IGD_VALID_CONNECTED)
		return Utils::UPnPResult(Utils::UPnPErrorType::IGDError, ret);

	ret = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
		std::to_string(externalport).c_str(), std::to_string(internalport).c_str(),
		lanaddr, ruleName.c_str(), tcp ? "TCP" : "UDP", NULL, NULL);

	Utils::UPnPErrorType type = Utils::UPnPErrorType::None;
	if (ret != UPNPCOMMAND_SUCCESS)
		type = Utils::UPnPErrorType::PortMapError;

	FreeUPNPUrls(&urls);

	return Utils::UPnPResult(type, ret);
}

void ForwardPorts()
{
	if (!H2Config_upnp_enable)
	{
		LOG_INFO_NETWORK("ForwardPorts() - UPNP disabled by config, skipping forwarding.");
		return;
	}

	ModuleUPnP upnp;
	Utils::UPnPResult upnpResult(Utils::UPnPErrorType::None, 0);

	upnpResult = upnp.UPnPForwardPort(false, H2Config_base_port, H2Config_base_port, "Halo2_port");
	LOG_INFO_NETWORK("ForwardPorts() - Halo2 port forwarding result: {}", upnpResult.ErrorCode);

	upnpResult = upnp.UPnPForwardPort(false, H2Config_base_port + 1, H2Config_base_port + 1, "Halo2_port1");
	LOG_INFO_NETWORK("ForwardPorts() - Halo2 port forwarding result: {}", upnpResult.ErrorCode);

	upnpResult = upnp.UPnPForwardPort(true, H2Config_base_port + 10, H2Config_base_port + 10, "Halo2_port_QoS");
	LOG_INFO_NETWORK("ForwardPorts() - Halo2 QoS forwarding result: {}", upnpResult.ErrorCode);

	LOG_INFO_NETWORK("ForwardPorts() - Finished forwarding ports.");
}

DWORD WINAPI XLiveGetUPnPState(DWORD a1)
{
	LOG_TRACE_XLIVE("XLiveGetUPnPState  (a1 = {:x})", a1);
	return 0;
}
