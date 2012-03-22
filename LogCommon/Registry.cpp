#include "pch.hpp"
#include <cassert>
#include <limits>
#include "Win32Exception.hpp"
#include "RuntimeDynamicLinker.hpp"
#include "Registry.hpp"

namespace Instalog { namespace SystemFacades {

	static NtOpenKeyFunc PNtOpenKey = GetNtDll().GetProcAddress<NtOpenKeyFunc>("NtOpenKey");
	static NtCreateKeyFunc PNtCreateKey = GetNtDll().GetProcAddress<NtCreateKeyFunc>("NtCreateKey");
	static NtCloseFunc PNtClose = GetNtDll().GetProcAddress<NtCloseFunc>("NtClose");
	static NtDeleteKeyFunc PNtDeleteKey = GetNtDll().GetProcAddress<NtCloseFunc>("NtDeleteKey");
	static NtQueryKeyFunc PNtQueryKey = GetNtDll().GetProcAddress<NtQueryKeyFunc>("NtQueryKey");
	static NtEnumerateKeyFunc PNtEnumerateKey = GetNtDll().GetProcAddress<NtEnumerateKeyFunc>("NtEnumerateKey");
	static NtEnumerateValueKeyFunc PNtEnumerateValueKeyFunc = GetNtDll().GetProcAddress<NtEnumerateValueKeyFunc>("NtEnumerateValueKey");
	static NtQueryValueKeyFunc PNtQueryValueKeyFunc = GetNtDll().GetProcAddress<NtQueryValueKeyFunc>("NtQueryValueKey");

	RegistryKey::~RegistryKey()
	{
		if (hKey_ != INVALID_HANDLE_VALUE)
		{
			PNtClose(hKey_);
		}
	}

	HANDLE RegistryKey::GetHkey() const
	{
		return hKey_;
	}

	RegistryKey::RegistryKey( HANDLE hKey )
	{
		hKey_ = hKey;
	}

	RegistryKey::RegistryKey( RegistryKey && other )
		: hKey_(other.hKey_)
	{
		other.hKey_ = INVALID_HANDLE_VALUE;
	}

	RegistryKey::RegistryKey()
		: hKey_(INVALID_HANDLE_VALUE)
	{ }

	RegistryValue RegistryKey::operator[]( std::wstring name )
	{
		return GetValue(std::move(name));
	}

	RegistryValue RegistryKey::GetValue( std::wstring name )
	{
		return RegistryValue(hKey_, std::move(name));
	}

	static RegistryKey RegistryKeyOpen( HANDLE hRoot, UNICODE_STRING& key, REGSAM samDesired )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = hRoot;
		attribs.ObjectName = &key;
		attribs.Attributes = OBJ_CASE_INSENSITIVE;
		attribs.SecurityDescriptor = NULL;
		attribs.SecurityQualityOfService = NULL;
		NTSTATUS errorCheck = PNtOpenKey(&hOpened, samDesired, &attribs);
		if (!NT_SUCCESS(errorCheck))
		{
			::SetLastError(errorCheck);
			hOpened = INVALID_HANDLE_VALUE;
		}
		return RegistryKey(hOpened);
	}

	static RegistryKey RegistryKeyOpen( HANDLE hRoot, std::wstring const& key, REGSAM samDesired )
	{
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		return RegistryKeyOpen(hRoot, ustrKey, samDesired);
	}

	RegistryKey RegistryKey::Open( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(0, key, samDesired);
	}

	RegistryKey RegistryKey::Open( RegistryKey const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent.GetHkey(), key, samDesired);
	}

	RegistryKey RegistryKey::Open( RegistryKey const& parent, UNICODE_STRING& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/ )
	{
		return RegistryKeyOpen(parent.GetHkey(), key, samDesired);
	}

	static RegistryKey RegistryKeyCreate( HANDLE hRoot, std::wstring const& key, REGSAM samDesired, DWORD options )
	{
		HANDLE hOpened;
		OBJECT_ATTRIBUTES attribs;
		UNICODE_STRING ustrKey = WstringToUnicodeString(key);
		attribs.Length = sizeof(attribs);
		attribs.RootDirectory = hRoot;
		attribs.ObjectName = &ustrKey;
		attribs.Attributes = OBJ_CASE_INSENSITIVE;
		attribs.SecurityDescriptor = NULL;
		attribs.SecurityQualityOfService = NULL;
		NTSTATUS errorCheck = PNtCreateKey(&hOpened, samDesired, &attribs, NULL, NULL, options, NULL);
		if (!NT_SUCCESS(errorCheck))
		{
			::SetLastError(errorCheck);
			hOpened = INVALID_HANDLE_VALUE;
		}
		return RegistryKey(hOpened);
	}

	RegistryKey RegistryKey::Create( std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(0, key, samDesired, options);
	}

	RegistryKey RegistryKey::Create( RegistryKey const& parent, std::wstring const& key, REGSAM samDesired /*= KEY_ALL_ACCESS*/, DWORD options /*= REG_OPTION_NON_VOLATILE */ )
	{
		return RegistryKeyCreate(parent.GetHkey(), key, samDesired, options);
	}

	void RegistryKey::Delete()
	{
		NTSTATUS errorCheck = PNtDeleteKey(GetHkey());
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
	}

	RegistryKeySizeInformation RegistryKey::GetSizeInformation() const
	{
		auto const buffSize = 32768ul;
		unsigned char buffer[buffSize];
		auto keyFullInformation = reinterpret_cast<KEY_FULL_INFORMATION const*>(&buffer[0]);
		auto bufferPtr = reinterpret_cast<void *>(&buffer[0]);
		ULONG resultLength = 0;
		NTSTATUS errorCheck = PNtQueryKey(GetHkey(), KeyFullInformation, bufferPtr, buffSize, &resultLength);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return RegistryKeySizeInformation(
			keyFullInformation->LastWriteTime.QuadPart,
			keyFullInformation->SubKeys,
			keyFullInformation->Values
			);
	}

	std::wstring RegistryKey::GetName() const
	{
		auto const buffSize = 32768ul;
		unsigned char buffer[buffSize];
		auto keyBasicInformation = reinterpret_cast<KEY_NAME_INFORMATION const*>(&buffer[0]);
		auto bufferPtr = reinterpret_cast<void *>(&buffer[0]);
		ULONG resultLength = 0;
		NTSTATUS errorCheck = PNtQueryKey(GetHkey(), KeyNameInformation, bufferPtr, buffSize, &resultLength);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		return std::wstring(keyBasicInformation->Name, keyBasicInformation->NameLength / sizeof(wchar_t));
	}

	std::vector<std::wstring> RegistryKey::EnumerateSubKeyNames() const
	{
		const auto bufferLength = 32768;
		std::vector<std::wstring> subkeys;
		NTSTATUS errorCheck;
		ULONG index = 0;
		ULONG resultLength = 0;
		unsigned char buff[bufferLength];
		auto basicInformation = reinterpret_cast<KEY_BASIC_INFORMATION const*>(buff);
		for(;;)
		{
			errorCheck = PNtEnumerateKey(
				GetHkey(),
				index++,
				KeyBasicInformation,
				buff,
				bufferLength,
				&resultLength
			);
			if (!NT_SUCCESS(errorCheck))
			{
				break;
			}
			subkeys.emplace_back(
				std::wstring(basicInformation->Name,
				             basicInformation->NameLength / sizeof(wchar_t)
							)
			);
		}
		if (errorCheck != STATUS_NO_MORE_ENTRIES)
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		std::sort(subkeys.begin(), subkeys.end());
		return std::move(subkeys);
	}

	RegistryKey& RegistryKey::operator=( RegistryKey other )
	{
		std::swap(hKey_, other.hKey_);
		return *this;
	}

	std::vector<RegistryKey> RegistryKey::EnumerateSubKeys(REGSAM samDesired /* = KEY_ALL_ACCESS */) const
	{
		std::vector<std::wstring> names(EnumerateSubKeyNames());
		std::vector<RegistryKey> result(names.size());
		std::transform(names.cbegin(), names.cend(), result.begin(), 
			[this, samDesired] (std::wstring const& name) -> RegistryKey {
			return Open(*this, name, samDesired);
		});
		return std::move(result);
	}

	bool RegistryKey::Valid() const
	{
		return hKey_ != INVALID_HANDLE_VALUE;
	}

	bool RegistryKey::Invalid() const
	{
		return !Valid();
	}

	std::vector<std::wstring> RegistryKey::EnumerateValueNames() const
	{
		std::vector<std::wstring> result;
		ULONG index = 0;
		const ULONG valueNameStructSize = 16384 * sizeof(wchar_t) +
			sizeof(KEY_VALUE_BASIC_INFORMATION);
		std::aligned_storage<valueNameStructSize,
			std::alignment_of<KEY_VALUE_BASIC_INFORMATION>::value>::type buff;
		auto basicValueInformation = reinterpret_cast<KEY_VALUE_BASIC_INFORMATION*>(&buff);
		for(;;)
		{
			ULONG resultLength;
			NTSTATUS errorCheck = PNtEnumerateValueKeyFunc(
				hKey_,
				index++,
				KeyValueBasicInformation,
				basicValueInformation,
				valueNameStructSize,
				&resultLength);
			if (NT_SUCCESS(errorCheck))
			{
				result.emplace_back(std::wstring(basicValueInformation->Name,
					basicValueInformation->NameLength / sizeof(wchar_t)));
			}
			else if (errorCheck == STATUS_NO_MORE_ENTRIES)
			{
				break;
			}
			else
			{
				Win32Exception::ThrowFromNtError(errorCheck);
			}
		}
		return result;
	}

	RegistryValue::RegistryValue( HANDLE hKey, std::wstring && name )
		: hKey_(hKey)
		, name_(std::move(name))
	{ }

	RegistryValue::RegistryValue( RegistryValue && other )
		: hKey_(other.hKey_)
		, name_(std::move(other.name_))
	{ }

	std::wstring const& RegistryValue::GetName() const
	{
		return name_;
	}

	RegistryData RegistryValue::GetData() const
	{
		UNICODE_STRING valueName(WstringToUnicodeString(GetName()));
		std::vector<unsigned char> buff(MAX_PATH);
		NTSTATUS errorCheck;
		do 
		{
			ULONG resultLength = 0;
			errorCheck = PNtQueryValueKeyFunc(
				hKey_,
				&valueName,
				KeyValuePartialInformation,
				buff.data(),
				static_cast<ULONG>(buff.size()),
				&resultLength
			);
			if ((errorCheck == STATUS_BUFFER_TOO_SMALL || errorCheck == STATUS_BUFFER_OVERFLOW) && resultLength != 0)
			{
				buff.resize(resultLength);
			}
		} while (errorCheck == STATUS_BUFFER_TOO_SMALL || errorCheck == STATUS_BUFFER_OVERFLOW);
		if (!NT_SUCCESS(errorCheck))
		{
			Win32Exception::ThrowFromNtError(errorCheck);
		}
		auto partialInfo = reinterpret_cast<KEY_VALUE_PARTIAL_INFORMATION const*>(buff.data());
		DWORD type = partialInfo->Type;
		ULONG len = partialInfo->DataLength;
		buff.erase(buff.begin(), buff.begin() + 3*sizeof(ULONG));
		buff.resize(len);
		return RegistryData(type, std::move(buff));
	}

	RegistryKeySizeInformation::RegistryKeySizeInformation( unsigned __int64 lastWriteTime, unsigned __int32 numberOfSubkeys, unsigned __int32 numberOfValues ) : lastWriteTime_(lastWriteTime)
		, numberOfSubkeys_(numberOfSubkeys)
		, numberOfValues_(numberOfValues)
	{ }

	unsigned __int32 RegistryKeySizeInformation::GetNumberOfSubkeys() const
	{
		return numberOfSubkeys_;
	}

	unsigned __int32 RegistryKeySizeInformation::GetNumberOfValues() const
	{
		return numberOfValues_;
	}

	unsigned __int64 RegistryKeySizeInformation::GetLastWriteTime() const
	{
		return lastWriteTime_;
	}


	RegistryValueAndData::RegistryValueAndData( std::wstring && name, RegistryData && data )
		: name_(name)
		, data_(data)
	{ }

	RegistryValueAndData::RegistryValueAndData( RegistryValueAndData && other )
		: name_(other.name_)
		, data_(other.data_)
	{ }

	std::wstring const& RegistryValueAndData::GetName() const
	{
		return name_;
	}

	RegistryData const& RegistryValueAndData::GetData() const
	{
		return data_;
	}


	RegistryData::RegistryData( DWORD type, std::vector<unsigned char> && data )
		: type_(type)
		, data_(data)
	{ }

	RegistryData::RegistryData( RegistryData && other )
		: type_(other.type_)
		, data_(std::move(other.data_))
	{ }

	DWORD RegistryData::GetType() const
	{
		return type_;
	}

	std::vector<unsigned char> const& RegistryData::GetContents() const
	{
		return data_;
	}

}}

