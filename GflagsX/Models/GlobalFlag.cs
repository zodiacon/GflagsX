using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GflagsX.Models {
	[Flags]
	enum GlobalFlagUsage {
		Registry = 1,
		Kernel = 2,
		Image = 4,
		All = Registry | Kernel | Image
	}

	enum GlobalFlagCategory {
		None,
		KernelModeOnly,
		UserModeOnly,
		UserModeAndKernelMode
	}

	class GlobalFlag {
		public GlobalFlagCategory Category { get; set; }
		public string Name { get; }
		public string Description { get; set; }
		public uint Value { get; }
		public GlobalFlagUsage Usage { get; }

		public GlobalFlag(string name, uint value, GlobalFlagUsage usage) {
			Name = name;
			Value = value;
			Usage = usage;
		}
	}
}
