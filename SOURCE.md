# Corresponding Source — BLAZAR 1.0.0

This Windows VST3 is distributed under AGPLv3. The exact project source, embedded artwork, tests, build instructions and notices are available without charge at:

https://github.com/bjglover/blazar/tree/v1.0.0

The historical tag above does not contain the macOS port. For macOS distributions,
provide a link to the exact source commit or release tag used to build the plugins
alongside the binary download. Follow the [macOS build instructions](README.md#macos)
at that revision; the local signing scripts do not publish source or create a
release tag. Modified builds need their own matching source link as well.

JUCE is a pinned Git submodule at `dependencies/JUCE`: version **8.0.12**, commit **29396c22c93392d6738e021b83196283d6e4d850**. Its source and embedded dependencies, including the VST3 SDK, are available at:

https://github.com/juce-framework/JUCE/tree/29396c22c93392d6738e021b83196283d6e4d850

Direct JUCE source archive:

https://github.com/juce-framework/JUCE/archive/29396c22c93392d6738e021b83196283d6e4d850.zip

Obtain the exact complete sources with:

```powershell
git clone --branch v1.0.0 --recurse-submodules https://github.com/bjglover/blazar.git
cd blazar
./build.ps1 -JucePath "$PWD/dependencies/JUCE"
```

Visual Studio Build Tools with C++ tools, a Windows SDK and CMake 3.22+ are required. They are general-purpose tools/System Libraries and are supplied separately. GitHub's automatic project source ZIP does **not** contain submodule contents: also download the exact JUCE archive above, extract it and pass its directory as -JucePath. No external samples or fonts are needed. The PNG is the available source form of the generated artwork; its provenance is in Assets/ARTWORK.md.

AGPL section 6(d) permits Corresponding Source on another public server with equivalent copying facilities and clear directions beside the binary. These directions accompany both the ZIP and its release page. The distributor remains responsible for maintaining equivalent access to the exact matching sources for as long as required; if the upstream snapshot becomes unavailable, mirror it and update the directions. No dependency snapshot is included in the binary ZIP.

No signing key or installation lock is required to rebuild/install modified plugins. Copy the complete rebuilt Blazar.vst3 bundle as described in README.md. Preserve AGPL and third-party notices on redistribution. If you modify covered software to support remote user interaction, AGPL section 13 also requires a prominent source offer to those users; this plugin does not expose a remote service.
