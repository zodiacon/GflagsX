using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GflagsX.ViewModels {
	class ProcessMitigationsViewModel : BindableBase {
		public void Update(ulong mitigations) {
			IsDEPEnabled = (mitigations & 1) == 1;
			IsDEPATLEnabled = (mitigations & 2) == 2;
			IsDEPSEHOPEnabled = (mitigations & 4) == 4;

			ASLRMitigation = MitigationBits(mitigations, 8, 9);
			HeapTerminationMitigation = MitigationBits(mitigations, 12, 13);
			BottomUpASLR = MitigationBits(mitigations, 16, 17);
			HighEntropyASLR = MitigationBits(mitigations, 20, 21);
            DisallowStrippedImages = MitigationBits(mitigations, 22, 23);
			StrictHandleChecks = MitigationBits(mitigations, 24, 25);
			DisableExtensionPoint = MitigationBits(mitigations, 32, 33);
			DisableDynamicCode = MitigationBits(mitigations, 36, 37);
			ControlFlowGuard = MitigationBits(mitigations, 40, 41);
			BlockNonMicrosoftBinaries = MitigationBits(mitigations, 44, 45);
			BlockNonSystemFonts = MitigationBits(mitigations, 48, 49);
			DisableRemoteLoads = MitigationBits(mitigations, 52, 53);
			DisableLowIntegrityLoads = MitigationBits(mitigations, 56, 57);
			PreferSystemImages = MitigationBits(mitigations, 60, 61);
		}


		public string MitigationsValueString {
			get => _mitigationsValue.ToString("X");
			set => MitigationsValue = Convert.ToUInt64(value, 16);
		}

		private ulong _mitigationsValue;

		public ulong MitigationsValue {
			get => _mitigationsValue;
			set {
				if (SetProperty(ref _mitigationsValue, value)) {
					Update(value);
					RaisePropertyChanged(nameof(MitigationsValueString));
				}
			}
		}

		private int MitigationBits(ulong value, params int[] bits) {
			foreach (var bit in bits)
				if ((value & (1UL << bit)) > 0)
					return bit;
			return 0;
		}

		public ulong Apply() {
            return MitigationsValue =
                (ASLRMitigation == 0 ? 0 : 1UL << ASLRMitigation) |
                (HeapTerminationMitigation == 0 ? 0 : 1UL << HeapTerminationMitigation) |
                (IsDEPEnabled ? 1UL : 0) | (IsDEPATLEnabled ? 2UL : 0) | (IsDEPSEHOPEnabled ? 4UL : 0) |
                (HighEntropyASLR == 0 ? 0 : 1UL << HighEntropyASLR) |
                (BottomUpASLR == 0 ? 0 : 1UL << BottomUpASLR) |
                (StrictHandleChecks == 0 ? 0 : 1UL << StrictHandleChecks) |
                (DisableWin32KCalls == 0 ? 0 : 1UL << DisableWin32KCalls) |
                (DisableExtensionPoint == 0 ? 0 : 1UL << DisableExtensionPoint) |
                (DisableDynamicCode == 0 ? 0 : 1UL << DisableDynamicCode) |
                (ControlFlowGuard == 0 ? 0 : 1UL << ControlFlowGuard) |
                (BlockNonMicrosoftBinaries == 0 ? 0 : 1UL << BlockNonMicrosoftBinaries) |
                (BlockNonSystemFonts == 0 ? 0 : 1UL << BlockNonSystemFonts) |
                (DisableRemoteLoads == 0 ? 0 : 1UL << DisableRemoteLoads) |
                (DisableLowIntegrityLoads == 0 ? 0 : 1UL << DisableLowIntegrityLoads) |
                (PreferSystemImages == 0 ? 0 : 1UL << PreferSystemImages) |
                (DisallowStrippedImages == 0 ? 0 : 1UL << DisallowStrippedImages);
		}

		private bool _isDEPEnabled;

		public bool IsDEPEnabled {
			get => _isDEPEnabled;
			set {
				if (SetProperty(ref _isDEPEnabled, value)) {
					Apply();
				}
			}
		}

		private bool _isDEPATLEnabled;

		public bool IsDEPATLEnabled {
			get => _isDEPATLEnabled;
			set {
				if (SetProperty(ref _isDEPATLEnabled, value)) {
					Apply();
				}
			}
		}

        int _disallowStrippedImages;
        public int DisallowStrippedImages {
            get => _disallowStrippedImages;
            set {
                if (SetProperty(ref _disallowStrippedImages, value)) {
                    Apply();
                }
            }
        }

        private bool _isDEPSEHOPEnabled;

		public bool IsDEPSEHOPEnabled {
			get => _isDEPSEHOPEnabled;
			set {
				if (SetProperty(ref _isDEPSEHOPEnabled, value)) {
					Apply();
				}
			}
		}

		private int _aslrMitigation;

		public int ASLRMitigation {
			get => _aslrMitigation;
            set {
				if (SetProperty(ref _aslrMitigation, value)) {
					Apply();
				}
			}
		}

		private int _heapTerminationMitigation;

		public int HeapTerminationMitigation {
			get => _heapTerminationMitigation;
			set {
				if (SetProperty(ref _heapTerminationMitigation, value)) {
					Apply();
				}
			}
		}

		private int _bottomUpASLR;

		public int BottomUpASLR {
			get => _bottomUpASLR;
			set {
				if (SetProperty(ref _bottomUpASLR, value))
					Apply();
			}
		}

		private int _highEntropyASLR;

		public int HighEntropyASLR {
			get => _highEntropyASLR;
			set {
				if (SetProperty(ref _highEntropyASLR, value)) {
					Apply();
				}
			}
		}

		private int _strictHandleChecks;

		public int StrictHandleChecks {
			get => _strictHandleChecks;
			set {
				if (SetProperty(ref _strictHandleChecks, value))
					Apply();
			}
		}

		private int _disableWin32KCalls;

		public int DisableWin32KCalls {
			get => _disableWin32KCalls;
			set { if (SetProperty(ref _disableWin32KCalls, value))
					Apply();
			}
		}

		private int _disableExtensionPoint;

		public int DisableExtensionPoint {
			get => _disableExtensionPoint;
			set {
				if (SetProperty(ref _disableExtensionPoint, value))
					Apply();
			}
		}

		private int _preferSystemImages;

		public int PreferSystemImages {
			get => _preferSystemImages;
			set {
				if (SetProperty(ref _preferSystemImages, value))
					Apply();
			}
		}

		private int _disableLowIntegrityLoads;

		public int DisableLowIntegrityLoads {
			get => _disableLowIntegrityLoads;
			set {
				if (SetProperty(ref _disableLowIntegrityLoads, value))
					Apply();
			}
		}

		private int _disableRemoteLoads;

		public int DisableRemoteLoads {
			get => _disableRemoteLoads;
			set {
				if (SetProperty(ref _disableRemoteLoads, value))
					Apply();
			}
		}

		private int _blockNonSystemFonts;

		public int BlockNonSystemFonts {
			get => _blockNonSystemFonts;
			set {
				if (SetProperty(ref _blockNonSystemFonts, value))
					Apply();
			}
		}

		private int _blockNonMicrosoftBinaries;

		public int BlockNonMicrosoftBinaries {
			get => _blockNonMicrosoftBinaries;
			set {
				if (SetProperty(ref _blockNonMicrosoftBinaries, value))
					Apply();
			}
		}

		private int _controlFlowGuard;

		public int ControlFlowGuard {
			get => _controlFlowGuard;
			set {
				if (SetProperty(ref _controlFlowGuard, value))
					Apply();
			}
		}

		private int _disableDynamicCode;

		public int DisableDynamicCode {
			get => _disableDynamicCode;
			set {
				if (SetProperty(ref _disableDynamicCode, value))
					Apply();
			}
		}

	}
}
