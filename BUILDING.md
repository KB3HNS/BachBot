# Building BachBot from sources

## Dependencies

- [MidiFile](https://github.com/craigsapp/midifile)[^1]
- [WxWidgets](https://www.wxwidgets.org/)
- [RTMidi](https://www.music.mcgill.ca/~gary/rtmidi/)
- [FMT](https://github.com/fmtlib/fmt)
- [wxFormBuilder](https://github.com/wxFormBuilder/wxFormBuilder)
  - Optional: This is for editing the GUI.

_Note:_ These packages may have dependencies of their own.

[^1]: MidiFile is not provided by any of the major library providers and will
likely need to be built manually.  Therefore it has been directly linked as a
submodule in the source code.

### Installing Dependencies : Windows

Dependencies are installed using [vcpkg](https://vcpkg.io/en/index.html).
Assuming that you have vcpkg correctly installed, simply install `wxwidgets`,
`rtmidi`, and `fmt` using the `x64-windows` configuration (see example).  A
successfully tested version of Midifile has been provided.  However, the default
configuration in the repository is for Visual Studio 2015 and x86 whereas the
default target for BachBot is Visual Studio 2019 and x64.  BachBot requires a
compiler with C++17 support or newer.

vcpkg example:

```shell
  vcpkg install fmt:x64-windows
  vcpkg integrate install
```

### Installing Dependencies : Linux

WxWidgets, libfmt and librtmidi are all available in most distribution package
managers.  Be sure to include the -dev (and if desired -doc) counterparts to
ensure that the appropriate headers are available to your compiler of choice.
Alternatively, download the latest version of the libraries, compile, and
install them in a location that is accessible to the compiler (usually under
`/usr/local`).

### Installing Dependencies : MacOS

Building under MacOS will require that a properly licensed version of XCode is
installed along with all tools.  To obtain the dependencies, use [Homebrew][5],
or [Mac Ports][6].  Alternatively build and install the dependencies manually.
**Be aware, wxwidgets 3.1+ requires a very recent version of CMake (3.25)**

[5]: https://brew.sh/
[6]: https://www.macports.org/

## Building

### MS Windows

Building under Windows is done through Visual Studio 2019.  **Do not use the
included CMake file**.

The current CMake recipe does not work for Windows because vcpkg integration
with cmake is far too broken to be useable.  See [here][1], [here][2], and
[here][3] for a few examples.  Furthermore, this would introduce the requirement
for several fixed paths / many optional sections that would be completely
specific to Windows and have no impact on any other platform.  Lastly, after
manually working through the issues and trying the suggested work-arounds, I
found that the compiler is still generating bad/invalid code when calling
functions; specifically when dealing with WxWidget templates.

[1]: https://github.com/microsoft/vcpkg/issues/4756
[2]: https://github.com/microsoft/vcpkg/issues/18066
[3]: https://stackoverflow.com/questions/64838971/setting-toolchain-file-in-visual-studio-code-and-cmake-tool-for-vsc-doesnt-work

### All other configurations

Build using the standard cmake workflow:

```shell
mkdir build
cd build
cmake ..
cmake --build .
```

BachBot has been tested and is known to build with CLang 10 and newer, but
should be able to compile successfully with any compiler supporting C++17 and
newer.
