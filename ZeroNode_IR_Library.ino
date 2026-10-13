/*
  ZeroNode IR Control - Embedded Library v1.0
  ESP32-C3 Super Mini + SSD1306 128x64 + two buttons

  Public releases contain only the compiled firmware. The generated command
  table and this source stay in private-source.
*/

#define RAW_BUFFER_LENGTH 750
#define DECODE_NEC
#define DECODE_SONY
#define DECODE_SAMSUNG
#define DECODE_LG
#define DECODE_JVC
#define DECODE_PANASONIC
#define DECODE_DENON
#define DECODE_SHARP
#define DECODE_RC5
#define DECODE_RC6
#define EXCLUDE_EXOTIC_PROTOCOLS
#include <IRremote.hpp>
#include <U8g2lib.h>
#include <Wire.h>
#include <Preferences.h>
#include "library_data.h"

// Wiring - identical to the Basic firmware.
constexpr uint8_t OLED_SDA = 5;
constexpr uint8_t OLED_SCL = 6;
constexpr uint8_t BUTTON_NEXT = 0;
constexpr uint8_t BUTTON_OK = 1;
constexpr uint8_t IR_SEND_PIN = 3;
constexpr uint8_t IR_RECV_PIN = 4;
constexpr uint8_t OLED_ADDRESS = 0x3C;

constexpr uint8_t SLOT_COUNT = 6;
constexpr uint16_t MAX_RAW_PULSES = 750;
constexpr uint16_t DEBOUNCE_MS = 26;
constexpr uint16_t LONG_PRESS_MS = 650;
constexpr uint16_t REPEAT_MS = 115;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
Preferences preferences;

struct UserCommand {
  char name[13];
  uint16_t rawLength;
  uint16_t raw[MAX_RAW_PULSES];
  bool valid;
};
UserCommand userCommands[SLOT_COUNT];
uint8_t activeSlot = 0;

enum Screen : uint8_t { HOME, BUILTIN, LEARN, MY_SLOTS, SETTINGS, ABOUT };
enum BrowserLevel : uint8_t { CATEGORY, BRAND, MODEL, COMMAND };
enum KeyEvent : uint8_t { KEY_NONE, KEY_SHORT, KEY_LONG, KEY_REPEAT };

Screen screen = HOME;
BrowserLevel browserLevel = CATEGORY;
const char *homeItems[] = {"BUILT-IN", "LEARN", "MY SLOTS", "SETTINGS", "ABOUT"};
const char *categoryNames[] = {
    "TV", "AIR CONDITIONER", "PROJECTOR", "WHITEBOARD", "DIGITAL SIGN"};
constexpr uint8_t HOME_COUNT = sizeof(homeItems) / sizeof(homeItems[0]);
uint8_t homeSelection = 0;
uint8_t categorySelection = 0;
uint16_t brandSelection = 0;
uint16_t modelSelection = 0;
uint16_t commandSelection = 0;

struct Button {
  uint8_t pin;
  bool stable = HIGH;
  bool lastRead = HIGH;
  uint32_t changedAt = 0;
  uint32_t pressedAt = 0;
  uint32_t repeatedAt = 0;
  bool longSent = false;
};
Button nextButton{BUTTON_NEXT};
Button okButton{BUTTON_OK};

void drawCentered(const char *text, int y) {
  display.drawStr((128 - display.getStrWidth(text)) / 2, y, text);
}

void drawHeader(const char *title) {
  display.setDrawColor(1);
  display.drawBox(0, 0, 128, 12);
  display.setDrawColor(0);
  display.setFont(u8g2_font_5x8_mf);
  display.drawStr(4, 9, title);
  display.setDrawColor(1);
}

void drawFooter(const char *left, const char *right) {
  display.setFont(u8g2_font_4x6_mf);
  display.drawHLine(0, 55, 128);
  display.drawStr(3, 63, left);
  display.drawStr(125 - display.getStrWidth(right), 63, right);
}

void shorten(const char *source, char *target, size_t targetSize, uint8_t maxChars = 18) {
  if (!targetSize) return;
  size_t length = strlen(source);
  if (length <= maxChars) {
    strlcpy(target, source, targetSize);
    return;
  }
  size_t keep = min<size_t>(maxChars - 2, targetSize - 3);
  memcpy(target, source, keep);
  target[keep] = '.';
  target[keep + 1] = '.';
  target[keep + 2] = '\0';
}

void bootAnimation() {
  for (uint8_t frame = 0; frame < 28; ++frame) {
    display.clearBuffer();
    int width = min(104, int(frame * 5));
    display.drawRFrame((128 - width) / 2, 19, width, 25, 4);
    if (frame > 7) {
      display.setFont(u8g2_font_logisoso20_tf);
      drawCentered("Z", 40);
    }
    display.sendBuffer();
    delay(22);
  }
  for (uint8_t frame = 0; frame < 20; ++frame) {
    display.clearBuffer();
    display.setFont(u8g2_font_logisoso20_tf);
    drawCentered("ZeroNode", 35);
    display.setFont(u8g2_font_5x8_mf);
    if (frame > 5) drawCentered("IR CONTROL", 49);
    display.sendBuffer();
    delay(28);
  }
  delay(350);
}

void toast(const char *top, const char *bottom, uint16_t duration = 700) {
  display.clearBuffer();
  display.drawRFrame(4, 15, 120, 36, 4);
  display.setFont(u8g2_font_6x10_tf);
  drawCentered(top, 30);
  display.setFont(u8g2_font_5x8_mf);
  char line[25];
  shorten(bottom, line, sizeof(line), 23);
  drawCentered(line, 43);
  display.sendBuffer();
  delay(duration);
}

void loadUserCommands() {
  preferences.begin("zeronode-ir", true);
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) {
    String key = "cmd" + String(i);
    if (preferences.getBytesLength(key.c_str()) == sizeof(UserCommand)) {
      preferences.getBytes(key.c_str(), &userCommands[i], sizeof(UserCommand));
    } else {
      userCommands[i] = {};
    }
  }
  preferences.end();
}

void saveUserCommand(uint8_t slot) {
  preferences.begin("zeronode-ir", false);
  String key = "cmd" + String(slot);
  preferences.putBytes(key.c_str(), &userCommands[slot], sizeof(UserCommand));
  preferences.end();
}

void clearUserCommand(uint8_t slot) {
  userCommands[slot] = {};
  preferences.begin("zeronode-ir", false);
  String key = "cmd" + String(slot);
  preferences.remove(key.c_str());
  preferences.end();
}

uint16_t browserCount() {
  if (browserLevel == CATEGORY) return ZN_CATEGORY_COUNT;
  if (browserLevel == BRAND) return ZN_CATEGORY_BRAND_COUNT[categorySelection];
  uint16_t brandIndex = ZN_CATEGORY_FIRST_BRAND[categorySelection] + brandSelection;
  const ZNBrand &brand = ZN_BRANDS[brandIndex];
  if (browserLevel == MODEL) return brand.modelCount;
  const ZNModel &model = ZN_MODELS[brand.firstModel + modelSelection];
  return model.commandCount;
}

uint16_t browserSelection() {
  if (browserLevel == CATEGORY) return categorySelection;
  if (browserLevel == BRAND) return brandSelection;
  if (browserLevel == MODEL) return modelSelection;
  return commandSelection;
}

const char *browserItemName(uint16_t relativeIndex) {
  if (browserLevel == CATEGORY) return categoryNames[relativeIndex];
  uint16_t brandIndex = ZN_CATEGORY_FIRST_BRAND[categorySelection] + brandSelection;
  if (browserLevel == BRAND) {
    brandIndex = ZN_CATEGORY_FIRST_BRAND[categorySelection] + relativeIndex;
    return znString(ZN_BRANDS[brandIndex].nameOffset);
  }
  const ZNBrand &brand = ZN_BRANDS[brandIndex];
  uint16_t modelIndex = brand.firstModel + modelSelection;
  if (browserLevel == MODEL) {
    modelIndex = brand.firstModel + relativeIndex;
    return znString(ZN_MODELS[modelIndex].nameOffset);
  }
  const ZNModel &model = ZN_MODELS[modelIndex];
  return znString(ZN_COMMANDS[model.firstCommand + relativeIndex].nameOffset);
}

const char *browserTitle() {
  switch (browserLevel) {
    case CATEGORY: return "BUILT-IN / TYPE";
    case BRAND: return "BUILT-IN / BRAND";
    case MODEL: return "BUILT-IN / MODEL";
    default: return "BUILT-IN / COMMAND";
  }
}

void drawList(const char *title, uint16_t selection, uint16_t count,
              const char *(*itemName)(uint16_t), const char *footerRight) {
  display.clearBuffer();
  drawHeader(title);
  display.setFont(u8g2_font_6x10_tf);
  for (int8_t row = -1; row <= 1; ++row) {
    int32_t item = int32_t(selection) + row;
    while (item < 0) item += count;
    item %= count;
    int y = 25 + (row + 1) * 13;
    char label[22];
    shorten(itemName(uint16_t(item)), label, sizeof(label));
    if (row == 0) {
      display.drawRBox(5, y - 9, 118, 11, 3);
      display.setDrawColor(0);
      display.drawStr(10, y, label);
      display.setDrawColor(1);
    } else {
      display.drawStr(11, y, label);
    }
  }
  char counter[16];
  snprintf(counter, sizeof(counter), "%u/%u", selection + 1, count);
  drawFooter(counter, footerRight);
  display.sendBuffer();
}

const char *homeName(uint16_t index) { return homeItems[index]; }

void drawHome() {
  drawList("ZERONODE / IR CONTROL", homeSelection, HOME_COUNT, homeName, "NEXT  SELECT");
}

void drawBuiltIn() {
  drawList(browserTitle(), browserSelection(), browserCount(), browserItemName,
           browserLevel == COMMAND ? "OK SEND  HOLD BACK" : "OK OPEN  HOLD BACK");
}

void drawLearn() {
  display.clearBuffer();
  drawHeader("LEARN SIGNAL");
  display.setFont(u8g2_font_6x10_tf);
  drawCentered("SELECT A SLOT", 26);
  char slot[22];
  snprintf(slot, sizeof(slot), "SLOT %u - %s", activeSlot + 1,
           userCommands[activeSlot].valid ? "USED" : "EMPTY");
  drawCentered(slot, 40);
  display.setFont(u8g2_font_4x6_mf);
  drawCentered("OK waits for remote signal", 50);
  drawFooter("HOLD OK: HOME", "NEXT  LEARN");
  display.sendBuffer();
}

void drawSlots() {
  display.clearBuffer();
  drawHeader("MY RECORDED SLOTS");
  display.setFont(u8g2_font_6x10_tf);
  char name[22];
  if (userCommands[activeSlot].valid) {
    shorten(userCommands[activeSlot].name, name, sizeof(name));
    drawCentered(name, 30);
    display.setFont(u8g2_font_5x8_mf);
    char details[24];
    snprintf(details, sizeof(details), "%u pulses / 38 kHz", userCommands[activeSlot].rawLength);
    drawCentered(details, 43);
  } else {
    drawCentered("EMPTY SLOT", 34);
  }
  char counter[16];
  snprintf(counter, sizeof(counter), "SLOT %u/%u", activeSlot + 1, SLOT_COUNT);
  drawFooter(counter, "NEXT  SEND");
  display.sendBuffer();
}

void drawSettings() {
  display.clearBuffer();
  drawHeader("SETTINGS");
  display.setFont(u8g2_font_6x10_tf);
  drawCentered("CLEAR ACTIVE SLOT?", 29);
  char slot[20];
  snprintf(slot, sizeof(slot), "SLOT %u - %s", activeSlot + 1,
           userCommands[activeSlot].valid ? "USED" : "EMPTY");
  drawCentered(slot, 43);
  drawFooter("NEXT: SLOT", "OK: CLEAR");
  display.sendBuffer();
}

void drawAbout() {
  display.clearBuffer();
  display.setFont(u8g2_font_logisoso16_tf);
  drawCentered("ZeroNode", 21);
  display.setFont(u8g2_font_5x8_mf);
  drawCentered("EMBEDDED LIBRARY v1.0", 34);
  drawCentered("12935 commands", 44);
  drawCentered("610 remotes / 255 brands", 53);
  drawFooter("5 DEVICE CATEGORIES", "HOLD OK: HOME");
  display.sendBuffer();
}

void render() {
  switch (screen) {
    case HOME: drawHome(); break;
    case BUILTIN: drawBuiltIn(); break;
    case LEARN: drawLearn(); break;
    case MY_SLOTS: drawSlots(); break;
    case SETTINGS: drawSettings(); break;
    case ABOUT: drawAbout(); break;
  }
}

void captureSignal() {
  toast("LISTENING", "Press remote button", 250);
  IrReceiver.resume();
  uint32_t started = millis();
  while (millis() - started < 10000) {
    if (IrReceiver.decode()) {
      auto &source = IrReceiver.irparams;
      if (source.OverflowFlag) {
        IrReceiver.resume();
        toast("SIGNAL TOO LONG", "Cannot save this frame");
        return;
      }
      uint16_t count = min(uint16_t(source.rawlen - 1), MAX_RAW_PULSES);
      if (count < 4) {
        IrReceiver.resume();
        continue;
      }
      UserCommand &saved = userCommands[activeSlot];
      saved.rawLength = count;
      for (uint16_t i = 0; i < count; ++i) {
        uint32_t micros = uint32_t(source.rawbuf[i + 1]) * MICROS_PER_TICK;
        saved.raw[i] = min<uint32_t>(micros, 65535);
      }
      snprintf(saved.name, sizeof(saved.name), "SIGNAL %u", activeSlot + 1);
      saved.valid = true;
      saveUserCommand(activeSlot);
      IrReceiver.resume();
      toast("SAVED", saved.name);
      return;
    }
    render();
    delay(12);
  }
  toast("NO SIGNAL", "Try closer to receiver");
}

void sendUserCommand() {
  if (!userCommands[activeSlot].valid) {
    toast("SLOT EMPTY", "Use Learn first");
    return;
  }
  IrSender.sendRaw(userCommands[activeSlot].raw,
                   userCommands[activeSlot].rawLength, 38);
  toast("SENT", userCommands[activeSlot].name, 380);
}

void sendRawLibraryCommand(const ZNCommand &command) {
  IrSender.enableIROut(command.frequencyKHz);
  for (uint16_t i = 0; i < command.rawLength; ++i) {
    uint32_t duration = uint32_t(ZN_RAW_TIMINGS[command.rawOffset + i]) * 2UL;
    // IRremote space() accepts uint16_t. Long gaps may be split safely.
    if ((i & 1U) == 0) {
      while (duration > 65535) {
        IrSender.mark(65535);
        duration -= 65535;
      }
      IrSender.mark(uint16_t(duration));
    } else {
      while (duration > 65535) {
        IrSender.space(65535);
        duration -= 65535;
      }
      IrSender.space(uint16_t(duration));
    }
  }
  IrSender.space(0);
}

void sendSelectedLibraryCommand() {
  uint16_t brandIndex = ZN_CATEGORY_FIRST_BRAND[categorySelection] + brandSelection;
  const ZNBrand &brand = ZN_BRANDS[brandIndex];
  const ZNModel &model = ZN_MODELS[brand.firstModel + modelSelection];
  const ZNCommand &command = ZN_COMMANDS[model.firstCommand + commandSelection];

  switch (command.protocol) {
    case ZN_NEC:
    case ZN_NEC_EXT:
      IrSender.sendNEC(command.address, command.command, 0);
      break;
    case ZN_SAMSUNG32:
      IrSender.sendSamsung(command.address, command.command, 0);
      break;
    case ZN_SONY12:
      IrSender.sendSony(command.address, command.command, 2, 12);
      break;
    case ZN_SONY15:
      IrSender.sendSony(command.address, command.command, 2, 15);
      break;
    case ZN_SONY20:
      IrSender.sendSony(command.address, command.command, 2, 20);
      break;
    case ZN_RC5:
      IrSender.sendRC5(command.address, command.command, 0, true);
      break;
    case ZN_RC6:
      IrSender.sendRC6(command.address, command.command, 0, true);
      break;
    case ZN_RAW:
      sendRawLibraryCommand(command);
      break;
  }
  toast("SENT", znString(command.nameOffset), 330);
}

KeyEvent readButton(Button &button, bool repeatAllowed) {
  uint32_t now = millis();
  bool value = digitalRead(button.pin);
  if (value != button.lastRead) {
    button.lastRead = value;
    button.changedAt = now;
  }
  if (now - button.changedAt < DEBOUNCE_MS) return KEY_NONE;
  if (value != button.stable) {
    button.stable = value;
    if (value == LOW) {
      button.pressedAt = now;
      button.repeatedAt = now;
      button.longSent = false;
    } else if (!button.longSent) {
      return KEY_SHORT;
    }
  }
  if (button.stable == LOW && !button.longSent && now - button.pressedAt >= LONG_PRESS_MS) {
    button.longSent = true;
    button.repeatedAt = now;
    return repeatAllowed ? KEY_REPEAT : KEY_LONG;
  }
  if (repeatAllowed && button.stable == LOW && button.longSent && now - button.repeatedAt >= REPEAT_MS) {
    button.repeatedAt = now;
    return KEY_REPEAT;
  }
  return KEY_NONE;
}

void nextBrowserItem() {
  uint16_t count = browserCount();
  if (browserLevel == CATEGORY) categorySelection = (categorySelection + 1) % count;
  else if (browserLevel == BRAND) brandSelection = (brandSelection + 1) % count;
  else if (browserLevel == MODEL) modelSelection = (modelSelection + 1) % count;
  else commandSelection = (commandSelection + 1) % count;
}

void openBrowserItem() {
  if (browserLevel == CATEGORY) {
    browserLevel = BRAND;
    brandSelection = modelSelection = commandSelection = 0;
  } else if (browserLevel == BRAND) {
    browserLevel = MODEL;
    modelSelection = commandSelection = 0;
  } else if (browserLevel == MODEL) {
    browserLevel = COMMAND;
    commandSelection = 0;
  } else {
    sendSelectedLibraryCommand();
  }
}

void back() {
  if (screen == BUILTIN && browserLevel != CATEGORY) {
    browserLevel = BrowserLevel(uint8_t(browserLevel) - 1);
  } else if (screen != HOME) {
    screen = HOME;
  }
}

void handleButtons() {
  KeyEvent nextEvent = readButton(nextButton, true);
  KeyEvent okEvent = readButton(okButton, false);

  if (okEvent == KEY_LONG) {
    back();
    return;
  }
  bool next = nextEvent == KEY_SHORT || nextEvent == KEY_REPEAT;
  bool ok = okEvent == KEY_SHORT;

  if (screen == HOME) {
    if (next) homeSelection = (homeSelection + 1) % HOME_COUNT;
    if (ok) screen = Screen(homeSelection + 1);
  } else if (screen == BUILTIN) {
    if (next) nextBrowserItem();
    if (ok) openBrowserItem();
  } else if (screen == LEARN) {
    if (next) activeSlot = (activeSlot + 1) % SLOT_COUNT;
    if (ok) captureSignal();
  } else if (screen == MY_SLOTS) {
    if (next) activeSlot = (activeSlot + 1) % SLOT_COUNT;
    if (ok) sendUserCommand();
  } else if (screen == SETTINGS) {
    if (next) activeSlot = (activeSlot + 1) % SLOT_COUNT;
    if (ok) {
      clearUserCommand(activeSlot);
      toast("SLOT CLEARED", "Recorded signal removed");
    }
  }
}

void setup() {
  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);
  Wire.begin(OLED_SDA, OLED_SCL);
  display.setI2CAddress(OLED_ADDRESS << 1);
  display.begin();
  display.setFontMode(1);
  bootAnimation();
  loadUserCommands();

  IrSender.begin(IR_SEND_PIN, DISABLE_LED_FEEDBACK);
  IrReceiver.begin(IR_RECV_PIN, DISABLE_LED_FEEDBACK);
}

void loop() {
  handleButtons();
  render();
  delay(12);
}
