# Cardputer MPX Player

A multi-format media player project for the **M5Stack Cardputer-Adv**, developed with **PlatformIO + Arduino framework**.

**MPX means multi-format playback**, not just MP3.

## Current implementation

- Cardputer-Adv display and keyboard initialization.
- microSD initialization using the project pin map.
- Root-directory WAV browser (up to 32 files).
- Initial WAV playback path using M5Unified's speaker API.

### WAV compatibility in this milestone

Only uncompressed PCM WAV, 16-bit, mono or stereo, 8–48 kHz is accepted. Put WAV files in the microSD root. Controls: **J** next, **K** previous, **P** play, **S** stop during playback, **R** rescan. The initial chunk-based playback implementation may have audible gaps between chunks and needs hardware validation.

## Roadmap and priority

1. **Audio first:** verify WAV playback and fix any hardware/API issues on the actual Cardputer-Adv.
2. Add MP3 decoding and robust play/pause/stop/volume controls.
3. Improve buffering so audio is continuous and responsive.
4. Add library navigation, progress/seek, repeat/shuffle.
5. Only after audio is stable, investigate video playback. Target policy: preserve the source frame rate up to 24 FPS; sources above 24 FPS are sampled down to a maximum of 24 FPS.
6. For video experiments, evaluate a PC-converted RGB565 RAW format to avoid on-device video decoding.

## Hardware notes

The project design notes list these microSD SPI pins:

- CS: GPIO 12
- MOSI: GPIO 14
- SCLK: GPIO 40
- MISO: GPIO 39

The audio path uses the onboard ES8311 codec and I2S through M5Unified. Hardware initialization and playback behavior must be validated on a real Cardputer-Adv.

## Planned format support

| Format | Intended handling | Status |
|---|---|---|
| WAV | PCM playback | Initial implementation; not hardware-tested |
| MP3 | Decode and playback | Planned |
| M4A / AAC / FLAC / OGG | Evaluate decoder and memory compatibility | To be verified |
| MP4 / WebM | Video/container feasibility research | Research required |

Formats are not considered supported until tested on the actual Cardputer-Adv. Container support does not automatically imply support for every codec inside the container.

## Build notes

The current PlatformIO environment still uses esp32-s3-devkitc-1 as a provisional board target. Confirm/adjust the board definition and library versions against the actual Cardputer-Adv before treating a successful compile as proof of hardware compatibility.
