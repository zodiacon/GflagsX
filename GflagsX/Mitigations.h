#pragma once

#include <array>

//
// An image's mitigation options (Image File Execution Options' MitigationOptions). The value is the kernel's map of
// the mitigations (PS_MITIGATION_OPTIONS_MAP): a field of 4 bits per mitigation, at its number (PS_MITIGATION_OPTION)
// times 4: 0 the default, 1 always on, 2 always off, 3 the mitigation's other choice (or reserved). Past the first 64
// bits this is NOT the layout of CreateProcess's PROCESS_CREATION_MITIGATION_POLICY2_* flags (winbase.h): those are
// another numbering of the same mitigations. Set-ProcessMitigation writes the map as 24 bytes (3 words); DEP's field
// is two bits of its own (DEP, ATL thunk emulation). The value's bits that no entry describes are kept as they are.
//
namespace Mitigations {
	constexpr int Words = 3;		// the map's 64-bit words that are shown and edited (24 bytes)

	struct Options {
		std::array<uint64_t, Words> Word{};		// the first 64 bits are Word[0]

		bool operator==(Options const&) const = default;
		// as one 192-bit number (Word[2] the highest)
		std::strong_ordering operator<=>(Options const& other) const {
			for (int i = Words - 1; i >= 0; i--)
				if (auto order = Word[i] <=> other.Word[i]; order != 0)
					return order;
			return std::strong_ordering::equal;
		}
		bool IsZero() const {
			return std::ranges::all_of(Word, [](auto w) { return w == 0; });
		}
		// past the first word (a REG_QWORD can't have them)
		bool HasHighWords() const {
			return std::any_of(Word.begin() + 1, Word.end(), [](auto w) { return w != 0; });
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
		int Option;				// PS_MITIGATION_OPTION: the field is at bit Option * 4
		PCWSTR OtherName;		// what 3 means, if anything
		// the option's number is inferred (from the kernel's list's order), not seen written by Set-ProcessMitigation
		bool Inferred{ false };

		int Shift() const {
			return Option * 4;
		}
	};

	struct Bit {
		PCWSTR Name;
		uint64_t Value;			// in Word[0]
	};

	// in option order
	std::span<const Field> Fields();
	// DEP and ATL thunk emulation: option 0's field, as two bits
	std::span<const Bit> Bits();

	State GetState(Options const& options, Field const& field);
	Options SetState(Options options, Field const& field, State state);
	// a state's text for the field: "Default", "Always On", "Always Off", the field's OtherName or "Reserved (3)"
	std::wstring StateName(Field const& field, State state);

	// in hex as one number: 16 digits ("0000000000000100") while the higher words are 0, 32 or 48 when they aren't
	std::wstring Format(Options const& options);
	// hex digits (an optional 0x, up to 48 digits: the last 16 are Word[0]); nullopt if it's not that
	std::optional<Options> Parse(std::wstring_view text);
}
