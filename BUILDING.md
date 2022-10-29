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
`rtmidi`, and `fmt` using the `x64-windows` configuration.  A successfully tested
version of Midifile has been provided.  However, the default configuration in
the repository is for Visual Studio 2015 and x86 whereas the default target for
BachBot is Visual Studio 2019 and x64.  BachBot requires a compiler with C++17
support or newer.

### Installing Dependencies : Linux

WxWidgets, libfmt and librtmidi are all available in most distribution package
managers.  Be sure to include the -dev (and if desired -doc) counterparts to
ensure that the appropriate headers are available to your compiler of choice.
Alternatively, download the latest version of the libraries, compile, and
install them in a location that is accessible to the compiler (usually under
`/usr/local`).

## Building

### MS Windows

Building under Windows is done through Visual Studio 2019.
