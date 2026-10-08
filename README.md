# BLAZAR

**An experimental physical-modelling synthesizer built from imaginary physics.**

*Musical instruments made from models of physics.*

BLAZAR creates sound from interacting worlds of **MATTER, FIELD, PARTICLES and PLASMA**. Explore glassy pads, strange resonances, evolving atmospheres, unstable textures and sounds that can seem to have a life of their own.

It isn't an emulation of an existing instrument. Pick an object, give it energy, disturb its surroundings—and discover what it wants to become.

> Don't remove instability. Make instability playable.

**[Download BLAZAR v1.0.0 for Windows](https://github.com/bjglover/blazar/releases/tag/v1.0.0)** · Free and open source · Windows 10/11 · 64-bit VST3

macOS users can [build VST3 and Audio Unit plugins from source](#macos). The macOS build supports Apple Silicon and Intel; the v1.0.0 download above is Windows-only.

<!-- Screenshot location: add a real capture of the released v1.0.0 interface here.
Existing local captures show an older RATE dropdown; v1.0.0 uses a rotary control.
Do not use Blazar-GUI-Mockup.png or substitute the standalone artwork for a plugin screenshot. -->

## Four worlds, one instrument

Each world has its own character. The interesting things happen when they meet.

| World | Explore |
| --- | --- |
| **MATTER** | Resonant objects: strings, plates, glass, bells and stranger shapes. Strike, pluck, scrape or sustain them with an exciter. |
| **FIELD** | Harmonic structures, stretched partials and shifting spectral patterns. A place for luminous tones and slowly changing pads. |
| **PARTICLES** | Discrete events: drops, sparks, impacts, chirps and dust. Let them sound directly or use them to excite another world. |
| **PLASMA** | Continuous turbulent energy across 12 models. Explore flowing, stormy and unstable textures—or let the medium drive other worlds. |

Blend them gently for a playable instrument, or increase their interaction and listen for something unexpected. All sound is procedural: no external sample library is needed.

## What does it sound like?

Think glassy pads and ringing objects, strange percussion, drifting atmospheres, granular rustles and textures poised between a note and a weather system.

A patch can be delicate, resonant or unsettled. Hold a chord and let it develop; try a short phrase and hear how the same world responds to a different gesture. Exploration is part of playing it.

## Make it move

The main controls invite you to shape behaviour as well as tone:

| Control | Try this |
| --- | --- |
| **ENERGY** | Push the system towards a more excitable response. |
| **MOTION** | Change the pace of evolution, from a slow drift to more restless movement. |
| **EVOLVE** | Set how much the sound changes as it unfolds. Start low, hold a note, then bring it up. |
| **SPACE** | Let the sound spread through diffusion and echoes. |
| **PULSE / RATE** | Add tempo-related rhythmic breathing and choose its pace. |
| **TENSEGRITY** | Explore coupling between the worlds: how much they influence one another. |

MIDI pitch bend, mod wheel, channel pressure and sustain give you further ways to play the instrument.

## Forty starting points

BLAZAR includes **40 factory programs**. Treat them as instruments to play and starting points to pull apart: change a world, alter an exciter, or see what happens when an apparently quiet layer starts driving something else.

Save your discoveries as user patches and organise them into named banks. Factory programs are embedded; user patches live in:

- Windows: `%APPDATA%\Dog Lab Plugins\Blazar\User Patches`
- macOS: `~/Library/Application Support/Dog Lab Plugins/Blazar/User Patches`

User patches use the same `.blazar.json` format on both platforms. Copy bank folders between these locations to transfer them.

## Download and install

**Requirements:** Windows 10/11, 64-bit, and a VST3 host. BLAZAR is an instrument plugin; no standalone application is included. The release was built and validated on Windows 10 using the project's test host. No formal DAW certification is claimed.

1. [Open the v1.0.0 release](https://github.com/bjglover/blazar/releases/tag/v1.0.0) and download `BLAZAR-1.0.0-Windows-x64-VST3.zip`.
2. Close your DAW and extract the ZIP.
3. Copy the **entire `Blazar.vst3` folder** to `C:\Program Files\Common Files\VST3\`. Keep its `Contents` directory intact. Administrator permission may be needed.
4. Restart your 64-bit VST3 host, rescan plugins, load **Blazar** as an instrument and send it MIDI notes.

If your host reports missing Microsoft C++ runtime DLLs, install the [Microsoft Visual C++ v14 x64 Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).

## Open source and building

Curious about the physics? The source is here to explore, modify and rebuild.

### Windows

For a Windows x64 Release build, install Visual Studio Build Tools with Desktop development with C++, a Windows SDK, CMake 3.22 or newer, and Git. Run these commands in a Visual Studio developer shell:

```powershell
git clone --branch v1.0.0 --recurse-submodules https://github.com/bjglover/blazar.git
cd blazar
./build.ps1 -JucePath "$PWD/dependencies/JUCE"
```

The script builds the plugin and validation tools, then runs CTest. Output: `build-release/Blazar_artefacts/Release/VST3/Blazar.vst3`. Copy the whole bundle when installing.

JUCE **8.0.12** is pinned to commit `29396c22c93392d6738e021b83196283d6e4d850`. See [SOURCE.md](SOURCE.md) for exact corresponding source, submodule/archive instructions and build requirements. Pure DSP checks can also be built without JUCE using `-DQUASAR_DSP_ONLY=ON`.

### macOS

Requires macOS 11 or newer, Xcode Command Line Tools (`xcode-select --install`), CMake 3.22 or newer, and Git. Install CMake so that `cmake` and `ctest` are on your `PATH`. No Projucer project, external audio SDK, samples or fonts are required.

From a checkout containing the macOS port (the original `v1.0.0` tag predates it):

```sh
git submodule update --init --recursive
./build-macos.sh
```

The script builds universal `arm64;x86_64` Release binaries with a macOS 11 deployment target, then runs CTest. For a faster Apple Silicon development build, use `./build-macos.sh -DCMAKE_OSX_ARCHITECTURES=arm64`; Intel-only builds can use `x86_64`. An external JUCE 8.0.12 checkout can be selected with `-DJUCE_LOCAL_SOURCE=/absolute/path/to/JUCE`.

Outputs:

- VST3: `build-macos/Blazar_artefacts/Release/VST3/Blazar.vst3`
- Audio Unit (AUv2): `build-macos/Blazar_artefacts/Release/AU/Blazar.component`

To install, close your DAW and copy the complete bundles into your per-user plugin directories:

```sh
mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3" "$HOME/Library/Audio/Plug-Ins/Components"
ditto build-macos/Blazar_artefacts/Release/VST3/Blazar.vst3 \
    "$HOME/Library/Audio/Plug-Ins/VST3/Blazar.vst3"
ditto build-macos/Blazar_artefacts/Release/AU/Blazar.component \
    "$HOME/Library/Audio/Plug-Ins/Components/Blazar.component"
```

Restart your host and rescan plugins. Use the Audio Unit in Logic Pro or GarageBand, or the VST3 in a compatible host. BLAZAR is an instrument plugin; no standalone application is built. The build ad-hoc signs both bundles for local use; it does not create Developer ID signed or notarized release packages. Distributors with their own signing pipeline can disable this step with `-DBLAZAR_ADHOC_SIGN=OFF`.

CTest exercises the DSP, the built VST3, state recall, MIDI, user patches and editor controls/lifecycle. GUI tests need a logged-in graphical session; use `ctest --test-dir build-macos -C Release --output-on-failure -LE gui` in a headless environment. After installing the Audio Unit, validate its host interface with Apple's tool:

```sh
auval -v aumu Blzr Dglb -strict
```

If a newly installed Audio Unit is not discovered, close your audio hosts and run `killall -9 AudioComponentRegistrar`, then retry validation or reopen the host. This refreshes macOS's component discovery service.

A universal build contains both architectures, but CTest runs the native architecture of the current machine. Validate on Intel and on the oldest supported macOS before publishing a release; a successful build or `auval` run does not establish compatibility with every DAW.

## Licensing

BLAZAR is free and open source under **AGPL-3.0-only**, without warranty. See [LICENSE](LICENSE), [licensing details](LICENSE.md) and [NOTICE](NOTICE).

JUCE 8.0.12 uses its AGPLv3 option. VST3 SDK 3.8.0 and other dependencies retain their compatible original notices.

Made by **Dog Lab Plugins**.
