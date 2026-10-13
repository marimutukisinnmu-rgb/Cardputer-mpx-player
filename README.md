# Cardputer MPX Player

A multi-format media player project for the **M5Stack Cardputer-Adv**, developed with **PlatformIO + Arduino framework**.

**MPX means multi-format playback**, not just MP3.

## Intended format support

| Format | Intended handling | Status |
|---|---|---|
| MP3 | Audio playback | Planned |
| WAV | Audio playback | Planned |
| M4A / AAC / FLAC / OGG | Evaluate decoder and memory compatibility | To be verified |
| MP4 | Video container; video playback or supported audio-track playback must be evaluated separately | Research required |
| WebM | Video container; VP8/VP9/AV1 video is likely beyond practical real-time decoding on this hardware; audio-track support depends on codec/container support | Research required |

Formats are not considered supported until tested on the actual Cardputer-Adv. Container support (MP4/WebM) does not automatically mean every audio/video codec inside the container is supported.

## Hardware notes

The project targets Cardputer-Adv. The current design reference lists these microSD SPI pins:

- CS: GPIO 12
- MOSI: GPIO 14
- SCLK: GPIO 40
- MISO: GPIO 39

The audio path uses the onboard ES8311 codec and I2S. Its initialization should be based on verified Cardputer-Adv / M5Unified examples, not assumed from pin numbers alone.

## Development plan

1. PlatformIO build and hardware bring-up: display, keyboard, microSD.
2. Reliable audio playback: MP3 and WAV first.
3. Library/browser, play/pause, previous/next, volume, repeat and shuffle.
4. Seek/progress display and spectrum visualization where the decoder exposes usable data.
5. Investigate additional audio formats and what is feasible for MP4/WebM on the ESP32-S3's memory and CPU budget.
6. Test and document each format on real hardware.

## Current status

This repository is at the scaffold/bring-up stage. The formats above are goals, not claims of implemented or verified support. See source comments and commit history for implementation status.
