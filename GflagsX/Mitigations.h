#pragma once

//
// An image's mitigation options (Image File Execution Options' MitigationOptions): the PROCESS_CREATION_MITIGATION_POLICY_*
// flags (the first 64 bits) and the PROCESS_CREATION_MITIGATION_POLICY2_* flags (the next 64). DEP, ATL thunk emulation
// and SEHOP are single bits; every other mitigation is a field of its own (4 bits, of which 2 are used): 0 the default,
// 1 always on, 2 always off, and for some a third choice (3). The value's bits that no entry describes are kept as they are.
//
namespace Mitigations {
	struct Options {
		uint64_t Policy{ 0 };		// the first 64 bits
		uint64_t Policy2{ 0 };		// the next 64

		bool operator==(Options const&) const = default;
		// as one 128-bit number (Policy2 the high half)
		std::strong_ordering operator<=>(Options const& other) const {
			if (auto order = Policy2 <=> other.Policy2; order != 0)
				return order;
			return Policy <=> other.Policy;
		}
		bool IsZero() const {
			return Policy == 0 && Policy2 == 0;
		}
	};

	enum class State : uint32_t {
		Default = 0,
		AlwaysOn = 1,
		AlwaysOff = 2,
		Other = 3,		// the entry's OtherName, or reserved
	};

	struct Field {
		PCWSTR Name;
		int Shift;				// the field's first bit of the 128 (64 and up: Policy2's, from its bit 0)
		PCWSTR OtherName;		// what 3 means, if anything
	};

	struct Bit {
		PCWSTR Name;
		uint64_t Value;			// in Policy
	};

	// Policy's fields, then Policy2's
	std::span<const Field> Fields();
	// DEP, ATL thunk emulation, SEHOP
	std::span<const Bit> Bits();

	State GetState(Options const& options, Field const& field);
	Options SetState(Options options, Field const& field, State state);
	// a state's text for the field: "Default", "Always On", "Always Off", the field's OtherName or "Reserved (3)"
	std::wstring StateName(Field const& field, State state);

	// in hex as one 128-bit number: 16 digits ("0000000000000100") while Policy2 is 0, 32 when it isn't
	std::wstring Format(Options const& options);
	// hex digits (an optional 0x, up to 32 digits: the last 16 are Policy); nullopt if it's not that
	std::optional<Options> Parse(std::wstring_view text);
}
