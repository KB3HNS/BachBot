# Change Log

## 0.3.0 "Pentecost"

This represents a set of changes made from my first outing.  The next update
should be in November after I've had a few runs with this update in both
September, October, and possibly early November.

* _Major_ overhaul of the UI:
  * Added window texture and modified some colors from their "plain" windows
    3.1-esque appearance
  * Improved dialog boxes for import and Drag-n-Drop
  * Default window size is now actually useable
  * More buttons (send immediate prev/next/cancel)
  * Visual feedback between current playing configuration and next song
    configuration
  * Added highlights for next song, currently playing, and selected song
  * About window actually does something now
  * Bank/Mode renamed to Memory and General Piston Position
  * Memory and General Piston Position are now displayed in the order that they
    appear on the Syndyne console
  * "Configure Song" dialog box closes when song becomes "locked" by the player
    logic
* Preliminary "runway" to port to Linux and, eventually macOS
* Playlist automatically loops
* Multiple bug fixes:
  * **The `Go to mode 0` bug has been identified and fixed!**
  * "Configure" button becomes re-enabled when altering the song selection

### Known issues

* Current Accelerator strategy is incompatible with GTK
* The progress bar does not seem to work right during import

### Additional release notes

* `wood.png` must be manually copied to the output folder
* `midifile` library solution component still requires rebuilding for _Visual
  Studio 2019_ - changes have not been committed[^1].

[^1]: I would fork the project into my GitHub workspace, but I will be changing
      to CMake soon so there _should_ be no need for the outdated solution files.

## 0.2.0 "Ascension"

Ascension release is for my inaugural outing with the organ scheduled for
Monday, 20June22.

* Implemented Playlist
* Implemented Drag & Drop
* Many, many UI improvements and bug fixes.
* Minor adjustments to sequential note timing based on JS Bach's Toccata and
  Fugue in D Minor
* Moved all code to a namespace (well, 2 actually)

## 0.1.0 "Maundy Thursday"

Initial release for Maundy Thursday, Good Friday, and Easter services.

* Most basic playback functionality only
* Intra-song bank change supported
* Bank / Memory reporting
