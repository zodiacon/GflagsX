#include "pch.h"
#include "GlobalFlags.h"

using namespace GFlags;

namespace {
	constexpr auto Reg = Target::Registry;
	constexpr auto Ker = Target::Kernel;
	constexpr auto Img = Target::Image;
	constexpr auto All_ = Target::All;

	// GFlags' flag table (learn.microsoft.com, "GFlags Flag Table"): where each flag can be set, what it affects, and its
	// abbreviation (gflags.exe's: its command line's; Silent Process Exit has none)
	const Flag s_Flags[] = {
		{ L"Stop on Exception", 0x1, All_, Category::UserMode, L"soe" },
		{ L"Show Loader Snaps", 0x2, All_, Category::UserMode, L"sls" },
		{ L"Debug Initial Command", 0x4, Reg, Category::UserMode, L"dic" },
		{ L"Stop on Hung GUI", 0x8, Ker, Category::UserMode, L"shg" },
		{ L"Enable Heap Tail Checking", 0x10, All_, Category::UserMode, L"htc" },
		{ L"Enable Heap Free Checking", 0x20, All_, Category::UserMode, L"hfc" },
		{ L"Enable Heap Parameter Checking", 0x40, All_, Category::UserMode, L"hpc" },
		{ L"Enable Heap Validation on Call", 0x80, All_, Category::UserMode, L"hvc" },
		{ L"Enable Application Verifier", 0x100, All_, Category::UserMode, L"vrf" },
		{ L"Silent Process Exit", MonitorSilentProcessExit, Img, Category::UserMode, nullptr },
		{ L"Enable Pool Tagging", 0x400, Reg, Category::KernelMode, L"ptg" },
		{ L"Enable Heap Tagging", 0x800, All_, Category::UserMode, L"htg" },
		{ L"Create User Mode Stack Trace Database", 0x1000, All_, Category::UserMode, L"ust" },
		{ L"Create Kernel Mode Stack Trace Database", 0x2000, Reg, Category::KernelMode, L"kst" },
		{ L"Maintain a List of Objects for Each Type", 0x4000, Reg, Category::Both, L"otl" },
		{ L"Enable Heap Tagging by DLL", 0x8000, All_, Category::UserMode, L"htd" },
		{ L"Disable Stack Extension", 0x10000, Img, Category::UserMode, L"dse" },
		{ L"Enable Debugging of the Windows Subsystem", 0x20000, Reg, Category::UserMode, L"d32" },
		{ L"Enable Loading of Kernel Debugging Symbols", 0x40000, Reg | Ker, Category::KernelMode, L"ksl" },
		{ L"Disable Paging of Kernel Stacks", 0x80000, Reg, Category::KernelMode, L"dps" },
		{ L"Enable System Critical Breaks", 0x100000, All_, Category::Both, L"scb" },
		{ L"Disable Heap Coalesce on Free", 0x200000, All_, Category::UserMode, L"dhc" },
		{ L"Enable Close Exception", 0x400000, Reg | Ker, Category::UserMode, L"ece" },
		{ L"Enable Exception Logging", 0x800000, Reg | Ker, Category::KernelMode, L"eel" },
		{ L"Enable Object Handle Type Tagging", 0x01000000, Reg | Ker, Category::Both, L"eot" },
		{ L"Enable Page Heap", 0x02000000, All_, Category::UserMode, L"hpa" },
		{ L"Debug WinLogon", 0x04000000, Reg, Category::Both, L"dwl" },
		{ L"Buffer DbgPrint Output", 0x08000000, Reg | Ker, Category::KernelMode, L"ddp" },
		{ L"Early Critical Section Event Creation", 0x10000000, All_, Category::UserMode, L"cse" },
		{ L"Stop on Unhandled User Mode Exception", 0x20000000, All_, Category::UserMode, L"sue" },
		{ L"Enable Bad Handles Detection", 0x40000000, Reg | Ker, Category::Both, L"bhd" },
		{ L"Disable Protected DLL Verification", 0x80000000, All_, Category::UserMode, L"dpd" },
	};
}

std::span<const Flag> GFlags::All() {
	return s_Flags;
}

std::vector<Flag const*> GFlags::ForTarget(Target target) {
	std::vector<Flag const*> flags;
	for (auto& flag : s_Flags)
		if (HasTarget(flag.Targets, target))
			flags.push_back(&flag);
	return flags;
}

uint32_t GFlags::TargetMask(Target target) {
	uint32_t mask = 0;
	for (auto& flag : s_Flags)
		if (HasTarget(flag.Targets, target))
			mask |= flag.Value;
	return mask;
}

PCWSTR GFlags::CategoryName(Category category) {
	switch (category) {
		case Category::KernelMode: return L"Kernel Mode";
		case Category::UserMode: return L"User Mode";
	}
	return L"Kernel and User Mode";
}

std::wstring GFlags::FormatValue(uint32_t value) {
	return std::format(L"0x{:08X}", value);
}

GFlags::Flag const* GFlags::FindByAbbreviation(std::wstring_view abbreviation) {
	for (auto& flag : s_Flags)
		if (flag.Abbreviation && abbreviation.size() == wcslen(flag.Abbreviation) &&
			_wcsnicmp(flag.Abbreviation, abbreviation.data(), abbreviation.size()) == 0)
			return &flag;
	return nullptr;
}
