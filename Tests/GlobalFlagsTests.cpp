#include "pch.h"
#include "TestFramework.h"
#include "GlobalFlags.h"
#include "Mitigations.h"
#include "Helpers.h"
#include "PageHeap.h"
#include "CommandLine.h"

using namespace GFlags;

TEST(FlagValuesAreSingleUniqueBits) {
	uint32_t seen = 0;
	for (auto& flag : All()) {
		CHECK(flag.Value != 0 && (flag.Value & (flag.Value - 1)) == 0);
		CHECK((seen & flag.Value) == 0);
		seen |= flag.Value;
		CHECK(flag.Targets != Target::None);
	}
}

TEST(FlagsForTarget) {
	auto image = ForTarget(Target::Image);
	// Silent Process Exit is an image's only
	CHECK(std::ranges::any_of(image, [](auto f) { return f->Value == MonitorSilentProcessExit; }));
	CHECK(std::ranges::none_of(ForTarget(Target::Registry), [](auto f) { return f->Value == MonitorSilentProcessExit; }));
	// Stop on Hung GUI is the kernel's only
	auto kernel = ForTarget(Target::Kernel);
	CHECK(std::ranges::any_of(kernel, [](auto f) { return f->Value == 0x8; }));
	CHECK(std::ranges::none_of(image, [](auto f) { return f->Value == 0x8; }));
}

TEST(EveryBitIsAFlag) {
	// GFlags' table has all 32
	uint32_t all = 0;
	for (auto& flag : All())
		all |= flag.Value;
	CHECK_EQ(all, 0xffffffffu);
	CHECK(std::ranges::any_of(ForTarget(Target::Image), [](auto f) { return f->Value == 0x10000; }));	// Disable Stack Extension
	CHECK((TargetMask(Target::Kernel) & 0xc00000) == 0xc00000);	// close exception, exception logging
	CHECK((TargetMask(Target::Image) & 0xc00000) == 0);
}

TEST(TargetMaskCoversTheTargetsFlags) {
	auto mask = TargetMask(Target::Registry);
	for (auto flag : ForTarget(Target::Registry))
		CHECK((mask & flag->Value) == flag->Value);
	CHECK((mask & 0x8) == 0);	// the kernel's only
	CHECK((mask & MonitorSilentProcessExit) == 0);
}

TEST(FormatValue) {
	CHECK_EQ(FormatValue(0x2000000), std::wstring(L"0x02000000"));
	CHECK_EQ(FormatValue(0), std::wstring(L"0x00000000"));
}

namespace {
	Mitigations::Field const* FindField(PCWSTR name) {
		auto fields = Mitigations::Fields();
		auto it = std::ranges::find_if(fields, [&](auto& f) { return std::wstring_view(f.Name) == name; });
		return it == fields.end() ? nullptr : &*it;
	}

	bool IsSet(Mitigations::Options const& options, int bit) {
		return (options.Word[bit / 64] >> (bit % 64)) & 1;
	}
}

TEST(MitigationFieldStates) {
	using namespace Mitigations;
	auto aslr = FindField(L"Force ASLR (Relocate Images)");
	CHECK(aslr != nullptr);
	if (!aslr)
		return;
	Options options{ { 0x101 } };	// DEP, Force ASLR always on
	CHECK(GetState(options, *aslr) == State::AlwaysOn);
	options = SetState(options, *aslr, State::AlwaysOff);
	CHECK_EQ(options.Word[0], 0x201ull);
	// the other bits stay
	options.Word[0] |= 1ull << 63;
	options.Word[1] = 0x5;
	options = SetState(options, *FindField(L"Bottom-Up ASLR"), State::AlwaysOn);
	CHECK_EQ(options.Word[0], 0x8000000000010201ull);
	CHECK_EQ(options.Word[1], 0x5ull);
	CHECK(GetState(options, *aslr) == State::AlwaysOff);
}

TEST(MitigationsAsWindowsWritesThem) {
	using namespace Mitigations;
	// the bits Set-ProcessMitigation wrote for each (Windows 11, 2026): a field's first bit (always on), and for some,
	// with their variant, the next one too (the field's other choice)
	struct Seen {
		PCWSTR Name;
		int Bit;
		PCWSTR Other;	// the variant's name, if it was written
	};
	const Seen seen[] = {
		{ L"SEHOP", 4, L"Always On, Telemetry" },
		{ L"Force ASLR (Relocate Images)", 8, L"Always On, Require Relocations" },
		{ L"Heap Terminate on Corruption", 12 },
		{ L"Bottom-Up ASLR", 16 },
		{ L"High Entropy ASLR", 20 },
		{ L"Strict Handle Checks", 24 },
		{ L"Disable Win32k System Calls", 28 },
		{ L"Disable Extension Points", 32 },
		{ L"Prohibit Dynamic Code", 36 },
		{ L"Control Flow Guard", 40, L"Export Suppression" },
		{ L"Block Non-Microsoft Binaries", 44, L"Allow Store" },
		{ L"Block Non-System Fonts", 48 },
		{ L"Block Remote Image Loads", 52 },
		{ L"Block Low Integrity Image Loads", 56 },
		{ L"Prefer System32 Images", 60 },
		{ L"Loader Integrity Continuity", 68 },
		{ L"Strict Control Flow Guard", 72 },
		{ L"ROP: Stack Pivot", 80 },
		{ L"ROP: Caller Check", 84 },
		{ L"ROP: Simulated Execution", 88 },
		{ L"Export Address Filter", 92 },
		{ L"Export Address Filter Plus", 96 },
		{ L"Disallow Child Processes", 100 },
		{ L"Import Address Filter", 104 },
		{ L"Module Tampering Protection", 108 },
		{ L"CET Shadow Stacks", 124, L"Strict Mode" },
		{ L"Redirection Trust", 148 },
		{ L"Disable FSCTL System Calls", 156 },
	};
	for (auto& s : seen) {
		auto field = FindField(s.Name);
		CHECK(field != nullptr);
		if (!field)
			continue;
		CHECK_EQ(field->Shift(), s.Bit);
		CHECK(!field->Inferred);
		auto on = SetState({}, *field, State::AlwaysOn);
		CHECK(IsSet(on, s.Bit) && !IsSet(on, s.Bit + 1));
		if (s.Other) {
			auto other = SetState({}, *field, State::Other);
			CHECK(IsSet(other, s.Bit) && IsSet(other, s.Bit + 1));
			CHECK_EQ(StateName(*field, State::Other), std::wstring(s.Other));
		}
	}
	// DEP and ATL thunk emulation: bits 0 and 1
	CHECK(Bits().size() == 2 && Bits()[0].Value == 0x1 && Bits()[1].Value == 0x2);
}

TEST(MitigationFieldsDontOverlap) {
	uint64_t seen[Mitigations::Words]{};
	std::vector<int> options;
	for (auto& field : Mitigations::Fields()) {
		CHECK(field.Option > 0 && field.Shift() < Mitigations::Words * 64);
		CHECK(std::ranges::find(options, field.Option) == options.end());
		options.push_back(field.Option);
		auto mask = 3ull << (field.Shift() % 64);
		auto& word = seen[field.Shift() / 64];
		CHECK((word & mask) == 0);
		word |= mask;
	}
	for (auto& bit : Mitigations::Bits())
		CHECK((seen[0] & bit.Value) == 0);
	// in option order
	CHECK(std::ranges::is_sorted(options));
}

TEST(MitigationStateNames) {
	using namespace Mitigations;
	auto heap = FindField(L"Heap Terminate on Corruption");
	CHECK(heap != nullptr);
	if (!heap)
		return;
	CHECK_EQ(StateName(*heap, State::Other), std::wstring(L"Reserved (3)"));
	CHECK_EQ(StateName(*heap, State::Default), std::wstring(L"Default"));
	CHECK_EQ(StateName(*heap, State::AlwaysOff), std::wstring(L"Always Off"));
}

TEST(MitigationsParseAndFormat) {
	using namespace Mitigations;
	CHECK_EQ(Format({ { 0x100 } }), std::wstring(L"0000000000000100"));
	CHECK_EQ(Format({ { 0x100, 0x10000000 } }), std::wstring(L"00000000100000000000000000000100"));
	CHECK_EQ(Format({ { 0x1, 0, 0x2 } }), std::wstring(L"000000000000000200000000000000000000000000000001"));
	CHECK(Parse(L"100") == Options{ { 0x100 } });
	CHECK(Parse(L" 0x1000000000000100 ") == Options{ { 0x1000000000000100 } });
	CHECK(Parse(L"ffffffffffffffff") == Options{ { ~0ull } });
	// past 16 digits: the higher words'
	CHECK(Parse(L"11112222333344445") == (Options{ { 0x1112222333344445, 0x1 } }));
	CHECK(Parse(L"200000000000000000000000000000001") == (Options{ { 0x1, 0, 0x2 } }));
	CHECK(Parse(Format({ { 0xabc, 0xdef, 0x123 } })) == (Options{ { 0xabc, 0xdef, 0x123 } }));
	CHECK(!Parse(L""));
	CHECK(!Parse(L"12G"));
	CHECK(!Parse(std::wstring(49, L'1')));		// 49 digits
	// ordered as one number
	CHECK((Options{ { ~0ull, 0 } } < Options{ { 0, 1 } }));
	CHECK((Options{ { 0, ~0ull } } < Options{ { 0, 0, 1 } }));
}

TEST(SplitList) {
	using Helpers::SplitList;
	CHECK(SplitList(L"a.dll\r\n b.dll ,c.dll\r\n\r\n") == (std::vector<std::wstring>{ L"a.dll", L"b.dll", L"c.dll" }));
	CHECK(SplitList(L"  ").empty());
	CHECK_EQ(Helpers::JoinLines({ L"a", L"b" }), std::wstring(L"a\r\nb"));
}

TEST(ParseNumber) {
	using Helpers::ParseNumber;
	CHECK(ParseNumber(L"136") == 136u);
	CHECK(ParseNumber(L" 0x88 ") == 0x88u);
	CHECK(ParseNumber(L"0XfFfFfFfF") == 0xffffffffu);
	CHECK(ParseNumber(L"4294967295") == 0xffffffffu);
	CHECK(!ParseNumber(L"4294967296"));		// past 32 bits
	CHECK(!ParseNumber(L"0x100000000"));
	CHECK(!ParseNumber(L""));
	CHECK(!ParseNumber(L"0x"));
	CHECK(!ParseNumber(L"88h"));
	CHECK(!ParseNumber(L"ff"));				// hex needs 0x
}

TEST(SplitAndJoinWords) {
	using namespace Helpers;
	CHECK(SplitWords(L" a.dll  b.dll,c.dll\r\nd.dll ") == (std::vector<std::wstring>{ L"a.dll", L"b.dll", L"c.dll", L"d.dll" }));
	CHECK(SplitWords(L"  ").empty());
	CHECK_EQ(JoinWords({ L"a.dll", L"", L"b.dll" }), std::wstring(L"a.dll b.dll"));
}

TEST(PageHeapSummaryAndOptions) {
	using namespace PageHeap;
	CHECK_EQ(Summary(0), std::wstring(L"Normal"));
	CHECK_EQ(Summary(Defaults), std::wstring(L"Normal: stack traces"));
	CHECK_EQ(Summary(Full | StackTraces | BackwardOverruns | SizeRange), std::wstring(L"Full: stack traces, backward overruns, size range"));
	// as gflags.exe's /p options set them
	CHECK_EQ(Full | BackwardOverruns, 0x11u);
	CHECK_EQ(Full | Decommit, 0x41u);
	CHECK_EQ(Full | Protect, 0x1001u);
	CHECK_EQ(Full | TargetDlls, 0x401u);
	uint32_t seen = 0;
	for (auto& option : Options()) {
		CHECK((option.Value & (option.Value - 1)) == 0);
		CHECK((seen & option.Value) == 0);
		CHECK((option.Value & (Full | SizeRange | Random | TargetDlls | Faults)) == 0);	// those have controls of their own
		seen |= option.Value;
	}
}

TEST(FlagAbbreviations) {
	// gflags.exe's, unique, and every flag has one but Silent Process Exit
	std::vector<std::wstring> seen;
	for (auto& flag : All()) {
		if (flag.Value == MonitorSilentProcessExit) {
			CHECK(flag.Abbreviation == nullptr);
			continue;
		}
		CHECK(flag.Abbreviation != nullptr);
		if (!flag.Abbreviation)
			continue;
		CHECK(std::ranges::find(seen, flag.Abbreviation) == seen.end());
		seen.push_back(flag.Abbreviation);
		CHECK(FindByAbbreviation(flag.Abbreviation) == &flag);
	}
	CHECK(FindByAbbreviation(L"HPA") && FindByAbbreviation(L"HPA")->Value == 0x02000000);
	CHECK(FindByAbbreviation(L"ust")->Value == 0x1000);
	CHECK(FindByAbbreviation(L"dpd")->Value == 0x80000000);
	CHECK(!FindByAbbreviation(L"xyz"));
	CHECK(!FindByAbbreviation(L"hp"));
}

TEST(CommandLineParse) {
	using namespace CommandLine;
	auto ui = Parse({});
	CHECK(ui && ui->What == Action::Ui);
	auto usage = Parse({ L"/?" });
	CHECK(usage && usage->What == Action::Usage);

	auto show = Parse({ L"-r" });
	CHECK(show && show->What == Action::Show && show->Target == Target::Registry);
	auto image = Parse({ L"/I", L"notepad.exe", L"+hpa", L"+UST", L"-htc" });
	CHECK(image.has_value());
	if (!image)
		return;
	CHECK(image->What == Action::Change && image->Target == Target::Image && image->Image == L"notepad.exe");
	CHECK_EQ(image->Set, 0x02001000u);
	CHECK_EQ(image->Clear, 0x10u);
	CHECK_EQ(NewValue(*image, 0x12), 0x02001002u);

	// a value, then changes on it; the last change of a bit wins
	auto kernel = Parse({ L"/k", L"0x3", L"-soe", L"+0x100", L"-0x100" });
	CHECK(kernel.has_value());
	if (kernel) {
		CHECK(kernel->Value == 3u);
		CHECK_EQ(NewValue(*kernel, 0xffff), 0x2u);
	}

	CHECK(!Parse({ L"/x" }));
	CHECK(!Parse({ L"/i" }));
	CHECK(!Parse({ L"/i", L"a\\b.exe" }));
	CHECK(!Parse({ L"/r", L"hpa" }));			// not + or -
	CHECK(!Parse({ L"/r", L"+nope" }));
	CHECK(!Parse({ L"/k", L"+dic" }));			// the registry's only
	CHECK(!Parse({ L"/i", L"a.exe", L"+ksl" }));	// not an image's
	CHECK(!Parse({ L"/r", L"0x1", L"0x2" }));	// one value
	CHECK(!Parse({ L"/r", L"+0x100000000" }));
}

TEST(CommandLineDescribe) {
	using namespace CommandLine;
	auto text = Describe(0x02001000, Target::Image);
	CHECK(text.starts_with(L"0x02001000"));
	CHECK(text.find(L"hpa  Enable Page Heap") != std::wstring::npos);
	CHECK(text.find(L"ust  Create User Mode Stack Trace Database") != std::wstring::npos);
	// a bit that's no flag of the target
	CHECK(Describe(0x8, Target::Image).find(L"0x00000008 (not flags of an image)") != std::wstring::npos);
	CHECK(Usage().find(L"hpa  rki  Enable Page Heap") != std::wstring::npos);
}

TEST(FileName) {
	CHECK_EQ(Helpers::FileName(L"C:\\Windows\\notepad.exe"), std::wstring(L"notepad.exe"));
	CHECK_EQ(Helpers::FileName(L"notepad.exe"), std::wstring(L"notepad.exe"));
}
