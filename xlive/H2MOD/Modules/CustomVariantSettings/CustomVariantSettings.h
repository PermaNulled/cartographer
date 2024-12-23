#pragma once
#include "physics/physics_constants.h"
#include "memory/bitstream.h"

namespace CustomVariantSettings
{
	enum e_hill_rotation
	{
		_random = 0,
		_sequential = 1,
		_reverse = 2,
		_predefined = 3,
		_invalid = -1
	};
	static const wchar_t* hill_rotation_name[] = {
		L"random",
		L"sequential",
		L"reverse",
		L"predefined"
	};
	struct s_variant_settings
	{
		double gravity = 1.0f;
		bool infiniteAmmo = false;
		bool explosionPhysics = false;
		e_hill_rotation hillRotation = _random;
		double gameSpeed = 1.0f;
		bool infiniteGrenades = false;
		bool spawnProtection = 1;
		byte predefinedHillSet[16];
		uint8 forced_fov = 0;

		double ComputedGravity() const
		{
			return gravity * s_physics_constants::get_default_gravity();
		}

		inline bool operator==(s_variant_settings& other) const
		{
			return memcmp(this, &other, sizeof(s_variant_settings)) == 0;
		}
		inline bool operator!=(s_variant_settings& other) const
		{
			return !(*this == other);
		}
	};
	void __cdecl EncodeVariantSettings(c_bitstream* stream, int a2, s_variant_settings* data);
	bool __cdecl DecodeVariantSettings(c_bitstream* stream, int a2, s_variant_settings* data);

	void ApplyCustomSettings(s_variant_settings* newVariantSettings);
	void UpdateCustomVariantSettings(s_variant_settings* data);
	void SendCustomVariantSettings(int32 peer_index);
	void ApplyHooks();
	void Initialize();
}

extern CustomVariantSettings::s_variant_settings currentVariantSettings;
extern std::map<std::wstring, CustomVariantSettings::s_variant_settings> customVariantSettingsMap;

#define CustomVariantSettingsPacketSize (sizeof(CustomVariantSettings::s_variant_settings))