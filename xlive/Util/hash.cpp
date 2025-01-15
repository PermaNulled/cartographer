#include "stdafx.h"

#include "hash.h"

bool hash_open_file(const wchar_t *file_name, HANDLE &file)
{
	file = CreateFileW(file_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	return LOG_CHECK(file != INVALID_HANDLE_VALUE);
}

bool hash_get_crypto_provider(DWORD type, HCRYPTPROV *provider)
{
	return LOG_CHECK(CryptAcquireContext(provider,
		NULL,
		NULL,
		type,
		CRYPT_VERIFYCONTEXT));
}

#define file_chunk_size 1024 * 32

bool hash_do_file_hashing(HANDLE file, HCRYPTHASH hash, DWORD flags, long long len)
{
	if (len == 0) {
		len = LONG_MAX;
	}
	BYTE file_chunk[file_chunk_size];
	DWORD len_to_read = (DWORD)min(file_chunk_size, len);
	DWORD bytes_read = 0;
	while (LOG_CHECK(ReadFile(file, file_chunk, len_to_read,
		&bytes_read, NULL)))
	{
		if (bytes_read == 0)
		{
			return true;
		}

		if (!LOG_CHECK(CryptHashData(hash, file_chunk, bytes_read, flags)))
		{
			return false;
		}
		len = len - bytes_read;
		if (len <= 0)
			return true;
		len_to_read = (DWORD)min(file_chunk_size, len);
	}
	return true;
}

std::string hashes::as_hex_string(BYTE *data, size_t data_len)
{
	std::stringstream ss;
	ss << std::setfill('0') << std::hex;
	for (size_t i = 0; i < data_len; i++)
	{
		ss << std::setw(2) << static_cast<int>(data[i]);
	}
	return ss.str();
}

#define md5_len 16

bool hashes::calc_file_md5(const std::wstring &filename, BYTE *checksum, size_t &checksum_len, long long len_read)
{
	HANDLE file;
	if (hash_open_file(filename.c_str(), file))
	{
		HCRYPTPROV crypto_provider = NULL;
		if (hash_get_crypto_provider(PROV_RSA_FULL, &crypto_provider))
		{
			HCRYPTHASH hash = NULL;
			if (LOG_CHECK(CryptCreateHash(crypto_provider, CALG_MD5, 0, 0, &hash)))
			{
				if (hash_do_file_hashing(file, hash, 0, len_read))
				{
					DWORD hash_len = min(checksum_len, md5_len);
					if (LOG_CHECK(CryptGetHashParam(hash, HP_HASHVAL, checksum, &hash_len, 0)))
					{
						checksum_len = hash_len;
						CryptDestroyHash(hash);
						CryptReleaseContext(crypto_provider, 0);
						CloseHandle(file);
						return true;
					}
					checksum_len = 0;
				}
				CryptDestroyHash(hash);
			}
			CryptReleaseContext(crypto_provider, 0);
		}
		CloseHandle(file);
	}
	LOG_TRACE_FUNCW(L"Failed to calculate md5 hash for {}", filename);
	return false;
}


bool hashes::calc_file_md5(const std::wstring &filename, std::string &checksum_out, long long len_read)
{
	BYTE hash_data[md5_len];
	size_t hash_len = ARRAYSIZE(hash_data);
	if (calc_file_md5(filename, hash_data, hash_len, len_read))
	{
		checksum_out = hashes::as_hex_string(hash_data, hash_len);
		return true;
	}
	return false;
}
