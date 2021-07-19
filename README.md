# auto-lcd-font

Automatically creates a C-compatible font header for small LCD-type drawing libraries.

## Prerequisites

It's a pretty basic program, so only has a few requirements.
* `gcc` and `make` for compilation
* `pkg-config` for the library flags
* libfreetype6 library/headers for font manipulation

For Debian, `sudo apt install -y build-essential pkg-config libfreetype6-dev` should take care of everything.

## Building

`make`

## Usage

`./alf <fontfile> <fontsize> [-t]`
* fontfile: path to font file to render.  Should be able to be anything supported by freetype: TTF, OTF, BDF, and others.
* fontsize: desired rendered height in pixels.  Glyph widths are variable based on the font.
* -t: optional flag to transpose the output when blitting is vertical instead of horizontal.

Redirect the output to your desired filename. Some debug info will be output on stderr.

## Example (variable-size)
`./alf /usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf 12 > LibSans12.cpp`

should produce the following output and the cpp file for inclusion in your embedded project

```
Font Info: /usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf
Number of faces:  1
Number of glyphs: 681
Flags:            0x0A59
Fixed Sizes:      0

Set font size to 12px height

Collecting stats
Char BB: top -10 bot 3
Cell Height: 13
Mean Width: 5
Rendering into fixed-height cell.
```

## Example (fixed-size)
Fonts that are already fixed-size can only be output in that same size.

`./alf ucs-fonts/5x8.bdf 23`

will output

```
Font Info: ucs-fonts/5x8.bdf
Number of faces:  1
Number of glyphs: 1427
Flags:            0x0096
Fixed Sizes:      1
    * 5x8
Unable to set size 23 for font.
```

which shows the available fixed sizes.  Rerunning at size 8 works as expected.

`./alf ucs-fonts/5x8.bdf 8 > ucs5x8.cpp`

produces a valid result

```
Font Info: ucs-fonts/5x8.bdf
Number of faces:  1
Number of glyphs: 1427
Flags:            0x0096
Fixed Sizes:      1
    * 5x8

Set font size to 8px height

Collecting stats
Char BB: top -7 bot 1
Cell Height: 8
Mean Width: 5
Rendering into fixed-height cell.
```
