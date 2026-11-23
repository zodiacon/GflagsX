#include "pch.h"
#include "FlagStore.h"

#pragma comment(lib, "ntdll")

using namespace FlagStore;

extern "C" {
	NTSYSAPI LONG NTAPI NtQuerySystemInformation(ULONG infoClass, PVOID buffer, ULONG size, PULONG returned);
	NTSYSAPI LONG NTAPI NtSetSystemInformation(ULONG infoClass, PVOID buffer, ULONG size);
	NTSYSAPI ULONG NTAPI RtlNtStatusToDosError(LONG status);
}

namespace {
	constexpr ULONG SystemFlagsInformation = 9;
	// an x86 build on 64-bit Windows sees the native keys, as Windows reads them
	constexpr REGSAM View = KEY_WOW64_64KEY;

	std::wstring SubKey(std::wstring const& path, std::wstring const& name) {
		return path + L"\\" + name;
	}

	DWORD OpenKey(HKEY root, std::wstring const& path, REGSAM access, wil::unique_hkey& key) {
		return ::RegOpenKeyEx(root, path.c_str(), 0, access | View, key.addressof());
	}

	DWORD CreateKey(HKEY root, std::wstring const& path, wil::unique_hkey& key) {
		return ::RegCreateKeyEx(root, path.c_str(), 0, nullptr, 0, KEY_READ | KEY_WRITE | View, nullptr, key.addressof(), nullptr);
	}

	// a value's type and bytes; ERROR_FILE_NOT_FOUND if there's none
	DWORD QueryValue(HKEY key, PCWSTR name, DWORD& type, std::vector<BYTE>& data) {
		DWORD size = 0;
		auto error = ::RegQueryValueEx(key, name, nullptr, &type, nullptr, &size);
		if (error != ERROR_SUCCESS)
			return error;
		// room for a missing terminator of a string
		data.resize(size + sizeof(WCHAR) * 2);
		error = ::RegQueryValueEx(key, name, nullptr, &type, data.data(), &size);
		if (error == ERROR_SUCCESS)
			data.resize(size);
		return error;
	}

	std::wstring StringFrom(std::vector<BYTE> const& data) {
		std::wstring text(reinterpret_cast<PCWSTR>(data.data()), data.size() / sizeof(WCHAR));
		while (!text.empty() && text.back() == 0)
			text.pop_back();
		return text;
	}

	// a REG_SZ number as GFlags writes it ("0x02000000"), or decimal
	uint32_t ParseNumber(std::wstring const& text) {
		return (uint32_t)wcstoul(text.c_str(), nullptr, 0);
	}

	// a DWORD (or a string with a number) value; 0 and REG_NONE if there's none
	DWORD ReadFlagsValue(HKEY key, PCWSTR name, uint32_t& value, DWORD& type) {
		std::vector<BYTE> data;
		value = 0;
		auto error = QueryValue(key, name, type, data);
		if (error == ERROR_FILE_NOT_FOUND) {
			type = REG_NONE;
			return ERROR_SUCCESS;
		}
		if (error != ERROR_SUCCESS)
			return error;
		if ((type == REG_DWORD || type == REG_BINARY) && data.size() >= sizeof(DWORD))
			value = *reinterpret_cast<DWORD const*>(data.data());
		else if (type == REG_QWORD && data.size() >= sizeof(uint64_t))
			value = (uint32_t)*reinterpret_cast<uint64_t const*>(data.data());
		else if (type == REG_SZ || type == REG_EXPAND_SZ)
			value = ParseNumber(StringFrom(data));
		return ERROR_SUCCESS;
	}

	DWORD SetDword(HKEY key, PCWSTR name, DWORD value) {
		return ::RegSetValueEx(key, name, 0, REG_DWORD, reinterpret_cast<BYTE const*>(&value), sizeof(value));
	}

	DWORD SetString(HKEY key, PCWSTR name, std::wstring const& value) {
		return ::RegSetValueEx(key, name, 0, REG_SZ, reinterpret_cast<BYTE const*>(value.c_str()), DWORD((value.size() + 1) * sizeof(WCHAR)));
	}

	DWORD DeleteValue(HKEY key, PCWSTR name) {
		auto error = ::RegDeleteValue(key, name);
		return error == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : error;
	}

	std::wstring ReadString(HKEY key, PCWSTR name) {
		DWORD type;
		std::vector<BYTE> data;
		if (QueryValue(key, name, type, data) != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
			return {};
		return StringFrom(data);
	}

	DWORD ReadDword(HKEY key, PCWSTR name, DWORD defaultValue) {
		uint32_t value;
		DWORD type;
		if (ReadFlagsValue(key, name, value, type) != ERROR_SUCCESS || type == REG_NONE)
			return defaultValue;
		return value;
	}

	DWORD EnableDebugPrivilege() {
		wil::unique_handle token;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, token.addressof()))
			return ::GetLastError();
		TOKEN_PRIVILEGES tp{ 1 };
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!::LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
			return ::GetLastError();
		// succeeds without the privilege too, saying so with ERROR_NOT_ALL_ASSIGNED
		::AdjustTokenPrivileges(token.get(), FALSE, &tp, sizeof(tp), nullptr, nullptr);
		return ::GetLastError();
	}
}

Locations const& Locations::System() {
	static const Locations system{
		HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SilentProcessExit",
	};
	return system;
}

Locations Locations::Under(HKEY root, std::wstring const& path) {
	auto& system = System();
	return { root, SubKey(path, system.SessionManager), SubKey(path, system.ImageOptions), SubKey(path, system.SilentProcessExit) };
}

std::expected<uint32_t, DWORD> FlagStore::ReadRegistryFlags(Locations const& where) {
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, where.SessionManager, KEY_READ, key);
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
	uint32_t flags;
	DWORD type;
	error = ReadFlagsValue(key.get(), L"GlobalFlag", flags, type);
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
	return flags;
}

DWORD FlagStore::WriteRegistryFlags(Locations const& where, uint32_t flags) {
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, where.SessionManager, KEY_SET_VALUE, key);
	if (error != ERROR_SUCCESS)
		return error;
	return SetDword(key.get(), L"GlobalFlag", flags);
}

std::expected<uint32_t, DWORD> FlagStore::ReadKernelFlags() {
	ULONG flags = 0;
	auto status = NtQuerySystemInformation(SystemFlagsInformation, &flags, sizeof(flags), nullptr);
	if (status < 0)
		return std::unexpected(RtlNtStatusToDosError(status));
	return flags;
}

DWORD FlagStore::WriteKernelFlags(uint32_t flags) {
	EnableDebugPrivilege();
	ULONG value = flags;
	auto status = NtSetSystemInformation(SystemFlagsInformation, &value, sizeof(value));
	return status < 0 ? RtlNtStatusToDosError(status) : ERROR_SUCCESS;
}

std::expected<std::vector<std::wstring>, DWORD> FlagStore::GetImages(Locations const& where) {
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, where.ImageOptions, KEY_READ, key);
	if (error == ERROR_FILE_NOT_FOUND)
		return std::vector<std::wstring>{};
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
	std::vector<std::wstring> images;
	WCHAR name[256];
	for (DWORD i = 0; ; i++) {
		DWORD len = _countof(name);
		error = ::RegEnumKeyEx(key.get(), i, name, &len, nullptr, nullptr, nullptr, nullptr);
		if (error == ERROR_NO_MORE_ITEMS)
			break;
		if (error != ERROR_SUCCESS)
			return std::unexpected(error);
		images.emplace_back(name, len);
	}
	std::ranges::sort(images, [](auto& a, auto& b) { return _wcsicmp(a.c_str(), b.c_str()) < 0; });
	return images;
}

std::expected<ImageSettings, DWORD> FlagStore::ReadImage(Locations const& where, std::wstring const& image) {
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, SubKey(where.ImageOptions, image), KEY_READ, key);
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);

	ImageSettings settings;
	error = ReadFlagsValue(key.get(), L"GlobalFlag", settings.GlobalFlag, settings.GlobalFlagType);
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
	settings.Debugger = ReadString(key.get(), L"Debugger");

	DWORD type;
	std::vector<BYTE> data;
	if (QueryValue(key.get(), L"MitigationOptions", type, data) == ERROR_SUCCESS) {
		settings.MitigationsType = type;
		// a REG_BINARY of 8 bytes or more (as Set-ProcessMitigation writes it), a REG_QWORD, a REG_DWORD
		data.resize(std::max(data.size(), sizeof(uint64_t)));
		settings.Mitigations = *reinterpret_cast<uint64_t const*>(data.data());
		if (type == REG_DWORD)
			settings.Mitigations &= 0xffffffff;
		if (type == REG_BINARY)
			settings.MitigationsMore.assign(data.begin() + sizeof(uint64_t), data.end());
	}
	return settings;
}

DWORD FlagStore::WriteImage(Locations const& where, std::wstring const& image, ImageSettings const& settings) {
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, SubKey(where.ImageOptions, image), KEY_READ | KEY_SET_VALUE, key);
	if (error != ERROR_SUCCESS)
		return error;

	if (settings.GlobalFlag == 0)
		error = DeleteValue(key.get(), L"GlobalFlag");
	else if (settings.GlobalFlagType == REG_SZ || settings.GlobalFlagType == REG_EXPAND_SZ)
		error = SetString(key.get(), L"GlobalFlag", std::format(L"0x{:08x}", settings.GlobalFlag));
	else
		error = SetDword(key.get(), L"GlobalFlag", settings.GlobalFlag);
	if (error != ERROR_SUCCESS)
		return error;

	error = settings.Debugger.empty() ? DeleteValue(key.get(), L"Debugger") : SetString(key.get(), L"Debugger", settings.Debugger);
	if (error != ERROR_SUCCESS)
		return error;

	bool more = std::ranges::any_of(settings.MitigationsMore, [](auto b) { return b != 0; });
	if (settings.Mitigations == 0 && !more)
		return DeleteValue(key.get(), L"MitigationOptions");
	if (settings.MitigationsType == REG_BINARY) {
		std::vector<BYTE> data(sizeof(uint64_t));
		memcpy(data.data(), &settings.Mitigations, sizeof(uint64_t));
		data.insert(data.end(), settings.MitigationsMore.begin(), settings.MitigationsMore.end());
		return ::RegSetValueEx(key.get(), L"MitigationOptions", 0, REG_BINARY, data.data(), (DWORD)data.size());
	}
	return ::RegSetValueEx(key.get(), L"MitigationOptions", 0, REG_QWORD, reinterpret_cast<BYTE const*>(&settings.Mitigations), sizeof(uint64_t));
}

DWORD FlagStore::CreateImage(Locations const& where, std::wstring const& image) {
	if (!IsValidImageName(image))
		return ERROR_INVALID_NAME;
	wil::unique_hkey key;
	DWORD disposition = 0;
	auto error = ::RegCreateKeyEx(where.Root, SubKey(where.ImageOptions, image).c_str(), 0, nullptr, 0, KEY_READ | View, nullptr,
		key.addressof(), &disposition);
	if (error != ERROR_SUCCESS)
		return error;
	return disposition == REG_OPENED_EXISTING_KEY ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS;
}

DWORD FlagStore::DeleteImage(Locations const& where, std::wstring const& image) {
	if (!IsValidImageName(image))
		return ERROR_INVALID_NAME;
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, where.ImageOptions, KEY_READ | KEY_WRITE | DELETE, key);
	if (error != ERROR_SUCCESS)
		return error;
	error = ::RegDeleteTree(key.get(), image.c_str());
	if (error != ERROR_SUCCESS)
		return error;
	// RegDeleteTree leaves the key itself
	error = ::RegDeleteKeyEx(key.get(), image.c_str(), View, 0);
	if (error != ERROR_SUCCESS && error != ERROR_FILE_NOT_FOUND)
		return error;
	return DeleteSilentProcessExit(where, image);
}

bool FlagStore::IsValidImageName(std::wstring const& name) {
	return !name.empty() && name.find(L'\\') == std::wstring::npos && name.size() < 256;
}

std::expected<SilentProcessExitSettings, DWORD> FlagStore::ReadSilentProcessExit(Locations const& where, std::wstring const& image) {
	SilentProcessExitSettings settings;
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, SubKey(where.SilentProcessExit, image), KEY_READ, key);
	if (error == ERROR_FILE_NOT_FOUND)
		return settings;
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);

	settings.Exists = true;
	settings.IgnoreSelfExits = ReadDword(key.get(), L"IgnoreSelfExits", 0) != 0;
	settings.ReportingMode = ReadDword(key.get(), L"ReportingMode", 0);
	settings.MonitorProcess = ReadString(key.get(), L"MonitorProcess");
	settings.LocalDumpFolder = ReadString(key.get(), L"LocalDumpFolder");
	settings.DumpType = DumpType(std::min<DWORD>(ReadDword(key.get(), L"DumpType", DWORD(DumpType::Mini)), DWORD(DumpType::Custom)));

	DWORD type;
	std::vector<BYTE> data;
	if (QueryValue(key.get(), L"ModuleIgnoreList", type, data) == ERROR_SUCCESS && type == REG_MULTI_SZ) {
		auto text = reinterpret_cast<PCWSTR>(data.data());
		auto end = text + data.size() / sizeof(WCHAR);
		while (text < end && *text) {
			auto len = wcsnlen(text, end - text);
			settings.ModuleIgnoreList.emplace_back(text, len);
			text += len + 1;
		}
	}
	return settings;
}

DWORD FlagStore::WriteSilentProcessExit(Locations const& where, std::wstring const& image, SilentProcessExitSettings const& settings) {
	if (!IsValidImageName(image))
		return ERROR_INVALID_NAME;
	wil::unique_hkey key;
	auto error = CreateKey(where.Root, SubKey(where.SilentProcessExit, image), key);
	if (error != ERROR_SUCCESS)
		return error;
	auto string = [&](PCWSTR name, std::wstring const& value) {
		return value.empty() ? DeleteValue(key.get(), name) : SetString(key.get(), name, value);
	};

	for (auto e : {
		settings.IgnoreSelfExits ? SetDword(key.get(), L"IgnoreSelfExits", 1) : DeleteValue(key.get(), L"IgnoreSelfExits"),
		settings.ReportingMode ? SetDword(key.get(), L"ReportingMode", settings.ReportingMode) : DeleteValue(key.get(), L"ReportingMode"),
		string(L"MonitorProcess", settings.MonitorProcess),
		string(L"LocalDumpFolder", settings.LocalDumpFolder),
		SetDword(key.get(), L"DumpType", DWORD(settings.DumpType)) })
		if (e != ERROR_SUCCESS)
			return e;

	if (settings.ModuleIgnoreList.empty())
		return DeleteValue(key.get(), L"ModuleIgnoreList");
	std::wstring multi;
	for (auto& module : settings.ModuleIgnoreList)
		if (!module.empty())
			multi += module + L'\0';
	multi += L'\0';
	return ::RegSetValueEx(key.get(), L"ModuleIgnoreList", 0, REG_MULTI_SZ, reinterpret_cast<BYTE const*>(multi.data()), DWORD(multi.size() * sizeof(WCHAR)));
}

DWORD FlagStore::DeleteSilentProcessExit(Locations const& where, std::wstring const& image) {
	if (!IsValidImageName(image))
		return ERROR_INVALID_NAME;
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, where.SilentProcessExit, KEY_READ | KEY_WRITE | DELETE, key);
	if (error == ERROR_FILE_NOT_FOUND)
		return ERROR_SUCCESS;
	if (error != ERROR_SUCCESS)
		return error;
	error = ::RegDeleteTree(key.get(), image.c_str());
	if (error == ERROR_FILE_NOT_FOUND)
		return ERROR_SUCCESS;
	if (error != ERROR_SUCCESS)
		return error;
	error = ::RegDeleteKeyEx(key.get(), image.c_str(), View, 0);
	return error == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : error;
}
