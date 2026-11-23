#include "pch.h"
#include "CommandLine.h"
#include "FlagStore.h"
#include "Helpers.h"

using namespace CommandLine;
using namespace GFlags;

namespace {
	bool IsSwitch(std::wstring const& arg, wchar_t letter) {
		return arg.size() == 2 && (arg[0] == L'/' || arg[0] == L'-') && towlower(arg[1]) == letter;
	}

	bool IsHex(std::wstring_view text) {
		return text.size() > 2 && text[0] == L'0' && (text[1] == L'x' || text[1] == L'X');
	}

	PCWSTR TargetName(Target target) {
		switch (target) {
			case Target::Registry: return L"the registry";
			case Target::Kernel: return L"the running system";
		}
		return L"an image";
	}
}

std::expected<Command, std::wstring> CommandLine::Parse(std::vector<std::wstring> const& args) {
	Command command;
	if (args.empty())
		return command;

	auto& first = args[0];
	if (IsSwitch(first, L'?') || IsSwitch(first, L'h')) {
		command.What = Action::Usage;
		return command;
	}
	size_t next = 1;
	if (IsSwitch(first, L'r'))
		command.Target = Target::Registry;
	else if (IsSwitch(first, L'k'))
		command.Target = Target::Kernel;
	else if (IsSwitch(first, L'i')) {
		command.Target = Target::Image;
		if (args.size() < 2 || !FlagStore::IsValidImageName(args[1]))
			return std::unexpected(std::wstring(L"/i needs an image name (such as notepad.exe)."));
		command.Image = args[1];
		next = 2;
	}
	else
		return std::unexpected(std::format(L"Unknown argument: {}. /? shows what the command line takes.", first));

	command.What = Action::Show;
	for (size_t i = next; i < args.size(); i++) {
		auto& arg = args[i];
		if (IsHex(arg)) {
			auto value = Helpers::ParseNumber(arg);
			if (!value)
				return std::unexpected(std::format(L"Not a 32-bit number in hex: {}.", arg));
			if (command.Value)
				return std::unexpected(std::wstring(L"One value at most (0x...): +flag and -flag change it."));
			command.Value = *value;
			command.What = Action::Change;
			continue;
		}
		if (arg.size() < 2 || (arg[0] != L'+' && arg[0] != L'-'))
			return std::unexpected(std::format(L"Not a change: {}. Changes are +flag, -flag (abbreviations or 0x... bits) or 0x... (the value).", arg));
		auto name = std::wstring_view(arg).substr(1);
		uint32_t bits;
		if (IsHex(name)) {
			auto value = Helpers::ParseNumber(name);
			if (!value)
				return std::unexpected(std::format(L"Not a 32-bit number in hex: {}.", name));
			bits = *value;
		}
		else {
			auto flag = FindByAbbreviation(name);
			if (!flag)
				return std::unexpected(std::format(L"Unknown flag: {}. /? lists the abbreviations.", name));
			if (!HasTarget(flag->Targets, command.Target))
				return std::unexpected(std::format(L"{} ({}) can't be set in {}.", flag->Abbreviation, flag->Name, TargetName(command.Target)));
			bits = flag->Value;
		}
		// the last change of a bit wins
		if (arg[0] == L'+') {
			command.Set |= bits;
			command.Clear &= ~bits;
		}
		else {
			command.Clear |= bits;
			command.Set &= ~bits;
		}
		command.What = Action::Change;
	}
	return command;
}

uint32_t CommandLine::NewValue(Command const& command, uint32_t value) {
	return (command.Value.value_or(value) & ~command.Clear) | command.Set;
}

std::wstring CommandLine::Describe(uint32_t value, Target target) {
	auto text = FormatValue(value);
	uint32_t described = 0;
	for (auto flag : ForTarget(target)) {
		if (value & flag->Value) {
			text += std::format(L"\r\n    {:<4} {}", flag->Abbreviation ? flag->Abbreviation : L"", flag->Name);
			described |= flag->Value;
		}
	}
	if (auto other = value & ~described)
		text += std::format(L"\r\n    {} (not flags of {})", FormatValue(other), TargetName(target));
	return text;
}

std::wstring CommandLine::Usage() {
	std::wstring text =
		L"GflagsX [/r | /k | /i image] [changes]\r\n"
		L"\r\n"
		L"  /r        the system's flags in the registry (for the next boot)\r\n"
		L"  /k        the running system's flags (until it restarts)\r\n"
		L"  /i image  an image's flags (its processes started from now on)\r\n"
		L"\r\n"
		L"Changes: +flag or -flag (an abbreviation below, or bits as 0x...), or 0x... (the value).\r\n"
		L"Without changes, the flags are shown. Without arguments, the UI starts.\r\n"
		L"\r\n"
		L"Flags (where they can be set: r, k, i):";
	for (auto& flag : All()) {
		if (!flag.Abbreviation)
			continue;
		auto where = std::format(L"{}{}{}", HasTarget(flag.Targets, Target::Registry) ? L"r" : L"-",
			HasTarget(flag.Targets, Target::Kernel) ? L"k" : L"-", HasTarget(flag.Targets, Target::Image) ? L"i" : L"-");
		text += std::format(L"\r\n  {:<4} {}  {}", flag.Abbreviation, where, flag.Name);
	}
	text += L"\r\n\r\nExamples: GflagsX /i notepad.exe +hpa +ust    GflagsX /r -ksl    GflagsX /k";
	return text;
}
