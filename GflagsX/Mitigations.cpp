#include "pch.h"
#include "Mitigations.h"

using namespace Mitigations;

namespace {
	const Bit s_Bits[] = {
		{ L"DEP", 0x1 },						// PROCESS_CREATION_MITIGATION_POLICY_DEP_ENABLE
		{ L"DEP ATL Thunk Emulation", 0x2 },	// ..._DEP_ATL_THUNK_ENABLE
		{ L"SEHOP", 0x4 },						// ..._SEHOP_ENABLE
	};

	// winbase.h's PROCESS_CREATION_MITIGATION_POLICY_*_MASK and PROCESS_CREATION_MITIGATION_POLICY2_*_MASK fields (Policy2's
	// at 64 + their shift), with their third choice where there's one
	const Field s_Fields[] = {
		{ L"Force ASLR (Relocate Images)", 8, L"Always On, Require Relocations" },
		{ L"Heap Terminate on Corruption", 12, nullptr },
		{ L"Bottom-Up ASLR", 16, nullptr },
		{ L"High Entropy ASLR", 20, nullptr },
		{ L"Strict Handle Checks", 24, nullptr },
		{ L"Disable Win32k System Calls", 28, nullptr },
		{ L"Disable Extension Points", 32, nullptr },
		{ L"Prohibit Dynamic Code", 36, L"Always On, Allow Opt Out" },
		{ L"Control Flow Guard", 40, L"Export Suppression" },
		{ L"Block Non-Microsoft Binaries", 44, L"Allow Store" },
		{ L"Block Non-System Fonts", 48, L"Audit" },
		{ L"Block Remote Image Loads", 52, nullptr },
		{ L"Block Low Integrity Image Loads", 56, nullptr },
		{ L"Prefer System32 Images", 60, nullptr },

		{ L"Loader Integrity Continuity", 64 + 4, L"Audit" },
		{ L"Strict Control Flow Guard", 64 + 8, nullptr },
		{ L"Module Tampering Protection", 64 + 12, L"Always On, Not Inherited" },
		{ L"Restrict Indirect Branch Prediction", 64 + 16, nullptr },
		{ L"Allow Dynamic Code Policy Downgrade", 64 + 20, nullptr },
		{ L"Speculative Store Bypass Disable", 64 + 24, nullptr },
		{ L"CET Shadow Stacks", 64 + 28, L"Strict Mode" },
		{ L"CET Set Context IP Validation", 64 + 32, L"Relaxed Mode" },
		{ L"Block Non-CET Binaries", 64 + 36, L"Non-EHCONT Binaries" },
		{ L"Extended Control Flow Guard (XFG)", 64 + 40, nullptr },
		{ L"Pointer Authentication of User IP", 64 + 44, nullptr },
		{ L"CET Dynamic APIs Out of Process Only", 64 + 48, nullptr },
		{ L"Restrict Core Sharing", 64 + 52, nullptr },
		{ L"Disable FSCTL System Calls", 64 + 56, nullptr },
	};

	uint64_t& Word(Options& options, Field const& field) {
		return field.Shift < 64 ? options.Policy : options.Policy2;
	}
}

std::span<const Field> Mitigations::Fields() {
	return s_Fields;
}

std::span<const Bit> Mitigations::Bits() {
	return s_Bits;
}

State Mitigations::GetState(Options const& options, Field const& field) {
	auto word = field.Shift < 64 ? options.Policy : options.Policy2;
	return State((word >> (field.Shift % 64)) & 3);
}

Options Mitigations::SetState(Options options, Field const& field, State state) {
	auto& word = Word(options, field);
	int shift = field.Shift % 64;
	word = (word & ~(3ull << shift)) | (uint64_t(state) << shift);
	return options;
}

std::wstring Mitigations::StateName(Field const& field, State state) {
	switch (state) {
		case State::Default: return L"Default";
		case State::AlwaysOn: return L"Always On";
		case State::AlwaysOff: return L"Always Off";
	}
	return field.OtherName ? field.OtherName : L"Reserved (3)";
}

std::wstring Mitigations::Format(Options const& options) {
	if (options.Policy2 == 0)
		return std::format(L"{:016X}", options.Policy);
	return std::format(L"{:016X}{:016X}", options.Policy2, options.Policy);
}

std::optional<Options> Mitigations::Parse(std::wstring_view text) {
	while (!text.empty() && iswspace(text.front()))
		text.remove_prefix(1);
	while (!text.empty() && iswspace(text.back()))
		text.remove_suffix(1);
	if (text.size() > 2 && text[0] == L'0' && (text[1] == L'x' || text[1] == L'X'))
		text.remove_prefix(2);
	if (text.empty() || text.size() > 32)
		return std::nullopt;
	Options value;
	for (auto c : text) {
		int digit;
		if (c >= L'0' && c <= L'9')
			digit = c - L'0';
		else if (c >= L'a' && c <= L'f')
			digit = c - L'a' + 10;
		else if (c >= L'A' && c <= L'F')
			digit = c - L'A' + 10;
		else
			return std::nullopt;
		// shifted in from the right, through Policy into Policy2
		value.Policy2 = (value.Policy2 << 4) | (value.Policy >> 60);
		value.Policy = (value.Policy << 4) | digit;
	}
	return value;
}
