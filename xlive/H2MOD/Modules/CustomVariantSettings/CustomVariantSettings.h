#pragma once
#include "Blam\Engine\Game\PhysicsConstants.h"
#include "H2MOD\Modules\Networking\Memory\bitstream.h"

namespace CustomVariantSettings
{
	enum e_hill_rotation
	{
		random = 0,
		sequential = 1,
		reverse = 2,
		predefined = 3,
		invalid = -1
	};
	static const wchar_t* hill_rotation_name[] = {
		L"random",
		L"sequential",
		L"reverse",
		L"predefined"
	};
	struct s_variantSettings
	{
		double Gravity = 1.0f;
		bool InfiniteAmmo = false;
		bool ExplosionPhysics = false;
		e_hill_rotation HillRotation = random;
		double GameSpeed = 1.0f;
		bool InfiniteGrenades = false;
		byte SpawnProtection = 1;
		byte PredefinedHillSet[16];
		double ForcedFOV = 0;

		double ComputedGravity() const
		{
			return Gravity * physics_constants::get_default_gravity();
		}

		inline bool operator==(s_variantSettings& other) const
		{
			return memcmp(this, &other, sizeof(s_variantSettings)) == 0;
		}
		inline bool operator!=(s_variantSettings& other) const
		{
			return !(*this == other);
		}
	};
	void __cdecl EncodeVariantSettings(bitstream* stream, int a2, s_variantSettings* data);
	bool __cdecl DecodeVariantSettings(bitstream* stream, int a2, s_variantSettings* data);

	void ApplyCustomSettings(s_variantSettings* newVariantSettings);
	void UpdateCustomVariantSettings(s_variantSettings* data);
	void SendCustomVariantSettings(int peerIndex);
	void ApplyHooks();
	void Initialize();
}

extern CustomVariantSettings::s_variantSettings CurrentVariantSettings;
extern std::map<std::wstring, CustomVariantSettings::s_variantSettings> CustomVariantSettingsMap;

#define CustomVariantSettingsPacketSize (sizeof(CustomVariantSettings::s_variantSettings))