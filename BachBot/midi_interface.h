/**
 * @file midi_interface.h
 * @brief Include for 3rd party midi library dependencies.
 * @copyright
 * 2022 Andrew Buettner (ABi)
 *
 * @section LICENSE
 *
 * BachBot - A hymn Midi player for Schlicker organs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 * Another downside with vcpkg is that it doesn't put headers in a consistent
 * location with respect to other layouts (eg Linux, MacOS).  `midifile` and
 * `RtMidi` don't have a handy CMake `find_package` like `WxWidgets` does, so
 * this abstracts the different paths that their includes exist in to allow
 * BachBot to compile cross-platform.
 */

#pragma once

#ifdef _WIN32
#include <RtMidi.h>
#include <MidiEvent.h>
#include <MidiFile.h>
#else
#include <rtmidi/RtMidi.h>
#include <midifile/MidiEvent.h>
#include <midifile/MidiFile.h>
#endif
