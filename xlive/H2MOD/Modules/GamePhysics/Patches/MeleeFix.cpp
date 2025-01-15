#include "stdafx.h"
#include "MeleeFix.h"

#include "game/game_time.h"
#include "memory/data.h"
#include "tag_files/global_string_ids.h"
#include "physics/character_physics_mode_melee.h"

namespace MeleeFix
{
	//////////////////////////////////////////////////////
	////////////////////Experimental//////////////////////
	//////////////////////////////////////////////////////

	typedef int(__cdecl melee_get_time_to_target_t)(unsigned __int16 object_index);
	melee_get_time_to_target_t* p_melee_get_time_to_target;

	typedef void(__cdecl melee_damage_t)(int object_index, int melee_type, char unk2, float unk3);
	melee_damage_t* p_melee_damage;

	typedef void (__cdecl send_melee_damage_simulation_event_t)(int a1, int a2, int arg8);
	send_melee_damage_simulation_event_t* p_send_melee_damage_simulation_event;

	typedef void (__cdecl melee_environment_damage_t)(int a1, int arg4, int arg8);
	melee_environment_damage_t* p_melee_environment_damage;

	typedef void (__cdecl sub_88B54F_t)(int a2, int a3);
	sub_88B54F_t* p_sub_88B54F;
	bool MeleeHit = false;

	void __cdecl melee_environment_damage(int a1, int arg4, int arg8)
	{
		MeleeHit = true;
		p_melee_environment_damage(a1, arg4, arg8);
		//LOG_INFO_GAME("[MeleeFix] Environment Hit {} {} {}", a1, arg4, arg8);
	}

	void __cdecl send_melee_damage_simulation_event(int a1, int a2, int arg8)
	{
		MeleeHit = true;
		p_send_melee_damage_simulation_event(a1, a2, arg8);
		//LOG_INFO_GAME("[MeleeFix] Packet Send {} {} {}", a1, a2, arg8);
	}

	void __cdecl sub_88B54F(int a2, int a3)
	{
		MeleeHit = true;
		p_sub_88B54F(a2, a3);
		//LOG_INFO_GAME("[MeleeFix] unk {} {}", a2, a3);
	}
	

	int melee_next_animation_hook(int a1, int a2, int target_player, char a4)
	{
		int result;
		__asm
		{
			push a4
			push target_player
			push a2
			mov eax, a1
			mov edx, [0x167948]
			add edx, [Memory::baseAddress]
			call edx
			add esp, 12
			mov result, eax
		}
		return result;
	}

	void __cdecl simulation_melee_action_update_animation(int object_index)
	{
		s_data_array* objects = *Memory::GetAddress<s_data_array**>(0x4E461C);

		int biped_object = *(DWORD *)&objects->data[12 * (unsigned __int16)object_index + 8];
		int melee_info_offset = *(__int16 *)(biped_object + 858); //???
		auto biped_melee_info = (s_melee_animation_datum *)(biped_object + melee_info_offset);
		int melee_type = biped_melee_info->melee_type_string_id;

		if (biped_melee_info->melee_animation_update == biped_melee_info->max_animation_range || biped_melee_info->melee_animation_update == 0)
			MeleeHit = false;

		if (melee_type == -1)
		{
			biped_melee_info->melee_flags &= 0xF7FFFFFF;
			biped_melee_info->melee_type_string_id = -1;
			MeleeHit = false;
		}
		else
		{
			bool abort_melee_action = false;
			if (!MeleeHit) 
			{
				float currentFrame = (float)biped_melee_info->melee_animation_update;
				float actionFrame = (float)biped_melee_info->animation_action_index;
				//Static tolerance.. should probably be calculated based on the total frames in the animation?
				float tolerance = 0.1f;
				//This is to calculate how many frames before and after the action frame a melee should be possible..
				float leeway = (float)(biped_melee_info->max_animation_range) * tolerance / 2;
				LOG_TRACE_GAME("[MeleeFix] Frame Data: Current Frame {} Action Frame {} Leeway {}", currentFrame, actionFrame, leeway);

				if (currentFrame >= actionFrame - leeway && currentFrame <= actionFrame + leeway)
				{
					p_melee_damage(object_index, melee_type, biped_melee_info->field_30, (real32)(uint8)biped_melee_info->field_31 * 0.003921569f);
					if (MeleeHit) 
					{
						LOG_TRACE_GAME("[MeleeFix] Melee Hit!");
					}
					else 
					{
						LOG_TRACE_GAME("[MeleeFix] Melee Missed!");
					}
				}
			}

			/*if (biped_melee_info->melee_animation_update == biped_melee_info->animation_action_index) 
			{
				melee_damage(object_index, melee_type, biped_melee_info->field_30, (float)(unsigned __int8)biped_melee_info->field_31 * 0.0039215689);
			}*/
			
			// this is used only for sword
			if (melee_type == _string_id_melee_dash || melee_type == _string_id_melee_dash_airborne)
			{
				real32 melee_max_duration = melee_type == _string_id_melee_dash_airborne ? 0.22f : 0.15f;
				int32 melee_max_ticks = time_globals::seconds_to_ticks_round(melee_max_duration);
				if (melee_max_ticks < 0 || p_melee_get_time_to_target(object_index) <= melee_max_ticks)
					abort_melee_action = true;
			}
			if ((++biped_melee_info->melee_animation_update >= (int)biped_melee_info->max_animation_range || abort_melee_action)
				&& !melee_next_animation_hook(object_index, 0, -1, biped_melee_info->field_30))
			{
				biped_melee_info->melee_flags &= 0xF7FFFFFF;
				biped_melee_info->melee_type_string_id = NONE;
				MeleeHit = false;
			}
		}
	}

	void ApplyHooks()
	{
		p_melee_get_time_to_target = Memory::GetAddress<melee_get_time_to_target_t*>(0x150784);
		p_melee_damage = Memory::GetAddress<melee_damage_t*>(0x142D62);
		p_send_melee_damage_simulation_event = Memory::GetAddress<send_melee_damage_simulation_event_t*>(0x1B8618);
		p_melee_environment_damage = Memory::GetAddress<melee_environment_damage_t*>(0x13F26D);
		p_sub_88B54F = Memory::GetAddress<sub_88B54F_t*>(0x17B54F);
		PatchCall(Memory::GetAddress(0x143440), send_melee_damage_simulation_event);
		PatchCall(Memory::GetAddress(0x14345A), melee_environment_damage);
		PatchCall(Memory::GetAddress(0x143554), sub_88B54F);
		WritePointer(Memory::GetAddress(0x41E524), (void*)simulation_melee_action_update_animation);
	}

	void Initialize()
	{

		// TODO FIXME doesn't fix melee lunge, leaving as reasearch
		// replace cvttss2si instruction which is the convert to int by truncation (> .5 decimal values don't mean anything, truncation rounding always towards 0) 
		// with cvtss2si instruction which reads the MXCSR register that holds the flags of the conversion rounding setting
		// that the game sets, which is Round Control Near (if decimal part > .5, convert to upper value)
		// when converting the tick count from float to int
		// otherwise the game will convert to tick count off by 1 tick
		// to note this in H3 is handled by adding .5, which does the same thing
		// BYTE cvtss2si[] = { 0xF3, 0x0F, 0x2D };
		// WriteBytes(Memory::GetAddressRelative(0x50B419, 0x4FDA49), cvtss2si, sizeof(cvtss2si));

		// hooks and re-implements melee lunge function
		// ApplyHooks();
		PatchCall(Memory::GetAddressRelative(0x50BD96, 0x4FE3C6), call_character_melee_physics_input_update_internal);
	}
}
