;
/*uint16_t color565(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint16_t) (((r >> 3) & 0x1f) << 11 | ((g >> 2) & 0x3f) << 5 | (b >> 3) & 0x1f);
};*/

struct Density {
  byte id;
  char name[30];
  uint16_t color;
  int targetMl;
};

enum ScreenView {
  main,
  settings,
  error
};

struct Settings {
  int density;
  int mlStep;
  int blindMl;
  float sensorMultiplier;
  bool startBeep;
  bool finishBeep;
  bool invertOutputPin;
};

struct HistoryNote {
  long millis;
  float value;
};

struct BlinkInt {
  int value;
  uint16_t color;
};

bool AreBlinkIntEqual(BlinkInt val1, BlinkInt val2) {
  return val1.value == val2.value && val1.color == val2.color;
};

;