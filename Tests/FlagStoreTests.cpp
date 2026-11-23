#include "pch.h"
#include "TestFramework.h"
#include "FlagStore.h"

#include <DbgHelp.h>	// MINIDUMP_TYPE

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
	s.Mitigations.Policy = 0x100000101;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);
	auto read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value());
	CHECK_EQ(read->GlobalFlag, 0x2000000u);
	CHECK_EQ(read->GlobalFlagType, (DWORD)REG_DWORD);
	CHECK_EQ(read->Debugger, s.Debugger);
	CHECK(read->Mitigations == Mitigations::Options{ 0x100000101 });
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
	CHECK_EQ(read->Mitigations.Policy, 0x100ull);
	CHECK_EQ(read->Mitigations.Policy2, 0x2000000000000010ull);
	CHECK_EQ(read->MitigationsType, (DWORD)REG_BINARY);
	CHECK(read->MitigationsMore.empty());

	read->Mitigations.Policy = 0x200;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	BYTE written[32]{};
	DWORD size = sizeof(written), type = 0;
	CHECK_EQ(::RegGetValue(HKEY_CURRENT_USER, keys.Image(L"app.exe").c_str(), L"MitigationOptions", RRF_RT_ANY, &type, written, &size), (LONG)ERROR_SUCCESS);
	CHECK_EQ(type, (DWORD)REG_BINARY);
	CHECK_EQ(size, (DWORD)16);
	CHECK(written[1] == 0x02 && written[8] == 0x10 && written[15] == 0x20);

	// Policy all 0, Policy2 not: the value stays
	read->Mitigations.Policy = 0;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(keys.ValueType(keys.Image(L"app.exe"), L"MitigationOptions"), (DWORD)REG_BINARY);
}

TEST(ImageMitigationsBeyondPolicy2AreKept) {
	TestKeys keys;
	CreateImage(keys.Where, L"app.exe");
	// 24 bytes: Policy, Policy2 and 8 bytes of mitigations newer than this
	BYTE data[24]{ 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0, 0, 0, 0, 0x55, 0, 0, 0, 0, 0, 0, 0x66 };
	keys.SetValue(keys.Image(L"app.exe"), L"MitigationOptions", REG_BINARY, data, sizeof(data));
	auto read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value());
	if (!read)
		return;
	CHECK(read->Mitigations == (Mitigations::Options{ 0x1, 0x10000000 }));
	CHECK_EQ(read->MitigationsMore.size(), (size_t)8);

	read->Mitigations = {};
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	BYTE written[32]{};
	DWORD size = sizeof(written), type = 0;
	CHECK_EQ(::RegGetValue(HKEY_CURRENT_USER, keys.Image(L"app.exe").c_str(), L"MitigationOptions", RRF_RT_ANY, &type, written, &size), (LONG)ERROR_SUCCESS);
	CHECK_EQ(size, (DWORD)24);
	CHECK(written[0] == 0 && written[11] == 0 && written[16] == 0x55 && written[23] == 0x66);
}

TEST(ImageQwordMitigationsBecomeBinaryForPolicy2) {
	TestKeys keys;
	CreateImage(keys.Where, L"app.exe");
	ImageSettings s;
	s.Mitigations.Policy = 0x100;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(keys.ValueType(keys.Image(L"app.exe"), L"MitigationOptions"), (DWORD)REG_QWORD);

	// a REG_QWORD has no room for Policy2
	auto read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value());
	if (!read)
		return;
	CHECK(read->Mitigations == Mitigations::Options{ 0x100 });
	read->Mitigations.Policy2 = 1ull << 28;	// CET shadow stacks: always on
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(keys.ValueType(keys.Image(L"app.exe"), L"MitigationOptions"), (DWORD)REG_BINARY);
	auto again = ReadImage(keys.Where, L"app.exe");
	CHECK(again.has_value() && again->Mitigations == (Mitigations::Options{ 0x100, 1ull << 28 }));
}

TEST(ImagePageHeapAndVerifier) {
	TestKeys keys;
	CreateImage(keys.Where, L"app.exe");
	auto image = keys.Image(L"app.exe");
	// not shown, so never written: stays
	DWORD start = 0x10000;
	keys.SetValue(image, L"PageHeapDllRangeStart", REG_DWORD, &start, sizeof(start));

	ImageSettings s;
	s.PageHeap.Flags = 0x83;
	s.PageHeap.SizeRangeStart = 16;
	s.PageHeap.SizeRangeEnd = 4096;
	s.PageHeap.TargetDlls = { L"a.dll", L"b.dll" };
	s.Verifier.Dlls = { L"vfbasics.dll", L"vfcompat.dll" };
	s.Verifier.Flags = 0x80000000;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);

	// as gflags.exe writes them: PageHeapFlags a string, the DLLs separated by spaces
	CHECK_EQ(keys.ValueType(image, L"PageHeapFlags"), (DWORD)REG_SZ);
	WCHAR text[64]{};
	DWORD size = sizeof(text);
	CHECK_EQ(::RegGetValue(HKEY_CURRENT_USER, image.c_str(), L"PageHeapFlags", RRF_RT_REG_SZ, nullptr, text, &size), (LONG)ERROR_SUCCESS);
	CHECK_EQ(std::wstring(text), std::wstring(L"0x83"));
	size = sizeof(text);
	CHECK_EQ(::RegGetValue(HKEY_CURRENT_USER, image.c_str(), L"PageHeapTargetDlls", RRF_RT_REG_SZ, nullptr, text, &size), (LONG)ERROR_SUCCESS);
	CHECK_EQ(std::wstring(text), std::wstring(L"a.dll b.dll"));
	CHECK_EQ(keys.ValueType(image, L"PageHeapSizeRangeEnd"), (DWORD)REG_DWORD);
	CHECK_EQ(keys.ValueType(image, L"PageHeapRandomProbability"), (DWORD)REG_NONE);	// 0
	CHECK_EQ(keys.ValueType(image, L"VerifierFlags"), (DWORD)REG_DWORD);

	auto read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value());
	if (!read)
		return;
	CHECK_EQ(read->PageHeap.Flags, 0x83u);
	CHECK_EQ(read->PageHeap.FlagsType, (DWORD)REG_SZ);
	CHECK_EQ(read->PageHeap.SizeRangeEnd, 4096u);
	CHECK(read->PageHeap.TargetDlls == s.PageHeap.TargetDlls);
	CHECK(read->Verifier.Dlls == s.Verifier.Dlls);
	CHECK_EQ(read->Verifier.Flags, 0x80000000u);

	// a REG_DWORD PageHeapFlags stays one; nothing set: the values go, the others stay
	DWORD flags = 0x3;
	keys.SetValue(image, L"PageHeapFlags", REG_DWORD, &flags, sizeof(flags));
	read = ReadImage(keys.Where, L"app.exe");
	CHECK(read.has_value() && read->PageHeap.Flags == 3 && read->PageHeap.FlagsType == REG_DWORD);
	read->PageHeap.Flags = 0x13;
	CHECK_EQ(WriteImage(keys.Where, L"app.exe", *read), (DWORD)ERROR_SUCCESS);
	CHECK_EQ(keys.ValueType(image, L"PageHeapFlags"), (DWORD)REG_DWORD);

	CHECK_EQ(WriteImage(keys.Where, L"app.exe", ImageSettings{}), (DWORD)ERROR_SUCCESS);
	for (auto name : { L"PageHeapFlags", L"PageHeapSizeRangeStart", L"PageHeapSizeRangeEnd", L"PageHeapTargetDlls", L"VerifierDlls", L"VerifierFlags" })
		CHECK_EQ(keys.ValueType(image, name), (DWORD)REG_NONE);
	CHECK_EQ(keys.ValueType(image, L"PageHeapDllRangeStart"), (DWORD)REG_DWORD);
}

TEST(VerifierGlobalSettingsIsNotAnImage) {
	TestKeys keys;
	CreateImage(keys.Where, L"app.exe");
	CHECK(GetVerifierProviders(keys.Where).empty());
	// as Application Verifier has it
	auto global = keys.Where.ImageOptions + L"\\" + VerifierGlobalSettings;
	HKEY key;
	CHECK_EQ(::RegCreateKeyEx(HKEY_CURRENT_USER, global.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr), (LONG)ERROR_SUCCESS);
	std::wstring providers = L"vrfcore.dll vfbasics.dll  vfcompat.dll";
	::RegSetValueEx(key, L"VerifierProviders", 0, REG_SZ, reinterpret_cast<BYTE const*>(providers.c_str()), DWORD((providers.size() + 1) * sizeof(WCHAR)));
	::RegCloseKey(key);

	auto images = GetImages(keys.Where);
	CHECK(images.has_value() && *images == std::vector<std::wstring>{ L"app.exe" });
	CHECK(GetVerifierProviders(keys.Where) == (std::vector<std::wstring>{ L"vrfcore.dll", L"vfbasics.dll", L"vfcompat.dll" }));
}

TEST(SilentProcessExitRoundTrip) {
	TestKeys keys;
	auto none = ReadSilentProcessExit(keys.Where, L"app.exe");
	CHECK(none.has_value() && !none->Exists && none->DumpType == MiniDump && none->MaxNumberOfDumpFiles == 0);

	SilentProcessExitSettings s;
	s.IgnoreSelfExits = true;
	s.ReportingMode = LaunchMonitorProcess | LocalDump;
	s.MonitorProcess = L"monitor.exe -p";
	s.LocalDumpFolder = L"C:\\Dumps";
	s.DumpType = HeapDump;
	s.MaxNumberOfDumpFiles = 5;
	s.ModuleIgnoreList = { L"a.dll", L"b.dll" };
	CHECK_EQ(WriteSilentProcessExit(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);

	auto read = ReadSilentProcessExit(keys.Where, L"app.exe");
	CHECK(read.has_value());
	CHECK(read->Exists);
	CHECK(read->IgnoreSelfExits);
	CHECK_EQ(read->ReportingMode, LaunchMonitorProcess | LocalDump);
	CHECK_EQ(read->MonitorProcess, s.MonitorProcess);
	CHECK_EQ(read->LocalDumpFolder, s.LocalDumpFolder);
	CHECK_EQ(read->DumpType, HeapDump);
	CHECK_EQ(read->MaxNumberOfDumpFiles, 5u);
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

TEST(SilentProcessExitDumpTypes) {
	// DumpType is MINIDUMP_TYPE flags, as GFlags writes them (Micro is the docs' example)
	CHECK_EQ(MicroDump, DWORD(MiniDumpFilterMemory | MiniDumpFilterModulePaths));
	CHECK(PresetOf(0x88) == DumpPreset::Micro);
	CHECK(PresetOf(0x121) == DumpPreset::Mini);
	CHECK(PresetOf(0x10B25) == DumpPreset::Heap);
	CHECK(PresetOf(2) == DumpPreset::Custom);		// MiniDumpWithFullMemory
	CHECK(PresetOf(0) == DumpPreset::Custom);
	for (auto preset : { DumpPreset::Micro, DumpPreset::Mini, DumpPreset::Heap })
		CHECK(PresetValue(preset) && PresetOf(*PresetValue(preset)) == preset);
	CHECK(!PresetValue(DumpPreset::Custom));

	// a custom value is written as it is, and no limit removes MaxNumberOfDumpFiles
	TestKeys keys;
	SilentProcessExitSettings s;
	s.DumpType = MiniDumpWithFullMemory;
	s.MaxNumberOfDumpFiles = 3;
	CHECK_EQ(WriteSilentProcessExit(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);
	s.MaxNumberOfDumpFiles = 0;
	CHECK_EQ(WriteSilentProcessExit(keys.Where, L"app.exe", s), (DWORD)ERROR_SUCCESS);
	auto read = ReadSilentProcessExit(keys.Where, L"app.exe");
	CHECK(read.has_value());
	if (!read)
		return;
	CHECK_EQ(read->DumpType, DWORD(MiniDumpWithFullMemory));
	CHECK(PresetOf(read->DumpType) == DumpPreset::Custom);
	auto exitKey = keys.Where.SilentProcessExit + L"\\app.exe";
	CHECK_EQ(keys.ValueType(exitKey, L"MaxNumberOfDumpFiles"), (DWORD)REG_NONE);
}

TEST(KernelFlagsRead) {
	// reading needs no privilege; the value is the system's (writing it isn't tested: it changes the running system)
	auto flags = ReadKernelFlags();
	CHECK(flags.has_value());
}
