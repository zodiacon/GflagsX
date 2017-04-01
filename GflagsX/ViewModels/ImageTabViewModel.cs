using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using GflagsX.Models;
using Microsoft.Win32;

namespace GflagsX.ViewModels {
	class ImageTabViewModel : GlobalFlagsTabViewModelBase {
		const string IFEOKey = @"Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options";

		public string Text => "Image";

		public string Icon => "/icons/image.ico";

		ObservableCollection<string> _images;

		public IList<string> Images => _images;

		public ImageTabViewModel() : base(GlobalFlagUsage.Image) {
			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey)) {
				_images = new ObservableCollection<string>(key.GetSubKeyNames());
				if(_images.Count > 0)
					SelectedImage = _images[0];
			}
		}

		private string _selectedImage;

		public string SelectedImage {
			get { return _selectedImage; }
			set {
				if(SetProperty(ref _selectedImage, value)) {
					CalculateFlags();
					OnPropertyChanged(nameof(Flags));
				}
			}
		}

		public override void Apply() {
			if(SelectedImage == null)
				return;

			using(var key = Registry.LocalMachine.OpenSubKey(IFEOKey + "\\" + SelectedImage, true)) {
				var value = FlagsValue;
				key.SetValue("GlobalFlag", value, RegistryValueKind.DWord);
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
	}
}
