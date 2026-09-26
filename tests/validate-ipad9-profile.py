import pathlib
import re
import subprocess
import tempfile

root = pathlib.Path.cwd()
source = (root / "src/xenia/app/xenia_main_ios.mm").read_text()
start = source.index("static void ApplyIPad9ResourceProfile()")
end = source.index("DEFINE_bool(mount_scratch", start)
declarations = re.findall(r"^DECLARE_\w+\(\w+\);", source[:start], re.M)
fixture = "\n".join([
    '#include "xenia/base/ipad9_cvar_override.h"',
    '#include "xenia/base/ipad9_resource_budget.h"',
    '#include "xenia/base/logging.h"',
    '#include "xenia/gpu/gpu_flags.h"',
    *declarations, source[start:end],
])
sdk = subprocess.check_output(
    ["xcrun", "--sdk", "iphoneos", "--show-sdk-path"], text=True).strip()
with tempfile.TemporaryDirectory() as tmp:
    file = pathlib.Path(tmp) / "profile.cc"
    file.write_text(fixture)
    subprocess.run([
        "xcrun", "--sdk", "iphoneos", "clang++", "-std=c++17", "-arch", "arm64",
        "-miphoneos-version-min=16.3", "-isysroot", sdk,
        "-DXE_IOS_MOLTENVK_ENABLED=1", "-I", str(root / "src"), "-I", str(root),
        "-fsyntax-only", str(file),
    ], check=True)
print("Actual iPad9 profile function compiles with iOS 16 headers and config API")
