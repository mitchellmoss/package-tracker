
## Shippo API
For use with Shippo API
 
## Prerequisites

- Qt 6.7.3 or higher
- macOS 14.0 or higher (for macOS builds)
- Xcode Command Line Tools
- Homebrew (recommended for macOS)



# Build, Make, and Run
 make clean && qmake -spec macx-clang package-tracker.pro && make && PackageTracker.app/Contents/MacOS/PackageTracker -platform cocoa    

 
 
 # Create an iconset directory
mkdir app.iconset

# Name your png files according to this convention:
# icon_16x16.png
# icon_16x16@2x.png
# icon_32x32.png
# icon_32x32@2x.png
# icon_128x128.png
# icon_128x128@2x.png
# icon_256x256.png
# icon_256x256@2x.png
# icon_512x512.png
# icon_512x512@2x.png

# Convert the iconset to icns
iconutil -c icns app.iconset

## Generate the iconset from SVG
./generate_icons.sh
