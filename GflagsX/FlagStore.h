#pragma once

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
		uint64_t Mitigations{ 0 };
		// a REG_BINARY MitigationOptions' bytes past the first 8 (newer mitigations, which aren't shown), kept
		std::vector<BYTE> MitigationsMore;
		DWORD MitigationsType{ REG_NONE };	// REG_QWORD or REG_BINARY (or REG_DWORD); REG_NONE: no value

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

	// the documented DumpType values
	enum class DumpType : DWORD {
		Micro, Mini, Heap, Custom,
	};

	struct SilentProcessExitSettings {
		bool Exists{ false };	// read: the image has a key
		bool IgnoreSelfExits{ false };
		DWORD ReportingMode{ 0 };
		std::wstring MonitorProcess;
		std::wstring LocalDumpFolder;
		DumpType DumpType{ DumpType::Mini };
		std::vector<std::wstring> ModuleIgnoreList;
	};

	// no key: the defaults (Exists false)
	std::expected<SilentProcessExitSettings, DWORD> ReadSilentProcessExit(Locations const& where, std::wstring const& image);
	// creates the key; empty strings and lists remove their values
	DWORD WriteSilentProcessExit(Locations const& where, std::wstring const& image, SilentProcessExitSettings const& settings);
	// no key: success
	DWORD DeleteSilentProcessExit(Locations const& where, std::wstring const& image);
}
