# XeniOS on iPadOS 16.3.1

Experimental compatibility build for the ninth generation iPad and TrollStore.

The build workflow checks out [XeniOS](https://github.com/xenios-jp/XeniOS) at
`57c3ca1400ad80cd0c62995d906e00cef2a95604` (release 2.0.1 build 9730),
applies `patches/XeniOS-iPadOS-16.3.patch`, builds an IPA on macOS, and verifies
the deployment target and TrollStore JIT entitlement.

The patch lowers the deployment target to iOS 16.3, removes the app's dependency
on `quick_exit` (absent on iOS 16.3.1), and adds `get-task-allow` so TrollStore
2.0.12 or newer offers its Launch with JIT command.

Build 9731 still stopped in dyld before app startup on the target iPad. Comparing
its Mach-O bindings against the iOS 16.2 SDK found ten strong imports in the main
app and one in the bundled Metal converter that the older libc++ does not export.
Build 9732 adds `compat/libcxx_ios16_compat.cc`, a small library that reexports
the device libc++ and supplies those missing ABI functions. The fast patch
workflow takes the verified build 9731 IPA, changes the two affected Mach-O
images to link through that library, and signs the resulting IPA. The full
source build remains available as a manual workflow.

On the target iPad, build 9732 opens and TrollStore enables JIT, but a game
stops before graphics initialization. Its `xenia.log` reports that reserving
the contiguous 4.5 GiB guest address space fails. Build 9733 adds Apple's
`com.apple.developer.kernel.extended-virtual-addressing` entitlement to the
TrollStore signature. This is an unverified fix until tested on the device.

This remains experimental. The [XeniOS FAQ](https://xenios.jp/faq) lists iOS 18
on A16 class hardware as its lowest tested baseline. Runtime and game performance
on the ninth generation iPad have not been validated.
