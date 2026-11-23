#pragma once

#include "Mitigations.h"

//
// Where the flags live, read and written: the system's flags in the registry (read at boot), the running kernel's
// (ntdll), and the images' (Image File Execution Options: GlobalFlag, Debugger, MitigationOptions) with their Silent
// Process Exit settings. The registry functions take the keys' Locations: the system's (HKLM), or a key of the tests'.
// Errors are Win32 codes (std::expected<T, DWORD> or DWORD).
//
namespace FlagStore {
	struct Locations {
		HKEY Root;
		std::wstring SessionManager;		// the system's GlobalFlag value
		std::wstring ImageOptions;			// a subkey per image
		std::wstring SilentProcessExit;		// a subkey per image

		// under HKLM, where Windows reads them
		static Locations const& System();
		// the same paths under another root key and path (the tests': HKCU\Software\GflagsXTests)
		static Locations Under(HKEY root, std::wstring const& path);
	};

	// the system's flags in the registry, for the next boot (no value: 0)
	std::expected<uint32_t, DWORD> ReadRegistryFlags(Locations const& where);
	DWORD WriteRegistryFlags(Locations const& where, uint32_t flags);

	// the running system's flags (NtQuerySystemInformation: the kernel's, not this process's own), and setting them
	// (needs the debug privilege, which an administrator has: it's enabled here)
	std::expected<uint32_t, DWORD> ReadKernelFlags();
	DWORD WriteKernelFlags(uint32_t flags);

	struct ImageSettings {
		uint32_t GlobalFlag{ 0 };
		// as stored, kept when written: REG_DWORD, or REG_SZ ("0x02000000", as GFlags writes it); REG_NONE: no value
		DWORD GlobalFlagType{ REG_NONE };
		std::wstring Debugger;
		Mitigations::Options Mitigations;
		// a REG_BINARY MitigationOptions' bytes past the first 16 (mitigations newer than Policy2, which aren't shown),
		// kept
		std::vector<BYTE> MitigationsMore;
		// REG_QWORD or REG_BINARY (or REG_DWORD); REG_NONE: no value. Written as REG_BINARY when it was, or when Policy2
		// isn't 0 (a REG_QWORD has Policy only)
		DWORD MitigationsType{ REG_NONE };

		bool operator==(ImageSettings const&) const = default;
	};

	// the images that have a key (sorted, without case)
	std::expected<std::vector<std::wstring>, DWORD> GetImages(Locations const& where);
	std::expected<ImageSettings, DWORD> ReadImage(Locations const& where, std::wstring const& image);
	// a 0 GlobalFlag, an empty Debugger and 0 mitigations (with no more bytes set) remove their values
	DWORD WriteImage(Locations const& where, std::wstring const& image, ImageSettings const& settings);
	// ERROR_ALREADY_EXISTS if the image has a key already
	DWORD CreateImage(Locations const& where, std::wstring const& image);
	// the key with its subkeys, and the image's Silent Process Exit key
	DWORD DeleteImage(Locations const& where, std::wstring const& image);
	// a key name: not empty, no backslash
	bool IsValidImageName(std::wstring const& name);

	// the Silent Process Exit key's ReportingMode bits
	constexpr DWORD LaunchMonitorProcess = 1;
	constexpr DWORD LocalDump = 2;
	constexpr DWORD Notification = 4;

	// DumpType is MINIDUMP_TYPE flags (dbghelp.h). GFlags' dump types are these, any other value is its Custom: Micro is
	// documented (MiniDumpFilterMemory | MiniDumpFilterModulePaths), Mini and Heap are gflagsui.dll's constants.
	enum class DumpPreset {
		Micro, Mini, Heap, Custom,
	};
	constexpr DWORD MicroDump = 0x88;
	constexpr DWORD MiniDump = 0x121;		// data segments, unloaded modules, process and thread data
	constexpr DWORD HeapDump = 0x10B25;		// Mini's, handle data, private read/write memory, memory info, tokens
	// the preset a DumpType is (Custom: none of them)
	DumpPreset PresetOf(DWORD dumpType);
	// a preset's DumpType (Custom: nullopt, the value is the user's)
	std::optional<DWORD> PresetValue(DumpPreset preset);

	struct SilentProcessExitSettings {
		bool Exists{ false };	// read: the image has a key
		bool IgnoreSelfExits{ false };
		DWORD ReportingMode{ 0 };
		std::wstring MonitorProcess;
		std::wstring LocalDumpFolder;
		DWORD DumpType{ MiniDump };			// MINIDUMP_TYPE flags
		DWORD MaxNumberOfDumpFiles{ 0 };	// 0: no value (Windows' limit, 10 per the docs)
		std::vector<std::wstring> ModuleIgnoreList;
	};

	// no key: the defaults (Exists false)
	std::expected<SilentProcessExitSettings, DWORD> ReadSilentProcessExit(Locations const& where, std::wstring const& image);
	// creates the key; empty strings and lists remove their values
	DWORD WriteSilentProcessExit(Locations const& where, std::wstring const& image, SilentProcessExitSettings const& settings);
	// no key: success
	DWORD DeleteSilentProcessExit(Locations const& where, std::wstring const& image);
}
