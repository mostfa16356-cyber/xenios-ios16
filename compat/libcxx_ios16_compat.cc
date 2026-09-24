// Backfill libc++ exports emitted by Xcode 26 that are absent on iPadOS 16.
// This library reexports the device's libc++ and provides only the missing ABI.
#include <charconv>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <type_traits>

namespace {

template <typename T>
bool SameBits(T a, T b) {
  return std::memcmp(&a, &b, sizeof(T)) == 0;
}

template <typename T>
bool RoundTrips(const char* text, T original) {
  char* end = nullptr;
  T parsed;
  if constexpr (std::is_same_v<T, float>) {
    parsed = std::strtof(text, &end);
  } else if constexpr (std::is_same_v<T, double>) {
    parsed = std::strtod(text, &end);
  } else {
    parsed = std::strtold(text, &end);
  }
  return end && *end == '\0' && SameBits(parsed, original);
}

template <typename T>
std::to_chars_result Convert(char* first, char* last, T value,
                             std::chars_format format, int precision,
                             bool shortest) {
  if (first > last) {
    return {last, std::errc::value_too_large};
  }
  char buffer[4096];
  int length = 0;
  if (std::isnan(value)) {
    length = std::snprintf(buffer, sizeof(buffer), "%snan",
                           std::signbit(value) ? "-" : "");
  } else if (std::isinf(value)) {
    length = std::snprintf(buffer, sizeof(buffer), "%sinf",
                           std::signbit(value) ? "-" : "");
  } else {
    const bool is_long_double = std::is_same_v<T, long double>;
    const char* specifier = nullptr;
    switch (format) {
      case std::chars_format::fixed:
        specifier = is_long_double ? "%.*Lf" : "%.*f";
        break;
      case std::chars_format::scientific:
        specifier = is_long_double ? "%.*Le" : "%.*e";
        break;
      case std::chars_format::hex:
        specifier = is_long_double ? "%.*La" : "%.*a";
        break;
      default:
        specifier = is_long_double ? "%.*Lg" : "%.*g";
        break;
    }
    if (precision < 0) {
      precision = std::numeric_limits<T>::max_digits10;
    }
    if (precision > 3000) {
      return {last, std::errc::value_too_large};
    }
    if (shortest && format == std::chars_format::general) {
      for (int digits = 1; digits <= std::numeric_limits<T>::max_digits10;
           ++digits) {
        if constexpr (std::is_same_v<T, long double>) {
          length = std::snprintf(buffer, sizeof(buffer), specifier, digits, value);
        } else {
          length = std::snprintf(buffer, sizeof(buffer), specifier, digits,
                                 static_cast<double>(value));
        }
        if (length < 0 || length >= static_cast<int>(sizeof(buffer)) ||
            RoundTrips(buffer, value)) {
          break;
        }
      }
    } else if constexpr (std::is_same_v<T, long double>) {
      length = std::snprintf(buffer, sizeof(buffer), specifier, precision, value);
    } else {
      length = std::snprintf(buffer, sizeof(buffer), specifier, precision,
                             static_cast<double>(value));
    }
    if (format == std::chars_format::hex && length > 0) {
      const int prefix = buffer[0] == '-' ? 1 : 0;
      if (buffer[prefix] == '0' && buffer[prefix + 1] == 'x') {
        std::memmove(buffer + prefix, buffer + prefix + 2,
                     static_cast<size_t>(length - prefix - 1));
        length -= 2;
      }
    }
  }
  if (length < 0 || length >= static_cast<int>(sizeof(buffer)) ||
      length > last - first) {
    return {last, std::errc::value_too_large};
  }
  std::memcpy(first, buffer, static_cast<size_t>(length));
  return {first + length, std::errc{}};
}

}  // namespace

namespace std {
inline namespace __1 {

[[noreturn]] void __libcpp_verbose_abort(const char* format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
  std::abort();
}

#define DEFINE_TO_CHARS(TYPE)                                                 \
  to_chars_result to_chars(char* first, char* last, TYPE value) {            \
    return Convert(first, last, value, chars_format::general, -1, true);     \
  }                                                                          \
  to_chars_result to_chars(char* first, char* last, TYPE value,             \
                           chars_format format) {                           \
    return Convert(first, last, value, format, -1, true);                    \
  }                                                                          \
  to_chars_result to_chars(char* first, char* last, TYPE value,             \
                           chars_format format, int precision) {            \
    return Convert(first, last, value, format, precision, false);            \
  }

DEFINE_TO_CHARS(float)
DEFINE_TO_CHARS(double)
DEFINE_TO_CHARS(long double)

#undef DEFINE_TO_CHARS

}  // inline namespace __1
}  // namespace std
