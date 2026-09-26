#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 || $# -gt 4 ]]; then
  echo "usage: $0 <XeniOS.app> <entitlements.plist> <compat-source.cc> [extra-source.cc]" >&2
  exit 2
fi

app="$1"
entitlements="$2"
compat_source="$3"
compat_sources=("$compat_source")
if [[ $# -eq 4 ]]; then compat_sources+=("$4"); fi
sdk="$(xcrun --sdk iphoneos --show-sdk-path)"
frameworks="$app/Frameworks"
compat="$frameworks/libc.dylib"

xcrun --sdk iphoneos clang++ -std=c++17 -O2 -fPIC -dynamiclib \
  -arch arm64 -miphoneos-version-min=16.3 -isysroot "$sdk" \
  -Wl,-install_name,@rpath/libc.dylib \
  -Xlinker -reexport_library -Xlinker "$sdk/usr/lib/libc++.tbd" \
  "${compat_sources[@]}" -o "$compat"

for image in "$app/XeniOS" "$frameworks/libmetalirconverter.dylib"; do
  install_name_tool -change /usr/lib/libc++.1.dylib @rpath/libc.dylib "$image"
  if ! otool -L "$image" | grep -F '@rpath/libc.dylib'; then
    echo "failed to link compatibility runtime in $image" >&2
    exit 1
  fi
done

required_symbols=(
  '__ZNSt3__122__libcpp_verbose_abortEPKcz'
  '__ZNSt3__18to_charsEPcS0_d'
  '__ZNSt3__18to_charsEPcS0_dNS_12chars_formatE'
  '__ZNSt3__18to_charsEPcS0_dNS_12chars_formatEi'
  '__ZNSt3__18to_charsEPcS0_e'
  '__ZNSt3__18to_charsEPcS0_eNS_12chars_formatE'
  '__ZNSt3__18to_charsEPcS0_eNS_12chars_formatEi'
  '__ZNSt3__18to_charsEPcS0_f'
  '__ZNSt3__18to_charsEPcS0_fNS_12chars_formatE'
  '__ZNSt3__18to_charsEPcS0_fNS_12chars_formatEi'
)
for symbol in "${required_symbols[@]}"; do
  if ! nm -gU "$compat" | grep -F " $symbol" >/dev/null; then
    echo "compatibility runtime does not export $symbol" >&2
    exit 1
  fi
done

codesign --force --sign - "$compat"
codesign --force --sign - "$frameworks/libmetalirconverter.dylib"
codesign --force --sign - --entitlements "$entitlements" "$app"
codesign --verify --deep --strict "$app"
echo 'iPadOS 16 libc++ compatibility runtime installed and signed.'
