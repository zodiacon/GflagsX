#include "pch.h"
#include "TestFramework.h"
#include "GlobalFlags.h"
#include "Mitigations.h"
#include "Helpers.h"

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
	uint64_t options = 0x101;	// DEP, Force ASLR always on
	CHECK(GetState(options, aslr) == State::AlwaysOn);
	options = SetState(options, aslr, State::AlwaysOff);
	CHECK_EQ(options, 0x201ull);
	// the other bits stay
	options = SetState(options | (1ull << 63), fields[2], State::AlwaysOn);
	CHECK_EQ(options, 0x8000000000010201ull);
	CHECK(GetState(options, aslr) == State::AlwaysOff);
	options = SetState(options, aslr, State::Default);
	CHECK_EQ(options, 0x8000000000010001ull);
}

TEST(MitigationFieldsDontOverlap) {
	uint64_t seen = 0;
	for (auto& field : Mitigations::Fields()) {
		auto mask = 3ull << field.Shift;
		CHECK((seen & mask) == 0);
		seen |= mask;
	}
	for (auto& bit : Mitigations::Bits())
		CHECK((seen & bit.Value) == 0);
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
	CHECK_EQ(Format(0x100), std::wstring(L"0000000000000100"));
	CHECK(Parse(L"100") == 0x100ull);
	CHECK(Parse(L" 0x1000000000000100 ") == 0x1000000000000100ull);
	CHECK(Parse(L"ffffffffffffffff") == ~0ull);
	CHECK(!Parse(L""));
	CHECK(!Parse(L"12G"));
	CHECK(!Parse(L"11112222333344445"));	// 17 digits
}

TEST(SplitList) {
	using Helpers::SplitList;
	CHECK(SplitList(L"a.dll\r\n b.dll ,c.dll\r\n\r\n") == (std::vector<std::wstring>{ L"a.dll", L"b.dll", L"c.dll" }));
	CHECK(SplitList(L"  ").empty());
	CHECK_EQ(Helpers::JoinLines({ L"a", L"b" }), std::wstring(L"a\r\nb"));
}

TEST(FileName) {
	CHECK_EQ(Helpers::FileName(L"C:\\Windows\\notepad.exe"), std::wstring(L"notepad.exe"));
	CHECK_EQ(Helpers::FileName(L"notepad.exe"), std::wstring(L"notepad.exe"));
}
