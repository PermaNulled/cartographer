#include "stdafx.h"
#include "screen_campaign_options_dialog.h"

void* c_screen_campaign_options::load(s_screen_parameters* parameters)
{
    return INVOKE(0xAFE8, 0x0, c_screen_campaign_options::load, parameters);
}
