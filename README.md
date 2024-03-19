# ESPCM

ESPCM is a series of proprietary audio compression codecs developed by ESS Technology Inc. and used in their ESS AudioDrive line of sound cards. Until recently, not much was known about ESPCM and no efforts had been done to reverse-engineer it. That has since changed.

This repository implements an encoder and decoder for ESPCM WAV (.aud) files. You can use it to, for example, encode audio samples for playback on real hardware, or to decode files you have recorded back in the day.

NOTE: This was a quick 2-day project. The code is based on my SSDPCM encoder and many parts of the code are taken from there. So if you take a look at the code, you'll see many references to it.

## Dependencies

For compiling, you'll need **GNU Make.**

No external libraries are needed.

## Usage

#### To encode a file:

`./espcm_encoder espcm4 input_file.wav output_file.aud`

The input must be an 8-bit or 16-bit PCM WAV file, and it must be in mono.

Substitute `espcm4` for the name of the mode you want to use; the following modes are available:

- `espcm4` - 4-bit ESPCM (actual bitrate: 4.21 bits/sample)
- `espcm3` - 3-bit ESPCM (actual bitrate: 3.37 bits/sample)
- `espcm1` - 1-bit ESPCM (actual bitrate: 1.26 bits/sample)

#### To decode a file:

`./espcm_encoder decode encoded_file.aud decoded_file.wav`

The mode will be automatically determined from the file header and does not need to be specified.

The output will be an 8-bit WAV file.
