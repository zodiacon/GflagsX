using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using GflagsX.Models;
using GflagsX.Views;
using Microsoft.Win32;
using Prism.Commands;

namespace GflagsX.ViewModels {
	class ImageTabViewModel : GlobalFlagsTabViewModelBase {
		public const string IFEOKey = @"Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options";
		public ProcessMitigationsViewModel Mitigations { get; } = new ProcessMitigationsViewModel();

		public string Text => "Image";

		public string Icon => "/icons/image.ico";

		ObservableCollection<string> _images;

		public IList<string> Images => _images;

		public ImageTabViewModel() : base(GlobalFlagUsage.Image) {
			RefreshAll();
		}

		private string _selectedImage;

		public string SelectedImage {
			get { return _selectedImage; }
			set {
				if(SetProperty(ref _selectedImage, value)) {
					CalculateFlags();
					RaisePropertyChanged(nameof(Flags));
					ReloadSettings();
				}
			}
		}

		public override void Apply() {
			if(SelectedImage == null)
				return;

			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey + "\\" + SelectedImage, true)) {
				var value = FlagsValue;
				key.SetValue("GlobalFlag", value, RegistryValueKind.DWord);

				if (!Flags.Where(f => f.IsEnabled).Any(f => f.Flag.Value == 0x200)) {
					// no silent process exit, remove key
					Registry.LocalMachine.DeleteSubKey(SilentProcessExitViewModel.SilentProcessExitKey + "\\" + SelectedImage, false);
				}

			}
		}

		protected override void CalculateFlags() {
			if(SelectedImage == null)
				return;

			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey + "\\" + SelectedImage)) {
				var value = key.GetValue("GlobalFlag");
				var ntGlobalFlags = value == null ? 0 : (int)value;
				foreach(var vm in Flags) {
					vm.IsEnabled = (ntGlobalFlags & vm.Flag.Value) == vm.Flag.Value;
				}
			}
		}

		public override Visibility SilentProcessExitVisible => Visibility.Visible;

		public ICommand NewImageCommand => new DelegateCommand(() => {
			var vm = App.MainViewModel.UI.DialogService.CreateDialog<NewImageViewModel, NewImageView>();
			if(vm.ShowDialog() == true) {
				if(Images.Contains(vm.ImageName, StringComparer.InvariantCultureIgnoreCase)) {
					App.MainViewModel.UI.MessageBoxService.ShowMessage("Image name already exists.", Constants.AppName);
				}
				else {
					// add to registry and the list
					using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey, true)) {
						key.CreateSubKey(vm.ImageName);
					}
					var list = _images.ToList();
					list.Add(vm.ImageName);
					_images.Insert(list.IndexOf(vm.ImageName), vm.ImageName);
					SelectedImage = vm.ImageName;
				}
			}
		});

		public ICommand DeleteImageCommand => new DelegateCommand(() => {
			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey, true)) {
				key.DeleteSubKey(SelectedImage);
			}
			int index = _images.IndexOf(SelectedImage);
			Debug.Assert(index >= 0);
			_images.RemoveAt(index);
			if(_images.Count > 0)
				SelectedImage = _images[index - 1];
		}, () => SelectedImage != null).ObservesProperty(() => SelectedImage);

		public ICommand RefreshAllCommand => new DelegateCommand(() => RefreshAll());

		private void RefreshAll() {
			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey)) {
				_images = new ObservableCollection<string>(key.GetSubKeyNames());
				if(_images.Count > 0)
					SelectedImage = _images[0];
				RaisePropertyChanged(nameof(Images));
			}
		}

		private string _debuggerName;

		public string DebuggerName {
			get { return _debuggerName; }
			set { SetProperty(ref _debuggerName, value); }
		}

		public ICommand ApplySettingsCommand => new DelegateCommand(() => ApplySettings(), () => SelectedImage != null)
			.ObservesProperty(() => SelectedImage);

		private void ApplySettings() {
			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey + "\\" + SelectedImage, true)) {
				if(string.IsNullOrWhiteSpace(DebuggerName))
					key.DeleteValue("Debugger", false);
				else
					key.SetValue("Debugger", DebuggerName);
				var mitigations = Mitigations.Apply();
				if (mitigations == 0)
					key.DeleteValue("MitigationOptions", false);
				else
					key.SetValue("MitigationOptions", mitigations, RegistryValueKind.QWord);
			}
		}

		public ICommand ReloadSettingsCommand => new DelegateCommand(() => ReloadSettings(), () => SelectedImage != null)
			.ObservesProperty(() => SelectedImage);

		private void ReloadSettings() {
			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey + "\\" + SelectedImage)) {
				DebuggerName = (key.GetValue("Debugger") as string) ?? string.Empty;

				// read process mitigations

				object value;
				ulong mitigations = (value = key.GetValue("MitigationOptions")) == null ? 0 : Convert.ToUInt64(value);
				Mitigations.MitigationsValue = mitigations;
			}
		}

		public DelegateCommandBase ConfigSilentProcessExitCommand => new DelegateCommand(() => {
			var vm = App.MainViewModel.UI.DialogService.CreateDialog<SilentProcessExitViewModel, SilentProcessExitView>(SelectedImage);
			if (vm.ShowDialog() == true) {
				vm.SaveSettings();
				Flags.First(f => f.Flag.Value == 0x200).IsEnabled = true;
			}
		});
	}
}
