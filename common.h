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
  none,
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
  bool invertRelay;
};

struct HistoryNote {
  long millis;
  float value;
};

char *ToYesNo(bool value) {
  if (value) {
    return "YES";
  }
  return "NO";
};

;