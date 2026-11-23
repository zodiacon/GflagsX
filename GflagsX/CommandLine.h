#pragma once

#include "GlobalFlags.h"

//
// The command line, GFlags style: a target, then changes to its flags. With no changes, the flags are shown.
//
//   GflagsX /r [changes]          the system's flags in the registry (for the next boot)
//   GflagsX /k [changes]          the running system's flags
//   GflagsX /i image [changes]    an image's flags
//
// A change is +abbr or -abbr (gflags.exe's abbreviations: +hpa, -ust), +0x... or -0x... (bits in hex), or 0x... (the
// value as a whole, before the other changes). An abbreviation must be a flag of the target; bits in hex can be any.
// No arguments: the UI. /? (or -?, /h): the usage.
//
namespace CommandLine {
	enum class Action {
		Ui,			// no arguments
		Usage,
		Show,		// a target, no changes
		Change,
	};

	struct Command {
		Action What{ Action::Ui };
		GFlags::Target Target{ GFlags::Target::None };
		std::wstring Image;					// /i's
		std::optional<uint32_t> Value;		// 0x...: the value as a whole
		uint32_t Set{ 0 };
		uint32_t Clear{ 0 };
	};

	// the arguments without the program's name; an error is a message to show
	std::expected<Command, std::wstring> Parse(std::vector<std::wstring> const& args);
	// the value after the command's changes
	uint32_t NewValue(Command const& command, uint32_t value);
	// the value and its flags, one per line ("0x02001000" then "    hpa  Enable Page Heap"); bits no flag of the
	// target covers are on a line of their own
	std::wstring Describe(uint32_t value, GFlags::Target target);
	// what the command line takes, with the abbreviations
	std::wstring Usage();
}
