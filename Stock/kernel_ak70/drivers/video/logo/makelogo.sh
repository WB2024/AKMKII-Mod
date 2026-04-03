#!/bin/sh
bmptopnm ak201_logo.bmp | pnmtoplainpnm > image480x800.ppm
pnmquant -fs 223 image480x800.ppm > image480x800_256.ppm
pnmnoraw image480x800_256.ppm > logo_ak201_clut224.ppm

