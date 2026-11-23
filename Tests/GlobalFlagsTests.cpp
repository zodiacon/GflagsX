#include "pch.h"
#include "TestFramework.h"
#include "GlobalFlags.h"
#include "Mitigations.h"
#include "Helpers.h"
#include "PageHeap.h"

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

TEST(MitigationFieldStates) {
	using namespace Mitigations;
	auto fields = Fields();
	auto& aslr = fields[0];
	CHECK_EQ(aslr.Shift, 8);
	Options options{ 0x101 };	// DEP, Force ASLR always on
	CHECK(GetState(options, aslr) == State::AlwaysOn);
	options = SetState(options, aslr, State::AlwaysOff);
	CHECK_EQ(options.Policy, 0x201ull);
	// the other bits stay
	options.Policy |= 1ull << 63;
	options = SetState(options, fields[2], State::AlwaysOn);
	CHECK_EQ(options.Policy, 0x8000000000010201ull);
	CHECK(GetState(options, aslr) == State::AlwaysOff);
	options = SetState(options, aslr, State::Default);
	CHECK_EQ(options.Policy, 0x8000000000010001ull);
	CHECK_EQ(options.Policy2, 0ull);
}

TEST(MitigationPolicy2Fields) {
	using namespace Mitigations;
	auto fields = Fields();
	auto shadow = std::ranges::find_if(fields, [](auto& f) { return std::wstring_view(f.Name) == L"CET Shadow Stacks"; });
	CHECK(shadow != fields.end());
	if (shadow == fields.end())
		return;
	CHECK_EQ(shadow->Shift, 64 + 28);	// PROCESS_CREATION_MITIGATION_POLICY2_CET_USER_SHADOW_STACKS_MASK
	Options options{ 0x101, 1ull << 63 };
	options = SetState(options, *shadow, State::Other);
	CHECK_EQ(options.Policy2, (1ull << 63) | (3ull << 28));
	CHECK_EQ(options.Policy, 0x101ull);		// Policy's bits stay
	CHECK(GetState(options, *shadow) == State::Other);
	CHECK_EQ(StateName(*shadow, State::Other), std::wstring(L"Strict Mode"));
	// a Policy field doesn't read Policy2's bits at the same shift
	CHECK(GetState(Options{ 0, 1ull << 28 }, fields[5]) == State::Default);	// Disable Win32k System Calls: 28
}

TEST(MitigationFieldsDontOverlap) {
	uint64_t seen[2]{};
	for (auto& field : Mitigations::Fields()) {
		CHECK(field.Shift >= 0 && field.Shift < 128 && field.Shift % 4 == 0);
		auto mask = 3ull << (field.Shift % 64);
		auto& word = seen[field.Shift / 64];
		CHECK((word & mask) == 0);
		word |= mask;
	}
	for (auto& bit : Mitigations::Bits())
		CHECK((seen[0] & bit.Value) == 0);
}

TEST(MitigationStateNames) {
	using namespace Mitigations;
	auto fields = Fields();
	CHECK_EQ(StateName(fields[0], State::Other), std::wstring(L"Always On, Require Relocations"));
	CHECK_EQ(StateName(fields[1], State::Other), std::wstring(L"Reserved (3)"));
	CHECK_EQ(StateName(fields[1], State::Default), std::wstring(L"Default"));
}

TEST(MitigationsParseAndFormat) {
	using namespace Mitigations;
	CHECK_EQ(Format({ 0x100 }), std::wstring(L"0000000000000100"));
	CHECK_EQ(Format({ 0x100, 0x10000000 }), std::wstring(L"00000000100000000000000000000100"));
	CHECK(Parse(L"100") == Options{ 0x100 });
	CHECK(Parse(L" 0x1000000000000100 ") == Options{ 0x1000000000000100 });
	CHECK(Parse(L"ffffffffffffffff") == Options{ ~0ull });
	// past 16 digits: the high ones are Policy2's
	CHECK(Parse(L"11112222333344445") == (Options{ 0x1112222333344445, 0x1 }));
	CHECK(Parse(L"00000000100000000000000000000100") == (Options{ 0x100, 0x10000000 }));
	CHECK(Parse(Format({ 0xabc, 0xdef })) == (Options{ 0xabc, 0xdef }));
	CHECK(!Parse(L""));
	CHECK(!Parse(L"12G"));
	CHECK(!Parse(L"111122223333444455556666777788889"));	// 33 digits
	// ordered as one 128-bit number
	CHECK((Options{ ~0ull, 0 } < Options{ 0, 1 }));
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

TEST(FileName) {
	CHECK_EQ(Helpers::FileName(L"C:\\Windows\\notepad.exe"), std::wstring(L"notepad.exe"));
	CHECK_EQ(Helpers::FileName(L"notepad.exe"), std::wstring(L"notepad.exe"));
}
