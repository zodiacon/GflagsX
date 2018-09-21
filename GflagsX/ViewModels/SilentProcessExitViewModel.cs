using Microsoft.Win32;
using System;
using System.Windows;
using Zodiacon.WPF;

namespace GflagsX.ViewModels {
	enum MiniDumpType : uint {
		Normal = 0x00000000,
		WithDataSegs = 0x00000001,
		WithFullMemory = 0x00000002,
		WithHandleData = 0x00000004,
		FilterMemory = 0x00000008,
		ScanMemory = 0x00000010,
		WithUnloadedModules = 0x00000020,
		WithIndirectlyReferencedMemory = 0x00000040,
		FilterModulePaths = 0x00000080,
		WithProcessThreadData = 0x00000100,
		WithPrivateReadWriteMemory = 0x00000200,
		WithoutOptionalData = 0x00000400,
		WithFullMemoryInfo = 0x00000800,
		WithThreadInfo = 0x00001000,
		WithCodeSegs = 0x00002000,
		WithoutAuxiliaryState = 0x00004000,
		WithFullAuxiliaryState = 0x00008000,
		WithPrivateWriteCopyMemory = 0x00010000,
		IgnoreInaccessibleMemory = 0x00020000,
		WithTokenInformation = 0x00040000,
		WithModuleHeaders = 0x00080000,
		FilterTriage = 0x00100000,
		ValidTypeFlags = 0x001fffff
	}

	enum DumpType : uint {
		
		Micro = MiniDumpType.FilterModulePaths | MiniDumpType.FilterMemory,
		Mini = MiniDumpType.WithTokenInformation | MiniDumpType.WithProcessThreadData | MiniDumpType.WithUnloadedModules | MiniDumpType.WithDataSegs,
		Heap = MiniDumpType.WithTokenInformation | MiniDumpType.WithPrivateReadWriteMemory | MiniDumpType.WithThreadInfo 
			| MiniDumpType.WithFullMemory | MiniDumpType.WithPrivateWriteCopyMemory | MiniDumpType.WithProcessThreadData
			| MiniDumpType.WithUnloadedModules | MiniDumpType.WithHandleData | MiniDumpType.WithDataSegs,
	}

	[Flags]
	enum ReportingMode {
		None = 0,
		LaunchMonitorProcess = 1,
		EnableDumpCollection = 2,
		EnableNotification = 4
	}

	sealed class SilentProcessExitViewModel : DialogViewModelBase {
		public const string SilentProcessExitKey = @"SOFTWARE\Microsoft\Windows NT\CurrentVersion\SilentProcessExit";

		ReportingMode _reportingMode;

		public SilentProcessExitViewModel(Window dialog, string imageName) : base(dialog) {
			ImageName = imageName;
			LoadSettings();
		}

		public string ImageName { get; }

		bool _ignoreSelfExits;
		public bool IgnoreSelfExits {
			get => _ignoreSelfExits;
			set => SetProperty(ref _ignoreSelfExits, value);
		}

		bool _launchMonitorProcess;
		public bool LaunchMonitorProcess {
			get => _launchMonitorProcess;
			set => SetProperty(ref _launchMonitorProcess, value);
		}

		string _dumpFolder;
		public string DumpFolder {
			get => _dumpFolder;
			set => SetProperty(ref _dumpFolder, value);
		}

		public string[] DumpTypes => Enum.GetNames(typeof(DumpType));

		DumpType _selectedDumpType = DumpType.Mini;
		public string SelectedDumpType {
			get => _selectedDumpType.ToString();
			set => _selectedDumpType = (DumpType)Enum.Parse(typeof(DumpType), value);
		}

		public bool IsDumpEnabled {
			get => _reportingMode.HasFlag(ReportingMode.EnableDumpCollection);
			set {
				if (value)
					_reportingMode |= ReportingMode.EnableDumpCollection;
				else
					_reportingMode &= ~ReportingMode.EnableDumpCollection;
				RaisePropertyChanged();
			}
		}

		string _ignoreLoadedModules;
		public string IgnoreLoadedModules {
			get => _ignoreLoadedModules;
			set => SetProperty(ref _ignoreLoadedModules, value);
		}

		string _monitorProcess;
		public string MonitorProcess {
			get => _monitorProcess;
			set => SetProperty(ref _monitorProcess, value);
		}

		void LoadSettings() {
			try {
				using (var key = Registry.LocalMachine.OpenSubKey(SilentProcessExitKey + "\\" + ImageName, false)) {
					IgnoreSelfExits = ((int?)key.GetValue(nameof(IgnoreSelfExits)) ?? 0) == 0 ? false : true;
					DumpFolder = key.GetValue("LocalDumpFolder") as string;
					var mode = key.GetValue(nameof(ReportingMode));
					_reportingMode = mode == null ? ReportingMode.None : (ReportingMode)(int)mode;
					var modules = key.GetValue("ModuleIgnoreList") as string[];
					if (modules != null)
						IgnoreLoadedModules = string.Join(Environment.NewLine, modules);
					MonitorProcess = key.GetValue(nameof(MonitorProcess)) as string;
					RaisePropertyChanged(nameof(IsDumpEnabled));
				}
			}
			catch {
			}
		}

		public void SaveSettings() {
			using (var key = Registry.LocalMachine.OpenSubKey(SilentProcessExitKey + "\\" + ImageName, true)) {
				if (IgnoreSelfExits)
					key.SetValue(nameof(IgnoreSelfExits), 1);
				else
					key.DeleteValue(nameof(IgnoreSelfExits), false);
				if (_reportingMode != ReportingMode.None)
					key.SetValue(nameof(ReportingMode), (int)_reportingMode);
				else
					key.DeleteValue(nameof(ReportingMode), false);

				if (IsDumpEnabled && !string.IsNullOrWhiteSpace(DumpFolder)) {
					key.SetValue("LocalDumpFolder", DumpFolder);
					key.SetValue(nameof(DumpType), _selectedDumpType);
					if (!string.IsNullOrWhiteSpace(IgnoreLoadedModules))
						key.SetValue("ModuleIgnoreList", IgnoreLoadedModules.Split(new string[] { Environment.NewLine, "," }, StringSplitOptions.RemoveEmptyEntries));
					else
						key.DeleteValue("ModuleIgnoreList", false);

					if (LaunchMonitorProcess)
						key.SetValue(nameof(MonitorProcess), MonitorProcess);
					else
						key.DeleteValue(nameof(MonitorProcess), false); 
				}
			}
		}
	}
}
