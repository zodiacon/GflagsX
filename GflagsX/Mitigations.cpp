#include "pch.h"
#include "Mitigations.h"

using namespace Mitigations;

namespace {
	const Bit s_Bits[] = {
		{ L"DEP", 0x1 },						// Set-ProcessMitigation's DEP
		{ L"DEP ATL Thunk Emulation", 0x2 },	// its EmulateAtlThunks
	};

	// By PS_MITIGATION_OPTION, with their other choice (3) where there's one. The numbers were checked against what
	// Set-ProcessMitigation writes (Windows 11, 2026): 1-15, 17, 18, 20-27, 31, 37 and 39; 19, 28-30 follow the kernel's
	// list (phnt) between them; 32-36 and 38 are inferred: the list as phnt has it (whose 35 and 37 are 37 and 39 here)
	// with XFG and pointer authentication in CreateProcess's POLICY2 order. 16 (Return Flow Guard) was never shipped.
	const Field s_Fields[] = {
		{ L"SEHOP", 1, L"Always On, Telemetry" },
		{ L"Force ASLR (Relocate Images)", 2, L"Always On, Require Relocations" },
		{ L"Heap Terminate on Corruption", 3, nullptr },
		{ L"Bottom-Up ASLR", 4, nullptr },
		{ L"High Entropy ASLR", 5, nullptr },
		{ L"Strict Handle Checks", 6, nullptr },
		{ L"Disable Win32k System Calls", 7, nullptr },
		{ L"Disable Extension Points", 8, nullptr },
		{ L"Prohibit Dynamic Code", 9, L"Always On, Allow Opt Out" },
		{ L"Control Flow Guard", 10, L"Export Suppression" },
		{ L"Block Non-Microsoft Binaries", 11, L"Allow Store" },
		{ L"Block Non-System Fonts", 12, L"Audit" },
		{ L"Block Remote Image Loads", 13, nullptr },
		{ L"Block Low Integrity Image Loads", 14, nullptr },
		{ L"Prefer System32 Images", 15, nullptr },
		{ L"Loader Integrity Continuity", 17, L"Audit" },
		{ L"Strict Control Flow Guard", 18, nullptr },
		{ L"Restrict Set Thread Context", 19, nullptr },
		{ L"ROP: Stack Pivot", 20, nullptr },
		{ L"ROP: Caller Check", 21, nullptr },
		{ L"ROP: Simulated Execution", 22, nullptr },
		{ L"Export Address Filter", 23, nullptr },
		{ L"Export Address Filter Plus", 24, nullptr },
		{ L"Disallow Child Processes", 25, nullptr },
		{ L"Import Address Filter", 26, nullptr },
		{ L"Module Tampering Protection", 27, L"Always On, Not Inherited" },
		{ L"Restrict Indirect Branch Prediction", 28, nullptr },
		{ L"Speculative Store Bypass Disable", 29, nullptr },
		{ L"Allow Dynamic Code Policy Downgrade", 30, nullptr },
		{ L"CET Shadow Stacks", 31, L"Strict Mode" },
		{ L"CET Set Context IP Validation", 32, L"Relaxed Mode", true },
		{ L"Block Non-CET Binaries", 33, L"Non-EHCONT Binaries", true },
		{ L"Extended Control Flow Guard (XFG)", 34, nullptr, true },
		{ L"Pointer Authentication of User IP", 35, nullptr, true },
		{ L"CET Dynamic APIs Out of Process Only", 36, nullptr, true },
		{ L"Redirection Trust", 37, nullptr },
		{ L"Restrict Core Sharing", 38, nullptr, true },
		{ L"Disable FSCTL System Calls", 39, nullptr },
	};
}

std::span<const Field> Mitigations::Fields() {
	return s_Fields;
}

std::span<const Bit> Mitigations::Bits() {
	return s_Bits;
}

State Mitigations::GetState(Options const& options, Field const& field) {
	return State((options.Word[field.Shift() / 64] >> (field.Shift() % 64)) & 3);
}

Options Mitigations::SetState(Options options, Field const& field, State state) {
	auto& word = options.Word[field.Shift() / 64];
	int shift = field.Shift() % 64;
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
	// the highest word that isn't 0 (at least the first)
	int top = Words - 1;
	while (top > 0 && options.Word[top] == 0)
		top--;
	std::wstring text;
	for (int i = top; i >= 0; i--)
		text += std::format(L"{:016X}", options.Word[i]);
	return text;
}

std::optional<Options> Mitigations::Parse(std::wstring_view text) {
	while (!text.empty() && iswspace(text.front()))
		text.remove_prefix(1);
	while (!text.empty() && iswspace(text.back()))
		text.remove_suffix(1);
	if (text.size() > 2 && text[0] == L'0' && (text[1] == L'x' || text[1] == L'X'))
		text.remove_prefix(2);
	if (text.empty() || text.size() > Words * 16)
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
		// shifted in from the right, through the words
		for (int i = Words - 1; i > 0; i--)
			value.Word[i] = (value.Word[i] << 4) | (value.Word[i - 1] >> 60);
		value.Word[0] = (value.Word[0] << 4) | digit;
	}
	return value;
}
