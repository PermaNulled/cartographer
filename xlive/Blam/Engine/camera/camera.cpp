#include "stdafx.h"
#include "camera.h"

render_camera* get_global_camera()
{
	return Memory::GetAddress<render_camera*>(0x4E66C8);
}

render_camera* get_effect_camera()
{
	return Memory::GetAddress<render_camera*>(0xA3DF88);
}

void camera_apply_patches(void)
{
}
