#include "stdafx.h"
#include "online_account_xbox.h"
#include "interface/user_interface_guide.h"
#include "H2MOD/Modules/Accounts/AccountLogin.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Utils/Utils.h"

/* public code */

bool online_xuid_is_guest_account(XUID xuid)
{
	// return INVOKE(0x1AC4C0, 0 , online_xuid_is_guest_account, xuid);
	return (xuid & 3ULL) != 0;
}

uint8 online_xuid_get_guest_account_number(XUID xuid)
{
	// return INVOKE(0x1AC4C6, 0, online_xuid_get_guest_account_number,xuid);
	return (xuid & 3ULL);
}

bool __cdecl online_connected_to_xbox_live()
{
	//return INVOKE(0x1AC4A3, 0x0, online_connected_to_xbox_live);
	return user_interface_guide_state_manager_get()->m_sign_in_state == eXUserSigninState_SignedInToLive;
}

void online_account_transition_to_offline()
{
	BYTE abEnet[6];
	BYTE abOnline[20];
	XNetRandom(abEnet, sizeof(abEnet));
	XNetRandom(abOnline, sizeof(abOnline));
	ConfigureUserDetails("[Username]", "12345678901234567890123456789012", rand(), 0, H2Config_ip_lan, ByteToHexStr(abEnet, sizeof(abEnet)).c_str(), ByteToHexStr(abOnline, sizeof(abOnline)).c_str(), false);

	H2Config_master_ip = inet_addr("127.0.0.1");
	H2Config_master_port_relay = 2001;
	XUserSignInSetStatusChanged(0);
}