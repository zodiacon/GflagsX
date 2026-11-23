#include "pch.h"
#include "TestFramework.h"
#include "FlagStore.h"

using namespace FlagStore;

//
// The registry functions under HKCU\Software\GflagsXTests (made and removed by each test), never the system's keys
//
namespace {
	constexpr PCWSTR TestKey = L"Software\\GflagsXTests";

	struct TestKeys {
		Locations Where = Locations::Under(HKEY_CURRENT_USER, TestKey);

		TestKeys() {
			::RegDeleteTree(HKEY_CURRENT_USER, TestKey);
			for (auto path : { &Where.SessionManager, &Where.ImageOptions, &Where.SilentProcessExit }) {
				HKEY key;
				if (::RegCreateKeyEx(HKEY_CURRENT_USER, path->c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) == ERROR_SUCCESS)
					::RegCloseKey(key);
			}
		}
		~TestKeys() {
			::RegDeleteTree(HKEY_CURRENT_USER, TestKey);
			::RegDeleteKey(HKEY_CURRENT_USER, TestKey);
		}

		void SetValue(std::wstring const& path, PCWSTR name, DWORD type, void const* data, DWORD size) const {
			::RegSetKeyValue(HKEY_CURRENT_USER, path.c_str(), name, type, data, size);
		}
		std::wstring Image(PCWSTR name) const {
			return Where.ImageOptions + L"\\" + name;
		}
		DWORD ValueType(std::wstring const& path, PCWSTR name) const {
			DWORD type = REG_NONE;
			if (::RegGetValue(HKEY_CURRENT_USER, path.c_str(), name, RRF_RT_ANY | RRF_NOEXPAND, &type, nullptr, nullptr) != ERROR_SUCCESS)
				return REG_NONE;
			return type;
		}
	};
}

TEST(RegistryFlags) {
	TestKeys keys;
	// no value: none set
	CHECK(ReadRegistryFlags(keys.Where) == 0u);
	CHECK_EQ(WriteRegistryFlags(keys.Where, 0x2000400), (DWORD)ERROR_SUCCESS);
	CHECK(ReadRegistryFlags(keys.Where) == 0x2000400u);
	CHECK_EQ(keys.ValueType(keys.Where.SessionManager, L"GlobalFlag"), (DWORD)REG_DWORD);
}

TEST(ImagesCreateListDelete) {
	TestKeys keys;
	CHECK_EQ(CreateImage(keys.Where, L"b.exe"), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(CreateImage(keys.Where, L"A.exe"), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(CreateImage(keys.Where, L"a.EXE"), (DWORD)ERROR_ALREADY_EXISTS);
	CHECK_EQ(CreateImage(keys.Where, L"x\\y.exe"), (DWORD)ERROR_INVALID_NAME);
	auto images = GetImages(keys.Where);
	CHECK(images.has_value() && *images == (std::vector<std::wstring>{ L"A.exe", L"b.exe" }));

	// with a subkey (as Windows' filters have) and Silent Process Exit settings: all go
	keys.SetValue(keys.Image(L"b.exe") + L"\\Sub", L"X", REG_DWORD, "\1\0\0\0", 4);
	SilentProcessExitSettings exit;
	exit.ReportingMode = LocalDump;
	CHECK_EQ(WriteSilentProcessExit(keys.Where, L"b.exe", exit), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(DeleteImage(keys.Where, L"b.exe"), (DWORD)ERROR_SUCCESS);
	images = GetImages(keys.Where);
	CHECK(images.has_value() && *images == std::vector<std::wstring>{ L"A.exe" });
	auto read = ReadSilentProcessExit(keys.Where, L"b.exe");
	CHECK(read.has_value() && !read->Exists);
}

TEST(ImageSettingsRoundTrip) {
	TestKeys keys;
	CreateImage(keys.Where, L"app.exe");
	auto settings = ReadImage(keys.Where, L"app.exe");
	CHECK(settings.has_value() && *settings == ImageSettings{});

	ImageSettings s;
	s.GlobalFlag = 0x2000000;
	s.Debugger = L"\"C:\\Debuggers\\windbg.exe\" -g";
	s.Mitigations = 0x100000101;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);
	auto read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value());
	CHECK_EQ(read->GlobalFlag, 0x2000000u);
	CHECK_EQ(read->GlobalFlagType, (DWORD)REG_DWORD);
	CHECK_EQ(read->Debugger, s.Debugger);
	CHECK_EQ(read->Mitigations, 0x100000101ull);
	CHECK_EQ(read->MitigationsType, (DWORD)REG_QWORD);

	// nothing set: the values go
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", ImageSettings{}), (DWORD)ERROR_SUCCESS);
	for (auto name : { L"GlobalFlag", L"Debugger", L"MitigationOptions" })
		CHECK_EQ(keys.ValueType(keys.Image(L"app.exe"), name), (DWORD)REG_NONE);
}

TEST(ImageGlobalFlagAsString) {
	TestKeys keys;
	CreateImage(keys.Where, L"app.exe");
	// as GFlags writes it
	std::wstring text = L"0x00000100";
	keys.SetValue(keys.Image(L"app.exe"), L"GlobalFlag", REG_SZ, text.c_str(), DWORD((text.size() + 1) * sizeof(WCHAR)));
	auto read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value());
	CHECK_EQ(read->GlobalFlag, 0x100u);
	CHECK_EQ(read->GlobalFlagType, (DWORD)REG_SZ);

	// written back as a string too
	read->GlobalFlag |= 0x2000000;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(keys.ValueType(keys.Image(L"app.exe"), L"GlobalFlag"), (DWORD)REG_SZ);
	auto again = ReadImage(keys.Where, L"app.exe");
	CHECK(again.has_value() && again->GlobalFlag == 0x2000100u);
}

TEST(ImageBinaryMitigationsKeepTheirMoreBytes) {
	TestKeys keys;
	CreateImage(keys.Where, L"app.exe");
	// 16 bytes, as Set-ProcessMitigation writes them: the second 8 are newer mitigations
	BYTE data[16]{ 0x00, 0x01, 0, 0, 0, 0, 0, 0, 0x10, 0, 0, 0, 0, 0, 0, 0x20 };
	keys.SetValue(keys.Image(L"app.exe"), L"MitigationOptions", REG_BINARY, data, sizeof(data));
	auto read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value());
	CHECK_EQ(read->Mitigations, 0x100ull);
	CHECK_EQ(read->MitigationsType, (DWORD)REG_BINARY);
	CHECK_EQ(read->MitigationsMore.size(), (size_t)8);

	read->Mitigations = 0x200;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	BYTE written[32]{};
	DWORD size = sizeof(written), type = 0;
	CHECK_EQ(::RegGetValue(HKEY_CURRENT_USER, keys.Image(L"app.exe").c_str(), L"MitigationOptions", RRF_RT_ANY, &type, written, &size), (LONG)ERROR_SUCCESS);
	CHECK_EQ(type, (DWORD)REG_BINARY);
	CHECK_EQ(size, (DWORD)16);
	CHECK(written[1] == 0x02 && written[8] == 0x10 && written[15] == 0x20);

	// the first 8 bytes all 0, the others not: the value stays
	read->Mitigations = 0;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(keys.ValueType(keys.Image(L"app.exe"), L"MitigationOptions"), (DWORD)REG_BINARY);
}

TEST(SilentProcessExitRoundTrip) {
	TestKeys keys;
	auto none = ReadSilentProcessExit(keys.Where, L"app.exe");
	CHECK(none.has_value() && !none->Exists && none->DumpType == DumpType::Mini);

	SilentProcessExitSettings s;
	s.IgnoreSelfExits = true;
	s.ReportingMode = LaunchMonitorProcess | LocalDump;
	s.MonitorProcess = L"monitor.exe -p";
	s.LocalDumpFolder = L"C:\\Dumps";
	s.DumpType = DumpType::Heap;
	s.ModuleIgnoreList = { L"a.dll", L"b.dll" };
	CHECK_EQ(WriteSilentProcessExit(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);

	auto read = ReadSilentProcessExit(keys.Where, L"app.exe");
	CHECK(read.has_value());
	CHECK(read->Exists);
	CHECK(read->IgnoreSelfExits);
	CHECK_EQ(read->ReportingMode, LaunchMonitorProcess | LocalDump);
	CHECK_EQ(read->MonitorProcess, s.MonitorProcess);
	CHECK_EQ(read->LocalDumpFolder, s.LocalDumpFolder);
	CHECK(read->DumpType == DumpType::Heap);
	CHECK(read->ModuleIgnoreList == s.ModuleIgnoreList);
	auto exitKey = keys.Where.SilentProcessExit + L"\\app.exe";
	CHECK_EQ(keys.ValueType(exitKey, L"DumpType"), (DWORD)REG_DWORD);
	CHECK_EQ(keys.ValueType(exitKey, L"ModuleIgnoreList"), (DWORD)REG_MULTI_SZ);

	// cleared: the values go
	s.IgnoreSelfExits = false;
	s.MonitorProcess.clear();
	s.ModuleIgnoreList.clear();
	CHECK_EQ(WriteSilentProcessExit(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);
	for (auto name : { L"IgnoreSelfExits", L"MonitorProcess", L"ModuleIgnoreList" })
		CHECK_EQ(keys.ValueType(exitKey, name), (DWORD)REG_NONE);

	CHECK_EQ(DeleteSilentProcessExit(keys.Where, L"app.exe"), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(DeleteSilentProcessExit(keys.Where, L"app.exe"), (DWORD)ERROR_SUCCESS);	// none: fine
	read = ReadSilentProcessExit(keys.Where, L"app.exe");
	CHECK(read.has_value() && !read->Exists);
}

TEST(KernelFlagsRead) {
	// reading needs no privilege; the value is the system's (writing it isn't tested: it changes the running system)
	auto flags = ReadKernelFlags();
	CHECK(flags.has_value());
}
