#include "pch.h"
#include "Mitigations.h"

using namespace Mitigations;

namespace {
	const Bit s_Bits[] = {
		{ L"DEP", 0x1 },						// PROCESS_CREATION_MITIGATION_POLICY_DEP_ENABLE
		{ L"DEP ATL Thunk Emulation", 0x2 },	// ..._DEP_ATL_THUNK_ENABLE
		{ L"SEHOP", 0x4 },						// ..._SEHOP_ENABLE
	};

	// winnt.h's PROCESS_CREATION_MITIGATION_POLICY_*_MASK fields of the first 64 bits, with their third choice where
	// there's one
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
	};
}

std::span<const Field> Mitigations::Fields() {
	return s_Fields;
}

std::span<const Bit> Mitigations::Bits() {
	return s_Bits;
}

State Mitigations::GetState(uint64_t options, Field const& field) {
	return State((options >> field.Shift) & 3);
}

uint64_t Mitigations::SetState(uint64_t options, Field const& field, State state) {
	return (options & ~(3ull << field.Shift)) | (uint64_t(state) << field.Shift);
}

std::wstring Mitigations::StateName(Field const& field, State state) {
	switch (state) {
		case State::Default: return L"Default";
		case State::AlwaysOn: return L"Always On";
		case State::AlwaysOff: return L"Always Off";
	}
	return field.OtherName ? field.OtherName : L"Reserved (3)";
}

std::wstring Mitigations::Format(uint64_t options) {
	return std::format(L"{:016X}", options);
}

std::optional<uint64_t> Mitigations::Parse(std::wstring_view text) {
	while (!text.empty() && iswspace(text.front()))
		text.remove_prefix(1);
	while (!text.empty() && iswspace(text.back()))
		text.remove_suffix(1);
	if (text.size() > 2 && text[0] == L'0' && (text[1] == L'x' || text[1] == L'X'))
		text.remove_prefix(2);
	if (text.empty() || text.size() > 16)
		return std::nullopt;
	uint64_t value = 0;
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
		value = (value << 4) | digit;
	}
	return value;
}
