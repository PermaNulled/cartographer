#include "stdafx.h"
#include "online_account_xbox.h"
#include "interface/user_interface_guide.h"

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