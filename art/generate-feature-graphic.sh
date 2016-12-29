#!/bin/bash

echo "Generating feature graphics to ~/termux-icons/termux-feature-graphic.png..."
mkdir -p ~/termux-icons/
rsvg-convert feature-graphic.svg > ~/termux-icons/feature-graphic.png
