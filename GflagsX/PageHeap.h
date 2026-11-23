#pragma once

//
// Page heap's options: an image's PageHeapFlags bits, as gflags.exe's /p options set them (its code: /full 0x1, stack
// traces 0x2 unless /notraces, /backwards 0x10, /unaligned 0x20, /decommit 0x40, /size 0x80, /address 0x100, /random
// 0x200, /dlls 0x400, /fault 0x800, /protect 0x1000, /no_lock_checks 0x4000). Page heap is on with the image's Enable
// Page Heap flag (0x02000000); without Full it's the normal (light) page heap. The bits no entry describes are kept.
//
namespace PageHeap {
	constexpr uint32_t Full = 0x1;
	constexpr uint32_t StackTraces = 0x2;
	constexpr uint32_t BackwardOverruns = 0x10;
	constexpr uint32_t Unaligned = 0x20;
	constexpr uint32_t Decommit = 0x40;
	constexpr uint32_t SizeRange = 0x80;		// PageHeapSizeRangeStart..End
	constexpr uint32_t DllRange = 0x100;		// PageHeapDllRangeStart..End (addresses: not shown, kept)
	constexpr uint32_t Random = 0x200;			// PageHeapRandomProbability
	constexpr uint32_t TargetDlls = 0x400;		// PageHeapTargetDlls
	constexpr uint32_t Faults = 0x800;			// PageHeapFaultProbability, PageHeapFaultTimeOut
	constexpr uint32_t Protect = 0x1000;
	constexpr uint32_t NoLockChecks = 0x4000;

	// the GFlags /p defaults: normal page heap with stack traces
	constexpr uint32_t Defaults = StackTraces;

	struct Option {
		PCWSTR Name;
		uint32_t Value;
		bool NeedsFull;		// the option is full page heap's: setting it sets Full (as gflags.exe does)
	};

	// the options that are check boxes and nothing more (not Full, nor those with values)
	std::span<const Option> Options();

	// "Full: stack traces, backward overruns, size range" / "Normal: stack traces" / "Normal"
	std::wstring Summary(uint32_t flags);
}
