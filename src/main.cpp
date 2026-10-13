#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

// Cardputer-Adv microSD SPI pin map from the project hardware notes.
static constexpr int SD_CS   = 12;
static constexpr int SD_MOSI = 14;
static constexpr int SD_SCLK = 40;
static constexpr int SD_MISO = 39;

bool sdReady = false;
String lastKey = "None";

void drawScreen() {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5Cardputer.Display.setTextSize(1);

    M5Cardputer.Display.setCursor(4, 4);
    M5Cardputer.Display.println("MPX PLAYER");
    M5Cardputer.Display.drawFastHLine(4, 17, 232, TFT_DARKGREY);

    M5Cardputer.Display.setCursor(4, 25);
    M5Cardputer.Display.println("Cardputer-Adv");

    M5Cardputer.Display.setCursor(4, 43);
    M5Cardputer.Display.printf("microSD: %s\n", sdReady ? "OK" : "NG");

    M5Cardputer.Display.setCursor(4, 61);
    M5Cardputer.Display.println("Last key:");

    M5Cardputer.Display.setCursor(4, 75);
    M5Cardputer.Display.println(lastKey);

    M5Cardputer.Display.setCursor(4, 108);
    M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5Cardputer.Display.println("MULTI-FORMAT PLAYER");
}

void listRootToSerial() {
    File root = SD.open("/");
    if (!root) {
        Serial.println("Could not open SD root.");
        return;
    }

    Serial.println("Files in SD root:");
    File file = root.openNextFile();
    while (file) {
        Serial.printf("%s%s\n", file.name(),
                      file.isDirectory() ? "/" : "");
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    Serial.begin(115200);
    delay(300);
    M5Cardputer.Display.setRotation(1);

    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    sdReady = SD.begin(SD_CS, SPI, 25000000);

    Serial.println();
    Serial.println("Cardputer MPX Player - hardware bring-up");
    Serial.printf("microSD: %s\n", sdReady ? "OK" : "NG");

    if (sdReady) {
        listRootToSerial();
    }

    drawScreen();
}

void loop() {
    M5Cardputer.update();
    auto state = M5Cardputer.Keyboard.keysState();

    if (!state.word.empty()) {
        lastKey = state.word.c_str();
        Serial.print("Key: ");
        Serial.println(lastKey);
        drawScreen();
    }

    delay(10);
}
