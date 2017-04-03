using System.Windows;
using Zodiacon.WPF;

namespace GflagsX.ViewModels {
	class NewImageViewModel : DialogViewModelBase {
		public NewImageViewModel(Window window) : base(window) {
			CanExecuteOKCommand = () => !string.IsNullOrWhiteSpace(ImageName);
			OKCommand.ObservesProperty(() => ImageName);
		}

		private string _imageeName;

		public string ImageName {
			get { return _imageeName; }
			set { SetProperty(ref _imageeName, value); }
		}

	}
}
