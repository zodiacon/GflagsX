using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Threading.Tasks;

namespace GflagsX {
	enum SystemInformationClass : uint {
		SystemFlagsInformation = 9
	}

	[SuppressUnmanagedCodeSecurity]
	static class NativeMethods {
		[DllImport("ntdll", ExactSpelling = true)]
		public static extern int NtSetSystemInformation(SystemInformationClass infoClass, ref uint flags, int size = 4);

		[DllImport("ntdll", ExactSpelling = true)]
		public static extern uint RtlGetNtGlobalFlags();
	}
}
