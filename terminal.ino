#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <EEPROM.h>
#include <Wire.h>
//----------------------------------------compiler directives and constants-------------------

#define saveInterval 5000
#define M5StackKeyboardIncluded

#ifdef M5StackKeyboardIncluded
  #define CARDKB_ADDR 0x5F
#endif // M5StackKeyboardIncluded

//----------------------------------------variables-------------------------------------------

char charFromSerial;
bool charToPrintAvailable = false;

Adafruit_SH1106 myDisplay(7, 8, 6); //Adafruit_SH1106(int8_t DC, int8_t RST, int8_t CS);

//----------------------------------------functions-------------------------------------------
#define FONT_SIZE 1
#define fontX 7
#define fontY 8
#define cursorBlinkTime 750

class terminalSH1106 {
public:
  static const int ALCADA = 8;
  static const int AMPLADA = 18;
  char charArray[AMPLADA][ALCADA];

  byte currPosX;
  byte currPosY;

  bool cursorState,saved;
  unsigned long lastCursorBlink;
  unsigned long lastChange;

  Adafruit_SH1106* pantalla;
  char blank = ' ';
  int saveEepromStartIndex;
  terminalSH1106(Adafruit_SH1106* pantallaExt) {
    this->pantalla = pantallaExt;
  }
  void init() {
    pantalla->begin(SH1106_SWITCHCAPVCC);
    pantallaBlank();
    pantalla->setCursor(0, 0);
    pantalla->setTextWrap(true);
    pantalla->setTextColor(WHITE);
    pantalla->setTextSize(FONT_SIZE);
    pantalla->println("Greetings from:"); 
    pantalla->println("garfius@gmail.com");
    pantalla->display();
    delay(cursorBlinkTime);
    netejaArray();
    loadState();
    displayArray();
    cursorState = false;
    saved = false;
    lastCursorBlink = millis();
    lastChange = 0;
    saveEepromStartIndex = 0;
  }
  void scrollArray() {
    for (int y = 0; y < (ALCADA - 1); y++) {
      for (int x = 0; x < AMPLADA; x++)
      {
        charArray[x][y] = charArray[x][y + 1];
      }
    }
  }
  void crlf() {
    currPosX = 0;
    currPosY += 1;
  }
  byte lastFilledChar() {
    for (int x = (AMPLADA-1); x >= 0; x--)
    {
      if (charArray[x][currPosY] != blank) return x;
    }
    return 0;
  }
  void doBack() {
    if (currPosX > 0) {
      currPosX -= 1;
      charArray[currPosX][currPosY] = this->blank;
    }
    else if (currPosY > 0) {
      currPosY -= 1;
      currPosX = lastFilledChar();
      charArray[currPosX][currPosY] = this->blank;
    }
  }
  void escriuCaracter(char caracter,bool append=true) {
    //Serial.println(caracter, HEX);
    if ((caracter == 0x0D) || (caracter == 0x0A)) {// cr || lf  
      crlf();
    }else if ((caracter == 0x08) || (caracter == 0x7F)) {//backspace
      doBack();
    } else {
      charArray[currPosX][currPosY] = caracter;
      if(append)currPosX += 1;
      saved = false;
      this->lastChange = millis();
    }

    if (currPosX >= AMPLADA) {
      crlf();
    }
    
    if (currPosY >= ALCADA) {
      currPosY = ALCADA - 1;
      scrollArray();
      netejaLinia(ALCADA - 1);
    }

  }
  void netejaArray() {
    for (int y = 0; y < ALCADA; y++) {
      netejaLinia(y);
    }
  currPosX = 0;
  currPosY = 0;
  }
  void netejaLinia(int y) {
    for (int x = 0; x < AMPLADA; x++)
    {
      charArray[x][y] = this->blank;
    }
  }
  void displayArray() {
    pantalla->clearDisplay();   // clears the screen and buffer
    for (int y = 0; y < ALCADA; y++) {
      for (int x = 0; x < AMPLADA; x++)
      {
        pantalla->drawChar(x*fontX, y*fontY, charArray[x][y], WHITE, BLACK, FONT_SIZE);
      }
    }
    pantalla->display();
  }
  void pantallaBlank() {
    pantalla->clearDisplay();
    pantalla->display();
  }
  void cursorBlink() {
    this->pantalla->drawFastHLine((int16_t)this->currPosX * fontX, (((int16_t)this->currPosY + 1) * fontY) - 1, fontX - 2, (cursorState) ? WHITE : BLACK);
    cursorState = !cursorState;
    this->pantalla->display();
  }
  void cursorBlinkIfNeeded() {
    if (millis() > (lastCursorBlink+cursorBlinkTime)) {
      cursorBlink();
      lastCursorBlink = millis();
    }else if ((lastCursorBlink + cursorBlinkTime) < lastCursorBlink) {
      cursorBlink();
      lastCursorBlink = 0;
    }
  }
  void saveState() {
    //Serial.println("save");
    for (int y = 0; y < ALCADA; y++) {
      for (int x = 0; x < AMPLADA; x++)
      {
        EEPROM.write(((y*AMPLADA) + x+ saveEepromStartIndex), (uint8_t)charArray[x][y]);
      }
    }
    EEPROM.write(saveEepromStartIndex + (ALCADA*AMPLADA) + 1, currPosX);
    EEPROM.write(saveEepromStartIndex + (ALCADA*AMPLADA), currPosY);
    saved = true;
  }
  void loadState() {
    if ((EEPROM.read(0) == 0x00) || (EEPROM.read(0) == 0xFF)) {
      //Serial.println("format");
      netejaArray();
      saveState();
      return;
    }

    for (int y = 0; y < ALCADA; y++) {
      for (int x = 0; x < AMPLADA; x++)
      {
        charArray[x][y] = (char)EEPROM.read((y*AMPLADA)+x+ saveEepromStartIndex);
      }
    }
    
    currPosX = EEPROM.read(saveEepromStartIndex + (ALCADA*AMPLADA) + 1);
    currPosY = EEPROM.read(saveEepromStartIndex + (ALCADA*AMPLADA));
  
    saved = true;
  }
  void checkSave() {
    if (saved) return;
    if (millis() > (this->lastChange + saveInterval)) {
      saveState();
      lastChange = millis();
    }
    else if ((lastChange + saveInterval+1000) < lastChange) {
      saveState();
      lastChange = 0;
    }
  }
  int getpage() {
    return saveEepromStartIndex / ((ALCADA * AMPLADA) + 2);
  }
  void setPage(int page) {
  if ((page < 0) || (page == getpage())) {
    return;
  }
    if (!saved) {
      saveState();
    }
  saveEepromStartIndex = (page * ((ALCADA * AMPLADA) + 2)) + 1;
  //Serial.println(saveEepromStartIndex);
    loadState();
  displayPage();
  }
  void displayPage() {
  pantalla->clearDisplay();
    pantalla->setCursor(0, 0);
    pantalla->print("Pag: ");
    pantalla->print(getpage());
    pantalla->display();
    delay(cursorBlinkTime);
    displayArray();
  }
};

terminalSH1106 myTerminal= terminalSH1106(&myDisplay);;

void setup() {
  Serial.begin(57600);
  Serial.println("Greetings from garfius@gmail.com");

  myTerminal.init();
  
  Wire.begin();
  
}
void loop(void) {
  
#ifdef M5StackKeyboardIncluded
  Wire.requestFrom(CARDKB_ADDR, 1);

  while (Wire.available())
  {
    charFromSerial = Wire.read();
    if( (charFromSerial == 0xFFFFFFB6) || (charFromSerial == 0xFFFFFFB5)){
    if (charFromSerial == 0xFFFFFFB6) {
      myTerminal.setPage(myTerminal.getpage() + 1);
    }
    else {
      myTerminal.setPage(myTerminal.getpage() - 1);
    }
      charToPrintAvailable = false;
    charFromSerial = 0x00000000;
    }
  charToPrintAvailable = (charFromSerial != 0x00);
  }
#endif

  if (Serial.available()) {
    charFromSerial = Serial.read();
  //Serial.print(charFromSerial, HEX);
    charToPrintAvailable = true;
  }
  
  if (charToPrintAvailable) {
    
    Serial.println();
    charToPrintAvailable = false;
    myTerminal.escriuCaracter(charFromSerial);
    myTerminal.displayArray();
  }
  else {
    myTerminal.cursorBlinkIfNeeded();
    myTerminal.checkSave();
  }
  

}
