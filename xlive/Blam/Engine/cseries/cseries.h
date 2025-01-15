#pragma once

// 8-bit character that's specified as a multibyte utf8 string
typedef char utf8;

// 8-bit signed integer ranging from -128 to 127
typedef signed char int8;
static_assert(sizeof(int8) == 1);

// 8-bit unsigned integer ranging from 0 to 255
typedef unsigned char uint8;
static_assert(sizeof(uint8) == 1);

// 16-bit signed integer ranging from -32,768 to 32,767
typedef signed __int16 int16;
static_assert(sizeof(int16) == 2);

// 16-bit unsigned integer ranging from 0 to 65,535
typedef unsigned __int16 uint16;
static_assert(sizeof(uint16) == 2);

// 32-bit signed integer ranging from -2,147,483,648 to 2,147,483,647
typedef signed long int32;
static_assert(sizeof(int32) == 4);

// 32-bit unsigned integer ranging from 0 to 4,294,967,295
typedef unsigned long uint32;
static_assert(sizeof(uint32) == 4);

// 64-bit signed integer ranging from -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
typedef signed long long int64;
static_assert(sizeof(int64) == 8);

// 64-bit unsigned integer ranging from 0 to 18,446,744,073,709,551,615
typedef unsigned long long uint64;
static_assert(sizeof(uint64) == 8);

// 32-bit float
typedef float real32;
static_assert(sizeof(real32) == 4);

// 64-bit float
typedef double real64;
static_assert(sizeof(real64) == 8);

// 32-bit identifier: 16-bit index, 16-bit pseudounique "salt"
typedef long datum;
static_assert(sizeof(datum) == 4);

#define k_kilo 1024
#define k_unsigned_short_max 0xffff

#define SIZEOF_BITS(value) (8 * sizeof(value))
#define CHAR_BITS SIZEOF_BITS(int8)
#define SHORT_BITS SIZEOF_BITS(int16)
#define LONG_BITS SIZEOF_BITS(int32)

#if(!__INTEL_LLVM_COMPILER)
#define IS_MSVC_COMPILER
#endif

/* globals */

extern char g_temporary[256];

/* macros */

// Invokes a function
// ADDR_CLIENT: file offset in halo2.exe
// ADDR_SERVER: file offset in h2server.exe
// TYPE: function
// __VA_ARGS__: arguments for the function we want to invoke
// NOTE:
// if _addr_server is equal to 0 then we pass the client address as the server address (ONLY ON RELEASE BUILDS)
// REASON:
// optimizes out a cmov or related instruction when building release whenever we call a function that doesn't have a dedi address specified
#ifdef NDEBUG
#define INVOKE_BY_TYPE(_addr_client, _addr_server, _type, ...) \
Memory::GetAddress<_type>(_addr_client, (_addr_server) != 0 ? _addr_server : _addr_client)(__VA_ARGS__)
#else
#define INVOKE_BY_TYPE(_addr_client, _addr_server, _type, ...) \
Memory::GetAddress<_type>(_addr_client, _addr_server)(__VA_ARGS__)
#endif

#define INVOKE(_addr_client, _addr_server, _fn_decl, ...) INVOKE_BY_TYPE(_addr_client, _addr_server, decltype(_fn_decl)*, __VA_ARGS__)
// ### TODO find better name
#define INVOKE_TYPE(_addr_client, _addr_server, _type, ...) INVOKE_BY_TYPE(_addr_client, _addr_server, _type, __VA_ARGS__)

#define INVOKE_VFPTR_FN(_get_table_fn, _index, _type, ...) \
	(this->**_get_table_fn<_type>(_index))(__VA_ARGS__)

#define INVOKE_CLASS_FN(classobj, functionPtr)  ((classobj)->*(functionPtr))

#define NONE (-1)
#define DATUM_INDEX_NEW(_absolute_index, _salt) (datum)(((_salt) << 16) | (_absolute_index))
#define DATUM_INDEX_TO_ABSOLUTE_INDEX(_datum_index) ((uint16)((_datum_index) & 0xFFFF))
#define DATUM_INDEX_TO_IDENTIFIER(_datum_index) ((uint16)(((_datum_index) >> 16) & 0xFFFF))

#define NUMBEROF(_array) (sizeof(_array) / sizeof(*_array))
#define IN_RANGE(value, begin, end) (((value) >= (begin)) && ((value) <= (end)))
#define VALID_INDEX(index, count) ((index) >= 0 && (index) < (count))
#define VALID_COUNT(index, count) ((index) >= 0 && (index) <= (count))

// TODO remove padding macros
// Explanation:
// Harder to debug memory with it since types dont show up in the watch view
// Some values assigned to these macros actually contain data but are assigned incorrectly as padding

// PADDING MACROS
#define J( symbol1, symbol2 ) _DO_JOIN( symbol1, symbol2 )
#define _DO_JOIN( symbol1, symbol2 ) symbol1##symbol2
// Pad the desired length of BYTES and also hides it
#define PAD(BYTES)  char J(Unused_, __LINE__ )[BYTES];       

/// Add an anonymous 8-bit (1 byte) field to a structure.
#define PAD8 unsigned char : 8;
/// Add an anonymous 16-bit (2 byte) field to a structure.
#define PAD16 unsigned short : 16;
/// Add an anonymous 24-bit field to a structure.
#define PAD24 unsigned char : 8; unsigned short : 16;
/// Add an anonymous 32-bit (4 byte) field to a structure.
#define PAD32 unsigned long : 32;
/// Add an anonymous 48-bit field to a structure.
#define PAD48 unsigned short : 16; unsigned long : 32;
/// Add an anonymous 64-bit (8 byte) field to a structure.
#define PAD64 unsigned __int64 : 64;
/// Add an anonymous 128-bit (16 byte) field to a structure.
#define PAD128 unsigned __int64 : 64; unsigned __int64 : 64;

// Use this for setting up enum bitfields
// TODO: (uint64) cast is done as a hack to avoid warnings when doing the left shift on the final bit of a type
// Find a better way of dealing with this
#define FLAG(bit)( (uint64)1 << (uint64)(bit) )
#define TEST_BIT(flags, bit)( ((flags) & FLAG(bit)) != 0 )
#define TEST_FLAG(flag, flags)( ((flag) & (flags)) != 0 )
#define SET_FLAG(flags, bit, value)( (value) ? ((flags) |= FLAG(bit)) : ((flags) &= ~FLAG(bit)) )
#define SWAP_FLAG(flags, bit)			( (flags) ^=FLAG(bit) )
#define FLAG_RANGE(first_bit, last_bit)	( (FLAG( (last_bit)+1 - (first_bit) )-1) << (first_bit) )

#define BIT_VECTOR_SIZE_IN_LONGS(BIT_COUNT) (((BIT_COUNT) + (LONG_BITS - 1)) / LONG_BITS)
#define BIT_VECTOR_SIZE_IN_BYTES(BIT_COUNT) (4 * BIT_VECTOR_SIZE_IN_LONGS(BIT_COUNT))
#define BIT_VECTOR_TEST_FLAG(BIT_VECTOR, BIT) (TEST_BIT(BIT_VECTOR[(BIT) / LONG_BITS], ((BIT) & (LONG_BITS - 1))))
#define BIT_VECTOR_SET_FLAG(BIT_VECTOR, BIT, ENABLE) (SET_FLAG(BIT_VECTOR[(BIT) / LONG_BITS], ((BIT) & (LONG_BITS - 1)), ENABLE))

/// Creates a mask out of a count number of flags
#define MASK(count) ( (unsigned)FLAG(count) - (unsigned)1 )

#define ASSERT_STRUCT_SIZE(STRUCT, _SIZE)\
static_assert (sizeof(STRUCT) == (_SIZE), "Invalid size for struct ("#STRUCT") expected size (" #_SIZE")");

#define ASSERT_STRUCT_OFFSET(STRUCT,FIELD,OFFSET)\
static_assert (offsetof(STRUCT, FIELD) == (OFFSET), #STRUCT " Offset(" #OFFSET ") for " #FIELD " is invalid");

#ifdef ASSERTS_ENABLED
#define ASSERT_TRIGGER_EXCEPTION()					\
if (!is_debugger_present() && g_catch_exceptions)	\
	exit(-1);                                       \
else                                                \
	__debugbreak();                                 \
(void)0


#define DISPLAY_ASSERT_EXCEPTION(STATEMENT, IS_EXCEPTION)       \
display_assert(STATEMENT, __FILE__, __LINE__, IS_EXCEPTION);   \
ASSERT_TRIGGER_EXCEPTION();										\
(void)0

#define DISPLAY_ASSERT(STATEMENT)			\
DISPLAY_ASSERT_EXCEPTION(STATEMENT, true);	\
(void)0

#define ASSERT_EXCEPTION(STATEMENT, IS_EXCEPTION)               \
if (!(STATEMENT))                                               \
{																\
	DISPLAY_ASSERT_EXCEPTION(#STATEMENT, IS_EXCEPTION);			\
}                                                               \
(void)0

#define ASSERT(STATEMENT)			\
ASSERT_EXCEPTION(STATEMENT, true);	\
(void)0

#else
#define ASSERT_TRIGGER_EXCEPTION()							(void)(0)
#define DISPLAY_ASSERT_EXCEPTION(STATEMENT, IS_EXCEPTION)   (void)(#STATEMENT)
#define DISPLAY_ASSERT(STATEMENT)                           (void)(#STATEMENT)
#define ASSERT_EXCEPTION(STATEMENT, IS_EXCEPTION)           (void)(#STATEMENT)
#define ASSERT(STATEMENT)                                   (void)(#STATEMENT)


#endif // _DEBUG

extern bool g_catch_exceptions;

// TODO implement
void display_assert(char const* condition, char const* file, int32 line, bool assertion_failed);

void* csmemmove(void* destination, void* source, size_t size);

void* csmemset(void* destination, int32 val, size_t size);

void* csmemcpy(void* destination, const void* source, size_t size);

int32 vsprintf(char* buffer, size_t size, const char* format, va_list va_args);

/* 
* NOTES:
* size_t max_count was added in vista to this function call
*/
int32 vsnprintf(char* buffer, size_t size, size_t max_count, const char* format, va_list va_args);


const char* csprintf(char* buffer, size_t size, const char* format, ...);

/*
* NOTES:
* size_t max_count was added in vista to this function call
*/
const char* csnprintf(char* buffer, size_t size, size_t max_count, const char* format, ...);
