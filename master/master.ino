#include <LiquidCrystal.h>
#include <Wire.h>
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


int showX = 0;
int lastValue = 0;
int danger = 0;

void setup() {
  lcd.begin(8, 3);
  Wire.begin();
  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  // If data has been received
  if (Serial.available() > 0) {
      danger = Serial.read();
  }

  //Request heart rate data from  
  String a = "";
  Wire.requestFrom(8, 9);
  int i = 0;
  while(Wire.available() > 0){
      char c = Wire.read();
      if(c == '\0'){
        break;
      }
      a.concat(c);
  }
 
  if(!danger){
    printLCD(a);
  }else{
    printLCD("Danger!!!");
  }
  
  Serial.print(a);
  delay(1000);
}

void printLCD(String str){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(str.substring(0, 8));
  if(str.length() > 8){
    lcd.setCursor(0, 1);
    lcd.print(str.substring(8));
  }

}
