#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <algorithm>
#include <cstring>

static constexpr int SD_CS = 12;
static constexpr int SD_MOSI = 14;
static constexpr int SD_SCLK = 40;
static constexpr int SD_MISO = 39;
static constexpr uint32_t SD_HZ = 25000000;
static constexpr size_t AUDIO_SAMPLES = 2048;

bool sdReady = false;
bool stopRequested = false;
String statusText = "Booting...";
String wavFiles[32];
size_t wavCount = 0;
size_t selected = 0;
int16_t audioBuffer[AUDIO_SAMPLES];

static uint16_t le16(const uint8_t* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
static uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

void drawScreen() {
    auto& d = M5Cardputer.Display;
    d.fillScreen(TFT_BLACK);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    d.setTextSize(1);
    d.setCursor(4, 4);
    d.println("MPX PLAYER");
    d.drawFastHLine(4, 17, 232, TFT_DARKGREY);
    d.setCursor(4, 22);
    d.printf("microSD: %s   WAV: %u\n", sdReady ? "OK" : "NG",
             static_cast<unsigned>(wavCount));
    d.setCursor(4, 39);
    d.println("WAV browser (PCM 16-bit)");
    if (wavCount == 0) {
        d.setCursor(4, 53);
        d.println("Put .wav files in /music");
    } else {
        const size_t start = selected > 3 ? selected - 3 : 0;
        for (size_t i = start; i < wavCount && i < start + 5; ++i) {
            d.setCursor(4, 53 + int(i - start) * 12);
            d.setTextColor(i == selected ? TFT_CYAN : TFT_WHITE, TFT_BLACK);
            d.printf("%c %s\n", i == selected ? '>' : ' ', wavFiles[i].c_str());
        }
    }
    d.setTextColor(TFT_YELLOW, TFT_BLACK);
    d.setCursor(4, 111);
    d.println("J/K select P play S stop");
    d.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    d.setCursor(4, 123);
    d.println(statusText.substring(0, 38));
}

bool endsWithWav(const char* name) {
    String s(name);
    s.toLowerCase();
    return s.endsWith(".wav");
}

void scanWavFiles() {
    wavCount = 0;
    File musicDir = SD.open("/music");
    if (!musicDir || !musicDir.isDirectory()) {
        if (musicDir) musicDir.close();
        statusText = "Missing /music folder";
        selected = 0;
        return;
    }

    File f = musicDir.openNextFile();
    while (f && wavCount < 32) {
        // Scan only files directly inside /music; do not enter subfolders.
        if (!f.isDirectory() && endsWithWav(f.name())) {
            String name = String(f.name());
            int slash = name.lastIndexOf('/');
            if (slash >= 0) name = name.substring(slash + 1);
            wavFiles[wavCount++] = String("/music/") + name;
        }
        f.close();
        f = musicDir.openNextFile();
    }
    musicDir.close();
    if (selected >= wavCount) selected = wavCount ? wavCount - 1 : 0;
}

bool readExact(File& f, void* dst, size_t n) {
    return f.read(static_cast<uint8_t*>(dst), n) == n;
}

bool findWavData(File& f, uint32_t& sampleRate, uint16_t& channels,
                 uint16_t& bitsPerSample, uint32_t& dataBytes) {
    uint8_t header[12];
    if (!readExact(f, header, sizeof(header)) ||
        memcmp(header, "RIFF", 4) || memcmp(header + 8, "WAVE", 4)) return false;

    bool gotFmt = false, gotData = false;
    while (f.available() >= 8) {
        uint8_t chunk[8];
        if (!readExact(f, chunk, sizeof(chunk))) break;
        uint32_t size = le32(chunk + 4);
        uint32_t payload = f.position();

        if (!memcmp(chunk, "fmt ", 4)) {
            if (size < 16 || size > 4096) return false;
            uint8_t fmt[16];
            if (!readExact(f, fmt, sizeof(fmt))) return false;
            uint16_t format = le16(fmt);
            channels = le16(fmt + 2);
            sampleRate = le32(fmt + 4);
            bitsPerSample = le16(fmt + 14);
            if (format != 1) { statusText = "WAV must be PCM"; return false; }
            gotFmt = true;
        } else if (!memcmp(chunk, "data", 4)) {
            dataBytes = size;
            gotData = true;
            break;
        }
        if (!f.seek(payload + size + (size & 1))) break;
    }
    return gotFmt && gotData;
}

void serviceStopKey() {
    M5Cardputer.update();
    auto state = M5Cardputer.Keyboard.keysState();
    if (!state.word.empty()) {
        String key(state.word.data());
        key.toLowerCase();
        if (key == "s") stopRequested = true;
    }
}

void playSelectedWav() {
    if (!sdReady || wavCount == 0) return;
    stopRequested = false;
    String path = wavFiles[selected];
    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) { statusText = "Cannot open WAV"; drawScreen(); return; }

    uint32_t sampleRate = 0, dataBytes = 0;
    uint16_t channels = 0, bits = 0;
    if (!findWavData(f, sampleRate, channels, bits, dataBytes) ||
        (channels != 1 && channels != 2) || bits != 16 ||
        sampleRate < 8000 || sampleRate > 48000) {
        f.close();
        statusText = "Need PCM16 mono/stereo 8-48kHz";
        drawScreen();
        return;
    }

    statusText = "Playing: " + path;
    drawScreen();
    uint32_t remaining = dataBytes;
    while (remaining >= sizeof(int16_t) && !stopRequested) {
        size_t bytesToRead = std::min<size_t>(AUDIO_SAMPLES * sizeof(int16_t), remaining);
        bytesToRead -= bytesToRead % (channels * sizeof(int16_t));
        size_t bytesRead = f.read(reinterpret_cast<uint8_t*>(audioBuffer), bytesToRead);
        if (bytesRead == 0) break;
        size_t samples = bytesRead / sizeof(int16_t);
        M5Cardputer.Speaker.playRaw(audioBuffer, samples, sampleRate, channels == 2, 1);
        remaining -= bytesRead;
        while (M5Cardputer.Speaker.isPlaying() && !stopRequested) {
            serviceStopKey();
            delay(1);
        }
    }
    M5Cardputer.Speaker.stop();
    f.close();
    statusText = stopRequested ? "Stopped" : "Playback ended";
    drawScreen();
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    Serial.begin(115200);
    delay(300);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Speaker.begin();
    M5Cardputer.Speaker.setVolume(180);
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    sdReady = SD.begin(SD_CS, SPI, SD_HZ);
    if (sdReady) scanWavFiles();
    statusText = sdReady ? "Ready" : "microSD init failed";
    Serial.printf("MPX Player: SD=%s, WAV files=%u\n",
                  sdReady ? "OK" : "NG", static_cast<unsigned>(wavCount));
    drawScreen();
}

void loop() {
    M5Cardputer.update();
    auto state = M5Cardputer.Keyboard.keysState();
    if (!state.word.empty()) {
        String key(state.word.data());
        key.toLowerCase();
        if (key == "j" && wavCount) selected = (selected + 1) % wavCount;
        else if (key == "k" && wavCount) selected = (selected + wavCount - 1) % wavCount;
        else if (key == "p") playSelectedWav();
        else if (key == "r") { scanWavFiles(); statusText = "Rescanned SD"; }
        drawScreen();
    }
    delay(10);
}
