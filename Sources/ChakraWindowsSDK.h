#pragma once

#include <windows.h>

// The Chakra headers in the SDK do not expose some functions for store apps, even
// though they are documented as store compatible on MSDN. This header
// fudges #defines to get access to those functions and also adds ChakraCore-like helpers
#define USE_EDGEMODE_JSRT
#define CHAKRA_CALLBACK CALLBACK
#define CHAKRA_API STDAPI_(JsErrorCode)
typedef DWORD_PTR ChakraCookie;
typedef BYTE* ChakraBytePtr;
#include <jsrt.h>

#include <stdio.h>
#include <string>

#include "Utf8Helper.h"

#define PARAM_NOT_NULL(p) \
    if (p == nullptr) \
    { \
        return JsErrorNullArgument; \
    }

#define VALIDATE_JSREF(p) \
    if (p == JS_INVALID_REFERENCE) \
    { \
        return JsErrorInvalidArgument; \
    } \

CHAKRA_API
JsCreatePropertyId(
	_In_z_ const char *name,
	_In_ size_t length,
	_Out_ JsPropertyIdRef *propertyId) {

	utf8::NarrowToWide wName((LPCSTR)name);

	return JsGetPropertyIdFromName(wName, propertyId);
}

JsPropertyIdRef getId(const char* name) {
	JsPropertyIdRef id;
	JsErrorCode err = JsCreatePropertyId(name, strlen(name), &id);
	assert(err == JsNoError);
	return id;
}

CHAKRA_API
JsCreateString(
	_In_ const char *content,
	_In_ size_t length,
	_Out_ JsValueRef *value) {

	return JsCreateExternalArrayBuffer((void*)content, (unsigned int)length, nullptr, nullptr, value);
}

// TODO: add Utf16 support
CHAKRA_API
JsCreateStringUtf16(
	_In_ const uint16_t *content,
	_In_ size_t length,
	_Out_ JsValueRef *value) {
	return JsCreateExternalArrayBuffer((void*)content, (unsigned int)length, nullptr, nullptr, value);
}

template <class CopyFunc>
JsErrorCode WriteStringCopy(
	JsValueRef value,
	int start,
	int length,
	_Out_opt_ size_t* written,
	const CopyFunc& copyFunc)
{
	if (written)
	{
		*written = 0;  // init to 0 for default
	}

	const char16* str = nullptr;
	size_t strLength = 0;
	JsErrorCode errorCode = JsStringToPointer(value, &str, &strLength);
	if (errorCode != JsNoError)
	{
		return errorCode;
	}

	if (start < 0 || (size_t)start > strLength)
	{
		return JsErrorInvalidArgument;  // start out of range, no chars written
	}

	size_t count = min(static_cast<size_t>(length), strLength - start);
	if (count == 0)
	{
		return JsNoError;  // no chars written
	}

	errorCode = copyFunc(str + start, count, written);
	if (errorCode != JsNoError)
	{
		return errorCode;
	}

	if (written)
	{
		*written = count;
	}

	return JsNoError;
}

CHAKRA_API JsCopyStringUtf16(
	_In_ JsValueRef value,
	_In_ int start,
	_In_ int length,
	_Out_opt_ uint16_t* buffer,
	_Out_opt_ size_t* written)
{
	PARAM_NOT_NULL(value);
	VALIDATE_JSREF(value);

	return WriteStringCopy(value, start, length, written,
		[buffer](const char16* src, size_t count, size_t *needed)
	{
		if (buffer)
		{
			memmove(buffer, src, sizeof(char16) * count);
		}
		return JsNoError;
	});
}

CHAKRA_API JsCopyString(
	_In_ JsValueRef value,
	_Out_opt_ char* buffer,
	_In_ size_t bufferSize,
	_Out_opt_ size_t* length)
{
	PARAM_NOT_NULL(value);
	VALIDATE_JSREF(value);

	const char16* str = nullptr;
	size_t strLength = 0;
	JsErrorCode errorCode = JsStringToPointer(value, &str, &strLength);
	if (errorCode != JsNoError)
	{
		return errorCode;
	}

	utf8::WideToNarrow utf8Str(str, strLength, buffer, bufferSize);
	if (length)
	{
		*length = utf8Str.Length();
	}

	return JsNoError;
}

class JsRefReleaseAtScopeExit {
public:
	JsRefReleaseAtScopeExit(JsValueRef ref) : m_ref(ref) {}

	~JsRefReleaseAtScopeExit() { JsRelease(m_ref, nullptr); }

private:
	JsValueRef m_ref;
};

class JsCallAtScopeExit {
public:
	JsCallAtScopeExit(JsValueRef func) : m_function(func) {}

	void Revoke() { m_function = JS_INVALID_REFERENCE; }

	~JsCallAtScopeExit() { if (m_function != JS_INVALID_REFERENCE) JsCallFunction(m_function, &m_function, 1, nullptr); }

private:
	JsValueRef m_function;
};
