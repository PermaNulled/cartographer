#pragma once

#define BASE_IMAGE_ADDRESS_HALO2 0x00400000
#define BASE_IMAGE_ADDRESS_H2SERVER 0x00400000

class Memory
{
public:
	static void Initialize();

	static DWORD GetAddress()
	{
		return GetBaseAddress();
	}

	static DWORD GetAddress(DWORD client)
	{
		ASSERT(client != 0);
		return GetBaseAddress() + client;
	}

	static DWORD GetAddress(DWORD client, DWORD server)
	{
		ASSERT( Memory::IsDedicatedServer() || client != 0);
		ASSERT(!Memory::IsDedicatedServer() || server != 0);
		return GetBaseAddress() + (IsDedicatedServer() ? server : client);
	}
	
	template <typename T = void*>
	static T GetAddress(DWORD client)
	{
		return reinterpret_cast<T>((DWORD)GetAddress(client));
	}

	template <typename T = void*>
	static T GetAddress(DWORD client, DWORD server)
	{
		return reinterpret_cast<T>((DWORD)GetAddress(client, server));
	}

	static DWORD GetAddressRelative(DWORD client, DWORD server = 0)
	{
		return GetAddress(client - BASE_IMAGE_ADDRESS_HALO2, server - BASE_IMAGE_ADDRESS_H2SERVER);
	}

	template <typename T = void*>
	static T GetAddressRelative(DWORD client, DWORD server = 0)
	{
		return reinterpret_cast<T>((DWORD)GetAddress(client - BASE_IMAGE_ADDRESS_HALO2, server - BASE_IMAGE_ADDRESS_H2SERVER));
	}

	static void SetBaseAddress(DWORD base, bool isDedicatedServer)
	{
		baseAddress = base;
		dedicatedServer = isDedicatedServer;
	}

	// gets base address
	static DWORD GetBaseAddress() { return baseAddress; }
	static bool IsDedicatedServer() { return dedicatedServer; }

	static DWORD baseAddress;
	static bool dedicatedServer;
};

