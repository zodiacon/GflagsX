using GflagsX.Models;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace GflagsX.Converters {
	class CategoryToStringConverter : IValueConverter {
		public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
			switch((string)value) {
				case "KernelModeOnly":
					return "Kernel Mode";
				case "UserModeOnly":
					return "User Mode";
				default:
					return "General";
			}
		}

		public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) {
			throw new NotImplementedException();
		}
	}
}
