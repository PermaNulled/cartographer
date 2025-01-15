#pragma once

#define k_maximum_string_id_storage 393216

/*********************************************************************
* string_id
* 4 BYTE Special Tag Structure for String Values
* Index 3 Bytes (first 24 bits)
* Length 1 Byte (upper 8 bits)
**********************************************************************/

typedef uint32 string_id;

void __cdecl user_interface_global_string_get(string_id id, wchar_t* dest);