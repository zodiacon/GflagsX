#pragma once

//
// A minimal test runner, so the tests need no framework: TEST(Name) { ... } registers a test, CHECK and CHECK_EQ record
// failures (with the file and line) and the test goes on. Main.cpp runs them all (or those whose names contain the
// argument) and returns the number of failed tests, so a script or CI can tell.
//

namespace Tests {
	struct TestCase {
		const char* Name;
		void (*Function)();
	};

	std::vector<TestCase>& Registry();
	void Fail(const char* file, int line, std::string const& message);

	struct Registrar {
		Registrar(const char* name, void (*function)()) {
			Registry().push_back({ name, function });
		}
	};

	// a value as text, for a failed CHECK_EQ
	template<typename T>
	std::string ToText(T const& value) {
		if constexpr (std::is_same_v<T, std::wstring> || std::is_convertible_v<T, std::wstring_view>) {
			std::wstring_view text = value;
			std::string result = "\"";
			for (auto c : text)
				result += c < 128 ? (char)c : '?';
			return result + "\"";
		}
		else if constexpr (std::is_same_v<T, bool>)
			return value ? "true" : "false";
		else if constexpr (std::is_enum_v<T>)
			return std::to_string((long long)value);
		else if constexpr (std::is_arithmetic_v<T>)
			return std::to_string(value);
		else
			return "(value)";
	}
}

#define TEST(name) \
	static void name(); \
	static Tests::Registrar name##_registrar(#name, name); \
	static void name()

#define CHECK(expression) \
	do { if (!(expression)) Tests::Fail(__FILE__, __LINE__, "CHECK(" #expression ")"); } while (false)

#define CHECK_EQ(actual, expected) \
	do { \
		auto&& actual_ = (actual); \
		auto&& expected_ = (expected); \
		if (!(actual_ == expected_)) \
			Tests::Fail(__FILE__, __LINE__, "CHECK_EQ(" #actual ", " #expected "): " + Tests::ToText(actual_) + " != " + Tests::ToText(expected_)); \
	} while (false)
