#include "pch.h"
#include "FlagStore.h"
#include "Helpers.h"

#pragma comment(lib, "ntdll")

using namespace FlagStore;

extern "C" {
	NTSYSAPI LONG NTAPI NtQuerySystemInformation(ULONG infoClass, PVOID buffer, ULONG size, PULONG returned);
	NTSYSAPI LONG NTAPI NtSetSystemInformation(ULONG infoClass, PVOID buffer, ULONG size);
	NTSYSAPI ULONG NTAPI RtlNtStatusToDosError(LONG status);
}

namespace {
	constexpr ULONG SystemFlagsInformation = 9;
	constexpr ULONG SystemRefTraceInformation = 86;
	constexpr ULONG SystemSpecialPoolInformation = 87;
	constexpr LONG StatusInfoLengthMismatch = 0xC0000004L;
	constexpr LONG StatusBufferTooSmall = 0xC0000023L;

	// SYSTEM_SPECIAL_POOL_INFORMATION
	struct SpecialPoolInformation {
		ULONG PoolTag;
		ULONG Flags;		// 1: overruns (verify the end)
	};

	// UNICODE_STRING (winternl.h's declarations of the Nt functions differ from these)
	struct UnicodeString {
		USHORT Length;
		USHORT MaximumLength;
		PWSTR Buffer;
	};

	// SYSTEM_REF_TRACE_INFORMATION
	struct RefTraceInformation {
		BOOLEAN TraceEnable;
		BOOLEAN TracePermanent;
		UnicodeString TraceProcessName;
		UnicodeString TracePoolTags;
	};
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

	// 0: no value; otherwise a REG_DWORD
	DWORD WriteDword(HKEY key, PCWSTR name, DWORD value) {
		return value ? SetDword(key, name, value) : DeleteValue(key, name);
	}

	// 0: no value; a string ("0x..." in the format given) where it was one, or where there was none and the string is
	// the default; a REG_DWORD otherwise
	DWORD WriteFlags(HKEY key, PCWSTR name, uint32_t value, DWORD type, bool stringByDefault, std::wstring const& text) {
		if (value == 0)
			return DeleteValue(key, name);
		bool string = type == REG_SZ || type == REG_EXPAND_SZ || (type == REG_NONE && stringByDefault);
		return string ? SetString(key, name, text) : SetDword(key, name, value);
	}

	// separated by spaces; none: no value
	DWORD WriteWords(HKEY key, PCWSTR name, std::vector<std::wstring> const& words) {
		auto text = Helpers::JoinWords(words);
		return text.empty() ? DeleteValue(key, name) : SetString(key, name, text);
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

namespace {
	std::wstring MemoryManagement(Locations const& where) {
		return SubKey(where.SessionManager, L"Memory Management");
	}

	std::wstring KernelKey(Locations const& where) {
		return SubKey(where.SessionManager, L"Kernel");
	}

	// "Ntfs;Fat " (ObTracePoolTags) as tags
	std::vector<std::wstring> SplitTags(std::wstring const& text) {
		std::vector<std::wstring> tags;
		size_t start = 0;
		while (start <= text.size()) {
			auto end = text.find(L';', start);
			if (end == std::wstring::npos)
				end = text.size();
			if (end > start)
				tags.push_back(text.substr(start, end - start));
			start = end + 1;
		}
		return tags;
	}

	std::wstring JoinTags(std::vector<std::wstring> const& tags) {
		std::wstring text;
		for (auto& tag : tags)
			if (!tag.empty())
				text += (text.empty() ? L"" : L";") + tag;
		return text;
	}

	UnicodeString StringOf(std::wstring const& text) {
		UnicodeString string{};
		string.Length = string.MaximumLength = USHORT(text.size() * sizeof(WCHAR));
		string.Buffer = const_cast<PWSTR>(text.c_str());
		return string;
	}

	// a string the kernel returned, inside the buffer it filled (anything else: empty)
	std::wstring StringFrom(UnicodeString const& string, std::vector<BYTE> const& buffer) {
		auto start = reinterpret_cast<BYTE const*>(string.Buffer);
		if (!start || string.Length == 0 || start < buffer.data() || start + string.Length > buffer.data() + buffer.size())
			return {};
		return std::wstring(string.Buffer, string.Length / sizeof(WCHAR));
	}
}

std::expected<SpecialPoolSettings, DWORD> FlagStore::ReadSpecialPool(Locations const& where) {
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, MemoryManagement(where), KEY_READ, key);
	if (error == ERROR_FILE_NOT_FOUND)
		return SpecialPoolSettings{};
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
	SpecialPoolSettings settings;
	settings.Tag = ReadDword(key.get(), L"PoolTag", 0);
	settings.VerifyEnd = ReadDword(key.get(), L"PoolTagOverruns", 1) != 0;
	return settings;
}

DWORD FlagStore::WriteSpecialPool(Locations const& where, SpecialPoolSettings const& settings) {
	wil::unique_hkey key;
	auto error = CreateKey(where.Root, MemoryManagement(where), key);
	if (error != ERROR_SUCCESS)
		return error;
	if (settings.Tag == 0) {
		error = DeleteValue(key.get(), L"PoolTag");
		return error != ERROR_SUCCESS ? error : DeleteValue(key.get(), L"PoolTagOverruns");
	}
	error = SetDword(key.get(), L"PoolTag", settings.Tag);
	return error != ERROR_SUCCESS ? error : SetDword(key.get(), L"PoolTagOverruns", settings.VerifyEnd ? 1 : 0);
}

std::expected<SpecialPoolSettings, DWORD> FlagStore::ReadKernelSpecialPool() {
	SpecialPoolInformation info{};
	auto status = NtQuerySystemInformation(SystemSpecialPoolInformation, &info, sizeof(info), nullptr);
	if (status < 0)
		return std::unexpected(RtlNtStatusToDosError(status));
	return SpecialPoolSettings{ info.PoolTag, info.Flags == 1 };
}

DWORD FlagStore::WriteKernelSpecialPool(SpecialPoolSettings const& settings) {
	EnableDebugPrivilege();
	SpecialPoolInformation info{ settings.Tag, settings.VerifyEnd ? 1ul : 0ul };
	auto status = NtSetSystemInformation(SystemSpecialPoolInformation, &info, sizeof(info));
	return status < 0 ? RtlNtStatusToDosError(status) : ERROR_SUCCESS;
}

std::wstring FlagStore::FormatPoolTag(DWORD tag) {
	std::wstring text;
	for (int i = 0; i < 4; i++) {
		auto c = wchar_t((tag >> (i * 8)) & 0xff);
		if (c < 0x20 || c > 0x7e)
			return std::format(L"0x{:X}", tag);
		text += c;
	}
	while (!text.empty() && text.back() == L' ')
		text.pop_back();
	// all spaces: not a tag anyone types
	return text.empty() ? std::format(L"0x{:X}", tag) : text;
}

std::optional<DWORD> FlagStore::ParsePoolTag(std::wstring_view text) {
	if (text.size() > 2 && text[0] == L'0' && (text[1] == L'x' || text[1] == L'X')) {
		auto value = Helpers::ParseNumber(text);
		return value && *value ? value : std::nullopt;
	}
	if (text.empty() || text.size() > 4)
		return std::nullopt;
	DWORD tag = 0;
	for (int i = 0; i < 4; i++) {
		auto c = i < (int)text.size() ? text[i] : L' ';
		if (c < 0x20 || c > 0x7e)
			return std::nullopt;
		tag |= DWORD(c) << (i * 8);
	}
	return tag;
}

std::expected<ObjectTraceSettings, DWORD> FlagStore::ReadObjectTrace(Locations const& where) {
	wil::unique_hkey key;
	auto error = OpenKey(where.Root, KernelKey(where), KEY_READ, key);
	if (error == ERROR_FILE_NOT_FOUND)
		return ObjectTraceSettings{};
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
	ObjectTraceSettings settings;
	settings.PoolTags = SplitTags(ReadString(key.get(), L"ObTracePoolTags"));
	settings.ProcessName = ReadString(key.get(), L"ObTraceProcessName");
	settings.Permanent = ReadDword(key.get(), L"ObTracePermanent", 0) != 0;
	settings.Enabled = !settings.PoolTags.empty() || !settings.ProcessName.empty();
	return settings;
}

DWORD FlagStore::WriteObjectTrace(Locations const& where, ObjectTraceSettings const& settings) {
	wil::unique_hkey key;
	auto error = CreateKey(where.Root, KernelKey(where), key);
	if (error != ERROR_SUCCESS)
		return error;
	auto tags = settings.Enabled ? JoinTags(settings.PoolTags) : L"";
	auto process = settings.Enabled ? settings.ProcessName : L"";
	for (auto e : {
		tags.empty() ? DeleteValue(key.get(), L"ObTracePoolTags") : SetString(key.get(), L"ObTracePoolTags", tags),
		process.empty() ? DeleteValue(key.get(), L"ObTraceProcessName") : SetString(key.get(), L"ObTraceProcessName", process),
		settings.Enabled ? SetDword(key.get(), L"ObTracePermanent", settings.Permanent ? 1 : 0) : DeleteValue(key.get(), L"ObTracePermanent") })
		if (e != ERROR_SUCCESS)
			return e;
	return ERROR_SUCCESS;
}

std::expected<ObjectTraceSettings, DWORD> FlagStore::ReadKernelObjectTrace() {
	EnableDebugPrivilege();
	std::vector<BYTE> buffer(sizeof(RefTraceInformation) + 1024);
	LONG status;
	for (int tries = 0; tries < 4; tries++) {
		ULONG needed = 0;
		status = NtQuerySystemInformation(SystemRefTraceInformation, buffer.data(), (ULONG)buffer.size(), &needed);
		if (status != StatusInfoLengthMismatch && status != StatusBufferTooSmall)
			break;
		buffer.resize(std::max<size_t>(needed, buffer.size() * 2));
	}
	if (status < 0)
		return std::unexpected(RtlNtStatusToDosError(status));
	auto info = reinterpret_cast<RefTraceInformation const*>(buffer.data());
	ObjectTraceSettings settings;
	settings.Enabled = info->TraceEnable != 0;
	settings.Permanent = info->TracePermanent != 0;
	settings.ProcessName = StringFrom(info->TraceProcessName, buffer);
	settings.PoolTags = SplitTags(StringFrom(info->TracePoolTags, buffer));
	return settings;
}

DWORD FlagStore::WriteKernelObjectTrace(ObjectTraceSettings const& settings) {
	EnableDebugPrivilege();
	auto tags = JoinTags(settings.PoolTags);
	RefTraceInformation info{};
	info.TraceEnable = settings.Enabled;
	info.TracePermanent = settings.Permanent;
	info.TraceProcessName = StringOf(settings.ProcessName);
	info.TracePoolTags = StringOf(tags);
	auto status = NtSetSystemInformation(SystemRefTraceInformation, &info, sizeof(info));
	return status < 0 ? RtlNtStatusToDosError(status) : ERROR_SUCCESS;
}

std::vector<std::wstring> FlagStore::GetVerifierProviders(Locations const& where) {
	wil::unique_hkey key;
	if (OpenKey(where.Root, SubKey(where.ImageOptions, VerifierGlobalSettings), KEY_READ, key) != ERROR_SUCCESS)
		return {};
	return Helpers::SplitWords(ReadString(key.get(), L"VerifierProviders"));
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
		// Application Verifier's settings ({ApplicationVerifierGlobalSettings}) and the like: not images
		if (len > 0 && name[0] == L'{')
			continue;
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
		// a REG_BINARY (24 bytes as Set-ProcessMitigation writes it, maybe more or fewer), a REG_QWORD (the first
		// word), a REG_DWORD (its low half)
		if (type == REG_DWORD)
			data.resize(std::min(data.size(), sizeof(DWORD)));
		else if (type != REG_BINARY)
			data.resize(std::min(data.size(), sizeof(uint64_t)));
		constexpr size_t Size = sizeof(settings.Mitigations.Word);
		if (data.size() > Size)
			settings.MitigationsMore.assign(data.begin() + Size, data.end());
		data.resize(Size);
		memcpy(settings.Mitigations.Word.data(), data.data(), Size);
	}

	auto& heap = settings.PageHeap;
	error = ReadFlagsValue(key.get(), L"PageHeapFlags", heap.Flags, heap.FlagsType);
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
	heap.SizeRangeStart = ReadDword(key.get(), L"PageHeapSizeRangeStart", 0);
	heap.SizeRangeEnd = ReadDword(key.get(), L"PageHeapSizeRangeEnd", 0);
	heap.RandomProbability = ReadDword(key.get(), L"PageHeapRandomProbability", 0);
	heap.FaultProbability = ReadDword(key.get(), L"PageHeapFaultProbability", 0);
	heap.FaultTimeOut = ReadDword(key.get(), L"PageHeapFaultTimeOut", 0);
	heap.TargetDlls = Helpers::SplitWords(ReadString(key.get(), L"PageHeapTargetDlls"));

	auto& verifier = settings.Verifier;
	verifier.Dlls = Helpers::SplitWords(ReadString(key.get(), L"VerifierDlls"));
	error = ReadFlagsValue(key.get(), L"VerifierFlags", verifier.Flags, verifier.FlagsType);
	if (error != ERROR_SUCCESS)
		return std::unexpected(error);
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

	auto& heap = settings.PageHeap;
	auto& verifier = settings.Verifier;
	for (auto e : {
		// GFlags writes PageHeapFlags as a string
		WriteFlags(key.get(), L"PageHeapFlags", heap.Flags, heap.FlagsType, true, std::format(L"0x{:x}", heap.Flags)),
		WriteDword(key.get(), L"PageHeapSizeRangeStart", heap.SizeRangeStart),
		WriteDword(key.get(), L"PageHeapSizeRangeEnd", heap.SizeRangeEnd),
		WriteDword(key.get(), L"PageHeapRandomProbability", heap.RandomProbability),
		WriteDword(key.get(), L"PageHeapFaultProbability", heap.FaultProbability),
		WriteDword(key.get(), L"PageHeapFaultTimeOut", heap.FaultTimeOut),
		WriteWords(key.get(), L"PageHeapTargetDlls", heap.TargetDlls),
		WriteFlags(key.get(), L"VerifierFlags", verifier.Flags, verifier.FlagsType, false, std::format(L"0x{:x}", verifier.Flags)),
		WriteWords(key.get(), L"VerifierDlls", verifier.Dlls) })
		if (e != ERROR_SUCCESS)
			return e;

	bool more = std::ranges::any_of(settings.MitigationsMore, [](auto b) { return b != 0; });
	auto& options = settings.Mitigations;
	if (options.IsZero() && !more)
		return DeleteValue(key.get(), L"MitigationOptions");
	if (settings.MitigationsType == REG_BINARY || options.HasHighWords() || more) {
		std::vector<BYTE> data(sizeof(options.Word));
		memcpy(data.data(), options.Word.data(), data.size());
		data.insert(data.end(), settings.MitigationsMore.begin(), settings.MitigationsMore.end());
		return ::RegSetValueEx(key.get(), L"MitigationOptions", 0, REG_BINARY, data.data(), (DWORD)data.size());
	}
	return ::RegSetValueEx(key.get(), L"MitigationOptions", 0, REG_QWORD, reinterpret_cast<BYTE const*>(&options.Word[0]), sizeof(uint64_t));
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

FlagStore::DumpPreset FlagStore::PresetOf(DWORD dumpType) {
	switch (dumpType) {
		case MicroDump: return DumpPreset::Micro;
		case MiniDump: return DumpPreset::Mini;
		case HeapDump: return DumpPreset::Heap;
	}
	return DumpPreset::Custom;
}

std::optional<DWORD> FlagStore::PresetValue(DumpPreset preset) {
	switch (preset) {
		case DumpPreset::Micro: return MicroDump;
		case DumpPreset::Mini: return MiniDump;
		case DumpPreset::Heap: return HeapDump;
	}
	return std::nullopt;
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
	settings.DumpType = ReadDword(key.get(), L"DumpType", MiniDump);
	settings.MaxNumberOfDumpFiles = ReadDword(key.get(), L"MaxNumberOfDumpFiles", 0);

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
		SetDword(key.get(), L"DumpType", settings.DumpType),
		settings.MaxNumberOfDumpFiles ? SetDword(key.get(), L"MaxNumberOfDumpFiles", settings.MaxNumberOfDumpFiles) : DeleteValue(key.get(), L"MaxNumberOfDumpFiles") })
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
