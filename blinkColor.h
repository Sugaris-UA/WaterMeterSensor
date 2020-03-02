class BlinkColor {
  private:
    uint16_t defaultColor;
    uint16_t blinkColor;

  public:
    uint16_t color;
    bool changed;

    BlinkColor() {
      this->changed = false;
    }

    void reset(uint16_t defaultColor, uint16_t blinkColor) {
      this->defaultColor = defaultColor;
      this->blinkColor = blinkColor;
      this->changed = false;
      this->color = this->color + 1;
    }
    
    void update(unsigned long currentTime, unsigned long startTime) {
      if ((currentTime - startTime) % 600 < 300) {
        this->changed = this->color != blinkColor;
        this->color = blinkColor;
      } else {
        this->changed = this->color != defaultColor;
        this->color = defaultColor;
      }
    }
};
