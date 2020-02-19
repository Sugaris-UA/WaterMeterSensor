#include <math.h>
#include <Encoder.h>
#include <TFT_HX8357.h>
#include "common.h"
#include "Free_Fonts.h"

#define TFT_GREY 0x5AEB
#define encoderPinDT 2
#define encoderPinCLK 3
#define encoderPinButtonSW 4

Encoder encoder(encoderPinDT, encoderPinCLK);
TFT_HX8357 tft = TFT_HX8357();
ScreenView currentView = ScreenView::main;
Settings appSettings = {
  4,      // density index
  10,     // mlStep
  0,      // blindMl
  1,      // sensorMultiplier
  false,  // startBeep
  false,  // finishBeep
  false   // invertOutputPin
};

const uint16_t backgroundColor = 0x0000;
const uint16_t labelColor = tft.color565(190, 190, 187);
const uint16_t inactiveColor = tft.color565(35, 32, 35);
const uint16_t targetMlColor = labelColor;
const uint16_t currentMlColor = labelColor;

long now = 0;
const int encoderStep = 2;
long prevEncoderPosition = 0;
bool isEditTargetMl = false;
bool encoderButtonState = false;
long encoderButtonPressedMillis = 0;
long encoderButtonReleasedMillis = 0;
const int mlMin = 100;
const int mlMax = 19999;
const int mlPadding = 138; // tft.textWidth("0", 7) -> 32 pixels (1 symbol)
BlinkInt prevTargetMl = { 0, targetMlColor };
int currentMl = 600;
float prevProgress = -1;
bool isZeroPercAlreadyDrown = false; // added to avoid blinking 0.00% in progress bar when changing densities quickly
bool unknownTimeDrown = false;
long finishTime = 0;
long prevH = 0;
long prevM = 0;
long prevS = 0;
const int historyLength = 5;
HistoryNote history[historyLength];
bool missionComplete = false;
bool isDraining = false;
long lastUserTouch = 0;
const long lastUserTouchDelay = 5000;

uint16_t colorUltraSoft   = tft.color565(117, 158, 178);
uint16_t colorSoft        = tft.color565(122, 181, 181);
uint16_t colorNormal      = tft.color565(137, 188, 106);
uint16_t colorStrong      = tft.color565(229, 173, 39);
uint16_t colorUltraStrong = tft.color565(225, 160, 51);

Density densities[] = {
  { 10 , "UltraSoft", colorUltraSoft, 600 },
  { 20 , "UltraSoft+", colorUltraSoft, 700 },
  { 30 , "Soft", colorSoft, 900 },
  { 40 , "Soft+", colorSoft, 1000 },
  { 50 , "Normal", colorNormal, 1200 },
  { 60 , "Normal+", colorNormal, 1300 },
  { 70 , "Strong", colorStrong, 1500 },
  { 80 , "Strong+", colorStrong, 1600 },
  { 90 , "UltraStrong", colorUltraStrong, 1800 },
  { 100, "UltraStrong+", colorUltraStrong, 2000 },
};

int densitiesCount = sizeof(densities) / sizeof(densities[0]);

void setup(void) {
  //Serial.begin(115200);
  
  // pins
  pinMode(encoderPinButtonSW, INPUT_PULLUP);
  digitalWrite(encoderPinButtonSW, HIGH);

  // screen
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(backgroundColor);
  tft.setTextSize(1);

  // populate history
  for (int i = 0; i < historyLength; i++) {
    history[i] = { 0, 0 };
  }
  // test history values
  history[0] = { millis(), 10 };
  history[1] = { millis() + 10000, 22 };
  history[2] = { millis() + 20000, 33 };
  history[3] = { millis() + 30000, 42 };
  history[4] = { millis() + 40000, 50 };

  onViewSwitch();
}

Density &currentDensity() {
  return densities[appSettings.density];
}

void tryContinueMission() {
  if (missionComplete && !isDraining && currentDensity().targetMl > currentMl) {
    missionComplete = false;
  }
}

void onViewSwitch() {
  if (currentView == ScreenView::main)
  {
    prevProgress = -1;
    isZeroPercAlreadyDrown = false;
    drawDensityName();
    drawLabels();
    drawTargetMl();
    drawCurrentMl();
    drawTimeLeft();
  }
}

void drawDensityName() {
  tft.setFreeFont(&FreeSerifBold24pt7b);
  tft.setTextDatum(TC_DATUM);

  tft.fillRect(80, 24, 360, 50, backgroundColor);
  tft.setTextColor(currentDensity().color, backgroundColor);
  tft.drawString(currentDensity().name, 240, 25, GFXFF);
}

void drawLabels() {
  if (currentView == ScreenView::main) {
    tft.setFreeFont(&FreeSerif18pt7b);
    tft.setTextDatum(BR_DATUM);
    tft.setTextColor(labelColor);

    tft.drawString("Target :", 210, 147, GFXFF);
    tft.drawString("Current :", 210, 207, GFXFF);
    tft.drawString("ml", 410, 147, GFXFF);
    tft.drawString("ml", 410, 207, GFXFF);
    tft.drawString("Time left :", 210, 265, GFXFF);
  }
}

void drawProgress() {
  if (currentView == ScreenView::main) {
    int x = 50;
    int y = 280;
    int w = 380;
    int h = 35;
    int maxProgressW = w - 4;

    float progress = (float)currentMl / currentDensity().targetMl;  
    progress = roundf(progress * 10000.00) / 10000.00; // round to 0.1234

    if (progress > 1 || (missionComplete && !isEditTargetMl)) {
      progress = 1.0000;
    }

    if (prevProgress != progress) {
      prevProgress = progress;
      int progressW = progress * maxProgressW;

      // border
      tft.drawRect(x, y, w, h, currentDensity().color);

      if (progress > 0 || !isZeroPercAlreadyDrown) {
        // empty part
        if (progressW < maxProgressW) {
          tft.fillRect(x + 2 + progressW, y + 2, maxProgressW - progressW, h - 4, backgroundColor);
        }
        
        // filled part
        tft.fillRect(x + 2, y + 2, progressW, h - 4, currentDensity().color);

        // percentage
        tft.setFreeFont(&FreeMonoBold12pt7b);
        tft.setTextDatum(CL_DATUM);
        tft.setTextColor(TFT_WHITE);
        
        char perc[10];
        dtostrf(progress * 100, 2, 2, perc);
        int percWidth = tft.textWidth(perc, GFXFF);

        for (int i = 0; i < 10; i++) {
          if (perc[i] == NULL) {
            perc[i] = '%';
            perc[i + 1] = NULL;
            break;
          }
        }
        
        tft.drawString(perc, 239 - (percWidth / 2), y + (h / 2) - 1, GFXFF);
        isZeroPercAlreadyDrown = progress == 0;
      }
    }
  }
}

void drawTargetMl() {
  if (currentView == ScreenView::main) {
    BlinkInt newTargetMl = { currentDensity().targetMl, targetMlColor };

    if (isEditTargetMl) {
      if ((now - encoderButtonReleasedMillis) % 666 < 333) {
        newTargetMl.color = inactiveColor;
      } else {
        newTargetMl.color = targetMlColor;
      }
    }

    if (newTargetMl.value != prevTargetMl.value) {
      updateFinishTime();
      drawProgress();
    }
    

    if (!AreBlinkIntEqual(newTargetMl, prevTargetMl)) {
      prevTargetMl = newTargetMl;

      tft.setTextColor(newTargetMl.color, backgroundColor);
      tft.setTextDatum(BR_DATUM);
      tft.setTextPadding(mlPadding);
      tft.drawNumber(currentDensity().targetMl, 370, 140, 7);
      tft.setTextPadding(0);
    }
  }
}

void drawCurrentMl() {
  if (currentView == ScreenView::main) {
    tft.setTextColor(currentMlColor, backgroundColor);
    tft.setTextDatum(BR_DATUM);
    tft.setTextPadding(mlPadding);
    tft.drawNumber(currentMl, 370, 200, 7);
    tft.setTextPadding(0);
  }
}

void clearTimeLeft() {
  if (currentView == ScreenView::main) {
    tft.fillRect(212, 215, 268, 50, backgroundColor);
  }
}

void drawTimeLeft() {
  if (currentView == ScreenView::main) {
    const int xAncor = 200;
    const int yAncor = 265;

    if (finishTime == 0) {
      if (!unknownTimeDrown) {
        unknownTimeDrown = true;
        tft.setTextDatum(BR_DATUM);
        tft.setFreeFont(&FreeSansBold18pt7b);
        clearTimeLeft();
        tft.setTextColor(labelColor, backgroundColor);
        tft.drawString("?", xAncor + 128, yAncor, GFXFF);
      }
      return;
    }

    char buffer[6];
    long h = 0;
    long m = 0;
    long s = 0;
    long secondsLeft = (finishTime - now) / 1000;

    if (secondsLeft >= 360000) {
      secondsLeft = 359999;
    }

    bool isTimeUp = (missionComplete && !isEditTargetMl) || secondsLeft < 1;

    if (!isTimeUp) {
      h = secondsLeft / 3600;
      m = secondsLeft % 3600 / 60;
      s = secondsLeft % 3600 % 60;
    }

    bool isFirstDraw = !isTimeUp && prevH == 0 && prevM == 0 && prevS == 0;
    unknownTimeDrown = false;

    tft.setTextDatum(BR_DATUM);
    
    if (isFirstDraw) {
      tft.setFreeFont(&FreeMono18pt7b);
      tft.setTextColor(labelColor, backgroundColor);
      clearTimeLeft();
      tft.drawString(":", xAncor + 143, yAncor - 2, GFXFF);
      tft.drawString(":", xAncor + 82, yAncor - 2, GFXFF);
    }

    tft.setFreeFont(&FreeMonoBold18pt7b);
    tft.setTextColor(TFT_YELLOW, backgroundColor);

    tft.setTextPadding(0);

    if (isFirstDraw || prevS != s) {
      if (!isFirstDraw && prevS / 10 == s / 10) {
        // update 1 digit only
        tft.drawNumber(s % 10, xAncor + 189, yAncor, GFXFF);
      } else {
        // update both digits
        sprintf(buffer, "%02d", s);
        tft.drawString(buffer, xAncor + 189, yAncor, GFXFF);
      }
      prevS = s;
    }

    tft.setTextPadding(40);

    if (isFirstDraw || prevM != m) {
      sprintf(buffer, "%02d", m);
      tft.drawString(buffer, xAncor + 128, yAncor, GFXFF);
      prevM = m;
    }

    if (isFirstDraw || prevH != h) {
      tft.drawNumber(h, xAncor + 66, yAncor, GFXFF);
      prevH = h;
    }

    tft.setTextPadding(0);
  }
}

float calcMsFor1Ml() {
  int last = historyLength - 1;
  float ms = ((float)(history[last].millis - history[0].millis)) / (history[last].value - history[0].value);
  
  return ms;
}

void updateFinishTime() {
  // return if history is not populated
  for (int i = 0; i < historyLength; i++) {
    if (history[i].millis == 0) {
      finishTime = 0;
      prevH == 0;
      prevM == 0;
      prevS == 0;
      return;
    }
  }

  // calc finish time
  int mlLeft = currentDensity().targetMl - currentMl;

  if (mlLeft > 0) {
    float msFor1Ml = calcMsFor1Ml();
    finishTime = now + (msFor1Ml * mlLeft);
  } else {
    finishTime = now;
  }
}

void updateHistory(int ml) {
  for (int i = 1; i < historyLength; i++) {
    history[i - 1] = history[i];
  }
  history[historyLength - 1].millis = now;
  history[historyLength - 1].value = ml;
}

void readCurrentMl() {
  int newMl = 600; // read ml from sensor
  
  if (newMl > currentMl) {
    currentMl = newMl;
    updateHistory(currentMl);
    onCurrentMlChanged();
  } else if (newMl < currentMl) {
    if (isDraining || currentMl - newMl > 30 || newMl == 0) {
      isDraining = true;
      currentMl = newMl;
      onCurrentMlChanged();
    }
  }

  if (!isEditTargetMl && now - lastUserTouch > lastUserTouchDelay) {
    missionComplete = missionComplete || newMl >= currentDensity().targetMl;
  }
  
  if (missionComplete && currentMl == 0) {
    startNewPasta();
  }
}

void onCurrentMlChanged() {
  drawCurrentMl();
  updateFinishTime();
  drawProgress();
}

void switchDensity(int direction) {
  appSettings.density += direction;

  if (appSettings.density < 0) {
    appSettings.density = 0;
  } else if (appSettings.density >= densitiesCount) {
    appSettings.density = densitiesCount - 1;
  } else {
    tryContinueMission();
    prevProgress = -1;
    drawDensityName();
    drawTargetMl();
  }
}

void editDensityTargetMl(int increment) {
  int val = appSettings.mlStep * increment;
  Density &d = currentDensity();
  d.targetMl += val;

  d.targetMl = (d.targetMl / appSettings.mlStep) * appSettings.mlStep;

  if (d.targetMl < mlMin) {
    d.targetMl = mlMin;
  } else if (d.targetMl > mlMax) {
    d.targetMl = mlMax;
  }
}

void startNewPasta() {
  // set unknown finish time
  finishTime = 0;
  prevH == 0;
  prevM == 0;
  prevS == 0;
  unknownTimeDrown = false;

  // reset progress
  isZeroPercAlreadyDrown = false;
  prevProgress = -1;

  // clear history
  for (int i = 0; i < historyLength; i++) {
    history[i].millis = 0;
    history[i].value = 0;
  }

  // reset draining
  isDraining = false;

  // reset missionComplete flag
  missionComplete = false;
}

void onEncoderClick() {
  if (currentView == ScreenView::main) {
    isEditTargetMl = !isEditTargetMl;
    if (!isEditTargetMl) {
      tryContinueMission();
      drawTargetMl();
      updateFinishTime();
      drawProgress();
    }
  }
}

void onEncoderLongClick() {
  if (isEditTargetMl) {
    onEncoderClick();
  } else {
    startNewPasta();
  }
}

void checkEncoder() {
  // check button
  bool encoderButtonStateNew = !digitalRead(encoderPinButtonSW);

  if (encoderButtonState != encoderButtonStateNew) {
    encoderButtonState = encoderButtonStateNew;
    
    if (encoderButtonState) {
      encoderButtonPressedMillis = now;
    } else {
      encoderButtonReleasedMillis = now;
      if (encoderButtonReleasedMillis - encoderButtonPressedMillis > 1500) {
        onEncoderLongClick();
      } else {
        onEncoderClick();
      }
    }
  }

  // check position
  if (!encoderButtonState) {
    long newPosition = encoder.read() / encoderStep;
    
    if (newPosition < prevEncoderPosition) {
      prevEncoderPosition = newPosition;
      if (currentView == ScreenView::main) {
        if (isEditTargetMl) {
          editDensityTargetMl(-1);
        } else {
          switchDensity(-1);
        }
      }
    } else if (newPosition > prevEncoderPosition) {
      prevEncoderPosition = newPosition;
      if (currentView == ScreenView::main) {
        if (isEditTargetMl) {
          editDensityTargetMl(1);
        } else {
          switchDensity(1);
        }
      }
    }
  }
}

void loop() {
  now = millis();
  checkEncoder();
  readCurrentMl();

  if (currentView == ScreenView::main) {
    if (isEditTargetMl) {
      drawTargetMl();
    }

    drawTimeLeft();
  }
  
  delay(50);
}
