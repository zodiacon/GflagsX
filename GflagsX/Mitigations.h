#pragma once

//
// An image's mitigation options (Image File Execution Options' MitigationOptions): the first 64 bits of the
// PROCESS_CREATION_MITIGATION_POLICY_* flags. DEP, ATL thunk emulation and SEHOP are single bits; every other
// mitigation is a field of its own (4 bits, of which 2 are used): 0 the default, 1 always on, 2 always off, and for some
// a third choice (3). The value's bits that no entry describes are kept as they are.
//
namespace Mitigations {
	enum class State : uint32_t {
		Default = 0,
		AlwaysOn = 1,
		AlwaysOff = 2,
		Other = 3,		// the entry's OtherName, or reserved
	};

	struct Field {
		PCWSTR Name;
		int Shift;				// the field's first bit
		PCWSTR OtherName;		// what 3 means, if anything
	};

	struct Bit {
		PCWSTR Name;
		uint64_t Value;
	};

	std::span<const Field> Fields();
	// DEP, ATL thunk emulation, SEHOP
	std::span<const Bit> Bits();

	State GetState(uint64_t options, Field const& field);
	uint64_t SetState(uint64_t options, Field const& field, State state);
	// a state's text for the field: "Default", "Always On", "Always Off", the field's OtherName or "Reserved (3)"
	std::wstring StateName(Field const& field, State state);

	// "0000000000000100" (16 digits)
	std::wstring Format(uint64_t options);
	// hex digits (an optional 0x, up to 16 digits); nullopt if it's not that
	std::optional<uint64_t> Parse(std::wstring_view text);
}
