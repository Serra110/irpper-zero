#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Pin config, change this if you have different pins, the PCB follows this config
// oled:
#define PIN_OLED_SDA 0
#define PIN_OLED_SCL 12
// sd card:
#define PIN_SD_CS 1 //CD
#define PIN_SD_SCK 2 //CLK
#define PIN_SD_MOSI 3 //CMD
#define PIN_SD_MISO 4 //DAT0, miso is a soup for me
#define PIN_SD_DET 5 
// switches:
#define PIN_SW1 6  // Menu/Select
#define PIN_SW2 7  // Menu/Back
#define PIN_SW3 8  // Player 1 Up
#define PIN_SW4 9  // Player 1 Down
#define PIN_SW5 10 // Player 2 Up
#define PIN_SW6 11 // Player 2 Down
//misc
#define buzzer 15
// swtich 7 is the reset button, it is not connected to the microcontroller, but to the reset pin of the microcontroller, so it will reset the microcontroller when pressed
// IR stuff:
#define PIN_IR_RX 13 // thing that reads IR signals
#define PIN_IR_TX 14 // thing that sends IR signals

Adafruit_SSD1306 display(128, 64, &Wire, -1); // -1 is no reset
const uint8_t buttonPins[] = {PIN_SW1, PIN_SW2, PIN_SW3, PIN_SW4, PIN_SW5, PIN_SW6};

enum ButtonIndex {
    BTN_SELECT = 0,
    BTN_BACK = 1,
    BTN_P1_UP = 2,
    BTN_P1_DOWN = 3,
    BTN_P2_UP = 4,
    BTN_P2_DOWN = 5
};

enum MenuItem {
    MENU_LISTEN_IR = 0,
    MENU_LEARN_IR,
    MENU_SEND_IR,
    MENU_GAME_PONG,
    MENU_GAME_DUEL,
    MENU_SD_STATUS,
    MENU_COUNT
};

int currentMenu = 0;

constexpr size_t IR_MAX_SAMPLES = 400;
uint16_t irSamples[IR_MAX_SAMPLES];

bool sdReady = false;
uint8_t lastButtonStates[6] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

void showMessage(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(line1);
    if (line2) {
        display.println(line2);
    }
    if (line3) {
        display.println(line3);
    }
    display.display();
}

void drawHomeScreen() {
    const char* menu[MENU_COUNT] = {
        "Listen IR",
        "Learn IR",
        "Send IR",
        "Game: Pong 2P",
        "Game: Duel",
        "SD status"
    };

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("IRPER menu"));
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == currentMenu) {
            display.print(F(">"));
        } else {
            display.print(F(" "));
        }
        display.println(menu[i]);
    }
    display.println(F("SW1 OK SW2 Back"));
    display.display();
}

int readButtonEdge() {
    for (int i = 0; i < 6; i++) {
        uint8_t state = digitalRead(buttonPins[i]);
        if (lastButtonStates[i] == HIGH && state == LOW) {
            lastButtonStates[i] = state;
            delay(20);
            return i;
        }
        lastButtonStates[i] = state;
    }
    return -1;
}

int waitForButtonPress(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        int btn = readButtonEdge();
        if (btn >= 0) {
            return btn;
        }
    }
    return -1;
}

uint16_t estimateSignalFrequency(const uint16_t* data, size_t count) {
    if (count < 2) {
        return 0;
    }

    uint32_t sum = 0;
    uint16_t used = 0;
    for (size_t i = 0; i < count; i++) {
        if (data[i] > 10 && data[i] < 20000) {
            sum += data[i];
            used++;
        }
    }

    if (used == 0) {
        return 0;
    }

    uint32_t avg = sum / used;
    if (avg == 0) {
        return 0;
    }

    return static_cast<uint16_t>(1000000UL / avg);
}

void drawIrWaveform(const uint16_t* data, size_t count, uint16_t freq) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("IR SIGNAL"));
    display.print(F("freq: "));
    display.print(freq);
    display.println(F(" Hz"));

    const uint8_t baseline = 48;
    for (int x = 0; x < 128; x++) {
        size_t idx = (x * count) / 128;
        uint16_t level = data[idx] / 40;
        if (level > 20) {
            level = 20;
        }
        int y = baseline - level;
        if (y < 0) {
            y = 0;
        }
        if (y > 47) {
            y = 47;
        }
        display.drawLine(x, baseline, x, y, SSD1306_WHITE);
    }

    display.drawFastHLine(0, baseline, 128, SSD1306_WHITE);
    display.display();
}

bool isButtonPressed(int index) {
    return digitalRead(buttonPins[index]) == LOW;
}

void ensureSdFolders() {
    if (!sdReady) {
        return;
    }
    if (!SD.exists("/ir")) {
        SD.mkdir("/ir");
    }
}

void irMark38k(uint16_t usec) {
    uint32_t start = micros();
    while ((micros() - start) < usec) {
        digitalWrite(PIN_IR_TX, HIGH);
        delayMicroseconds(13);
        digitalWrite(PIN_IR_TX, LOW);
        delayMicroseconds(13);
    }
}

void irSpace(uint16_t usec) {
    digitalWrite(PIN_IR_TX, LOW);
    delayMicroseconds(usec);
}

size_t captureRawIr(uint16_t* out, size_t maxCount, bool* startsLow, uint32_t timeoutMs) {
    uint32_t startMs = millis();
    while (millis() - startMs < timeoutMs && digitalRead(PIN_IR_RX) == HIGH) {
       
    }
    if (digitalRead(PIN_IR_RX) == HIGH) {
        return 0;
    }

    int level = digitalRead(PIN_IR_RX);
    *startsLow = (level == LOW);
    uint32_t lastChange = micros();
    size_t count = 0;

    while (count < maxCount) {
        int now = digitalRead(PIN_IR_RX);
        uint32_t nowUs = micros();
        if (now != level) {
            out[count++] = static_cast<uint16_t>(nowUs - lastChange);
            lastChange = nowUs;
            level = now;
        }

        if (count > 10 && (nowUs - lastChange) > 15000) {
            out[count++] = static_cast<uint16_t>(nowUs - lastChange);
            break;
        }
    }
    return count;
}

bool saveIrSlot(uint8_t slot, const uint16_t* data, size_t count, bool startsLow) {
    if (!sdReady || slot > 5 || count == 0) {
        return false;
    }

    char path[20];
    snprintf(path, sizeof(path), "/ir/slot%u.txt", slot + 1);

    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        return false;
    }

    f.seek(0);
    f.truncate(0);
    f.println(F("IRPER_RAW_V1"));
    f.println(startsLow ? 1 : 0);
    f.println(static_cast<uint32_t>(count));
    for (size_t i = 0; i < count; i++) {
        f.println(data[i]);
    }
    f.close();
    return true;
}

bool loadIrSlot(uint8_t slot, uint16_t* data, size_t maxCount, size_t* count, bool* startsLow) {
    if (!sdReady || slot > 5) {
        return false;
    }

    char path[20];
    snprintf(path, sizeof(path), "/ir/slot%u.txt", slot + 1);
    File f = SD.open(path, FILE_READ);
    if (!f) {
        return false;
    }

    String header = f.readStringUntil('\n');
    header.trim();
    if (header != "IRPER_RAW_V1") {
        f.close();
        return false;
    }

    *startsLow = (f.parseInt() == 1);
    uint32_t n = static_cast<uint32_t>(f.parseInt());
    if (n == 0 || n > maxCount) {
        f.close();
        return false;
    }

    for (uint32_t i = 0; i < n; i++) {
        uint32_t v = static_cast<uint32_t>(f.parseInt());
        if (v == 0 || v > 65000) {
            f.close();
            return false;
        }
        data[i] = static_cast<uint16_t>(v);
    }

    *count = n;
    f.close();
    return true;
}

void sendRawIr(const uint16_t* data, size_t count, bool startsLow) {
    bool isMark = startsLow;
    for (size_t i = 0; i < count; i++) {
        if (isMark) {
            irMark38k(data[i]);
        } else {
            irSpace(data[i]);
        }
        isMark = !isMark;
    }
    digitalWrite(PIN_IR_TX, LOW);
}

int selectSlotScreen(const char* title) {
    int slot = 0;

    while (true) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(title);
        display.println(F("SW3/SW4 change"));
        display.println(F("SW1 confirm"));
        display.println(F("SW2 cancel"));
        display.print(F("Slot: "));
        display.println(slot + 1);
        display.display();

        int btn = waitForButtonPress(30000);
        if (btn < 0) {
            return -1;
        }

        if (btn == BTN_BACK) {
            return -1;
        }
        if (btn == BTN_SELECT) {
            return slot;
        }
        if (btn == BTN_P1_UP) {
            slot = (slot + 5) % 6;
        }
        if (btn == BTN_P1_DOWN) {
            slot = (slot + 1) % 6;
        }
    }
}

void drawIdleListenScreen() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("IR listening"));
    display.println(F("Waiting..."));
    display.drawFastHLine(0, 48, 128, SSD1306_WHITE);
    display.display();
}

void flowListenIrAuto() {
    drawIdleListenScreen();

    while (true) {
        int btn = readButtonEdge();
        if (btn == BTN_BACK) {
            drawHomeScreen();
            return;
        }

        if (digitalRead(PIN_IR_RX) == LOW) {
            uint32_t start = millis();
            while ((millis() - start) < 50 && digitalRead(PIN_IR_RX) == LOW) {
                delay(1);
            }

            if ((millis() - start) >= 10) {
                bool startsLow = true;
                size_t count = captureRawIr(irSamples, IR_MAX_SAMPLES, &startsLow, 200);
                if (count > 12) {
                    uint16_t freq = estimateSignalFrequency(irSamples, count);
                    drawIrWaveform(irSamples, count, freq);

                    while (true) {
                        int action = readButtonEdge();
                        if (action == BTN_SELECT) {
                            int slot = selectSlotScreen("Save signal");
                            if (slot >= 0 && saveIrSlot(slot, irSamples, count, startsLow)) {
                                char line[22];
                                snprintf(line, sizeof(line), "Saved slot %d", slot + 1);
                                showMessage("IR saved", line);
                                delay(1200);
                                drawHomeScreen();
                                return;
                            }
                            showMessage("Save failed", "Try again");
                            delay(1000);
                            drawHomeScreen();
                            return;
                        }
                        if (action == BTN_BACK) {
                            drawHomeScreen();
                            return;
                        }
                        delay(20);
                    }
                }
            }
        }

        drawIdleListenScreen();
        delay(20);
    }
}

void flowLearnIr() {
    if (!sdReady) {
        showMessage("SD not ready", "Insert SD card");
        delay(1000);
        return;
    }

    int slot = selectSlotScreen("Learn IR slot");
    if (slot < 0) {
        showMessage("Cancelled", "Back to menu");
        delay(1000);
        return;
    }

    showMessage("Point remote", "Sending data now", "Capturing...");
    bool startsLow = true;
    size_t captured = captureRawIr(irSamples, IR_MAX_SAMPLES, &startsLow, 10000);
    if (captured == 0) {
        showMessage("No IR detected", "Try again");
        delay(1200);
        return;
    }

    if (saveIrSlot(slot, irSamples, captured, startsLow)) {
        char line[22];
        snprintf(line, sizeof(line), "Saved to slot %d", slot + 1);
        showMessage("IR learned", line);
    } else {
        showMessage("Save failed", "Check SD card");
    }
    delay(1200);
}

void flowSendIr() {
    if (!sdReady) {
        showMessage("SD not ready", "Insert SD card");
        delay(1000);
        return;
    }

    int slot = selectSlotScreen("Send IR slot");
    if (slot < 0) {
        showMessage("Cancelled", "Back to menu");
        delay(1000);
        return;
    }

    bool startsLow = true;
    size_t count = 0;
    if (!loadIrSlot(slot, irSamples, IR_MAX_SAMPLES, &count, &startsLow)) {
        showMessage("Slot empty/invalid", "Learn first");
        delay(1200);
        return;
    }

    showMessage("Sending IR...", "Keep device aimed");
    sendRawIr(irSamples, count, startsLow);
    showMessage("Done", "IR signal sent");
    delay(800);
}

void gamePong2P() {
    int p1Y = 24;
    int p2Y = 24;
    int ballX = 64;
    int ballY = 32;
    int vx = 2;
    int vy = 1;
    int score1 = 0;
    int score2 = 0;
    uint32_t lastFrame = millis();

    showMessage("Pong 2P", "SW3/SW4 left", "SW5/SW6 right");
    delay(700);

    while (true) {
        if (isButtonPressed(BTN_P1_UP)) {
            p1Y--;
        }
        if (isButtonPressed(BTN_P1_DOWN)) {
            p1Y++;
        }
        if (isButtonPressed(BTN_P2_UP)) {
            p2Y--;
        }
        if (isButtonPressed(BTN_P2_DOWN)) {
            p2Y++;
        }

        p1Y = constrain(p1Y, 0, 48);
        p2Y = constrain(p2Y, 0, 48);

        int btn = readButtonEdge();
        if (btn == BTN_BACK) {
            break;
        }

        if (millis() - lastFrame >= 35) {
            lastFrame = millis();
            ballX += vx;
            ballY += vy;

            if (ballY <= 0 || ballY >= 63) {
                vy = -vy;
            }

            if (ballX <= 4 && ballY >= p1Y && ballY <= p1Y + 15) {
                vx = abs(vx);
            }
            if (ballX >= 123 && ballY >= p2Y && ballY <= p2Y + 15) {
                vx = -abs(vx);
            }

            if (ballX < 0) {
                score2++;
                ballX = 64;
                ballY = 32;
                vx = 2;
                vy = (random(0, 2) == 0) ? 1 : -1;
            }
            if (ballX > 127) {
                score1++;
                ballX = 64;
                ballY = 32;
                vx = -2;
                vy = (random(0, 2) == 0) ? 1 : -1;
            }

            display.clearDisplay();
            display.setCursor(0, 0);
            display.print(score1);
            display.print(F(" - "));
            display.println(score2);
            display.drawFastVLine(2, p1Y, 16, SSD1306_WHITE);
            display.drawFastVLine(125, p2Y, 16, SSD1306_WHITE);
            display.fillRect(ballX, ballY, 2, 2, SSD1306_WHITE);
            display.display();

            if (score1 >= 5 || score2 >= 5) {
                if (score1 > score2) {
                    showMessage("P1 wins", "SW1 continue");
                } else {
                    showMessage("P2 wins", "SW1 continue");
                }
                while (true) {
                    int endBtn = waitForButtonPress(20000);
                    if (endBtn == BTN_SELECT || endBtn == BTN_BACK || endBtn < 0) {
                        return;
                    }
                }
            }
        }
    }
}

void gameDuel() {
    int p1Y = 60;
    int p2Y = 60;

    showMessage("Duel climb", "Reach top first", "SW2 exits");
    delay(700);

    while (true) {
        int btn = readButtonEdge();
        if (btn == BTN_BACK) {
            break;
        }
        if (btn == BTN_P1_UP) {
            p1Y -= 3;
        }
        if (btn == BTN_P1_DOWN) {
            p1Y += 2;
        }
        if (btn == BTN_P2_UP) {
            p2Y -= 3;
        }
        if (btn == BTN_P2_DOWN) {
            p2Y += 2;
        }

        p1Y = constrain(p1Y, 0, 63);
        p2Y = constrain(p2Y, 0, 63);

        display.clearDisplay();
        display.drawFastHLine(0, 0, 128, SSD1306_WHITE);
        display.fillRect(24, p1Y, 8, 3, SSD1306_WHITE);
        display.fillRect(96, p2Y, 8, 3, SSD1306_WHITE);
        display.setCursor(0, 56);
        display.println(F("P1:SW3/4 P2:SW5/6"));
        display.display();

        if (p1Y <= 0 || p2Y <= 0) {
            if (p1Y <= 0 && p2Y <= 0) {
                showMessage("Draw", "Both reached top");
            } else if (p1Y <= 0) {
                showMessage("P1 wins", "SW1 continue");
            } else {
                showMessage("P2 wins", "SW1 continue");
            }
            while (true) {
                int endBtn = waitForButtonPress(20000);
                if (endBtn == BTN_SELECT || endBtn == BTN_BACK || endBtn < 0) {
                    return;
                }
            }
        }
    }
}

void setup() {
    Serial.begin(115200); // boots debugging
    for (int i = 0; i < 6; i++) {
        pinMode(buttonPins[i], INPUT_PULLUP);
    }
    pinMode(PIN_SD_DET, INPUT_PULLUP); // SD CARD DET INPUT
    // ir stuff
    pinMode(PIN_IR_TX, OUTPUT);
    digitalWrite(PIN_IR_TX, LOW);
    pinMode(PIN_IR_RX, INPUT);
    //oled
    Wire.setSDA(PIN_OLED_SDA);
    Wire.setSCL(PIN_OLED_SCL);
    Wire.begin();
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        Serial.println(F("Error, didnt find OLED"));
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(F("OLED found"));
        display.println(F("Initializing..."));
        delay(1000);
        display.clearDisplay();
        display.println(F("welcome to irper"));
        display.display();
    }

    SPI.setSCK(PIN_SD_SCK);
    SPI.setTX(PIN_SD_MOSI);
    SPI.setRX(PIN_SD_MISO);

    if (digitalRead(PIN_SD_DET) == LOW) {
        Serial.println(F("SD card detected"));
        sdReady = SD.begin(PIN_SD_CS);
        if (sdReady) {
            Serial.println(F("MicroSD initialized"));
            ensureSdFolders();
        } else {
            Serial.println(F("MicroSD initialization failed"));
        }
    } else {
        Serial.println(F("No SD card detected"));
    }

    randomSeed(analogRead(A0));
    drawHomeScreen();
}

void loop() {
    int btn = readButtonEdge();
    if (btn < 0) {
        return;
    }

    if (btn == BTN_P1_UP) {
        currentMenu = (currentMenu + MENU_COUNT - 1) % MENU_COUNT;
        drawHomeScreen();
        return;
    }
    if (btn == BTN_P1_DOWN) {
        currentMenu = (currentMenu + 1) % MENU_COUNT;
        drawHomeScreen();
        return;
    }

    if (btn == BTN_SELECT) {
        if (currentMenu == MENU_LISTEN_IR) {
            flowListenIrAuto();
        } else if (currentMenu == MENU_LEARN_IR) {
            flowLearnIr();
        } else if (currentMenu == MENU_SEND_IR) {
            flowSendIr();
        } else if (currentMenu == MENU_GAME_PONG) {
            gamePong2P();
        } else if (currentMenu == MENU_GAME_DUEL) {
            gameDuel();
        } else if (currentMenu == MENU_SD_STATUS) {
            if (sdReady) {
                showMessage("SD ready", "IR slots in /ir");
            } else {
                showMessage("SD not ready", "Check card wiring");
            }
            delay(1000);
        }
    }

    if (btn == BTN_BACK) {
        showMessage("Already at menu", "Use SW1 Select");
        delay(600);
    }

    drawHomeScreen();
}







