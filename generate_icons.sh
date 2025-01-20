#!/bin/bash

# Create iconset directory
mkdir -p app.iconset

# Convert SVG to various PNG sizes
for size in 16 32 128 256 512 1024; do
    # Regular resolution
    inkscape -w $size -h $size icon.svg -o "app.iconset/icon_${size}x${size}.png"
    
    # Retina resolution (@2x)
    if [ $size -lt 512 ]; then
        inkscape -w $((size*2)) -h $((size*2)) icon.svg -o "app.iconset/icon_${size}x${size}@2x.png"
    fi
done

# Generate icns file
iconutil -c icns app.iconset

# Move the icns file to the icons directory
mkdir -p icons
mv app.icns icons/ 