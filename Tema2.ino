

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const uint8_t LM35_PIN = A0;

volatile uint8_t oraCurenta = 17;
volatile uint8_t minutCurent = 0;
volatile uint8_t secundaCurenta = 0;

volatile bool trebuieActualizat = true;

void configureazaTimer1() {
  cli();               
  TCCR1A = 0;            
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 15624;
  TCCR1B |= (1 << WGM12);                 
  TCCR1B |= (1 << CS12) | (1 << CS10);   
  TIMSK1 |= (1 << OCIE1A);                
  sei();                                  
}

ISR(TIMER1_COMPA_vect) {
  secundaCurenta++;

  if (secundaCurenta >= 60) {
    secundaCurenta = 0;
    minutCurent++;

    if (minutCurent >= 60) {
      minutCurent = 0;
      oraCurenta++;

      if (oraCurenta >= 24) {
        oraCurenta = 0;
      }
    }
  }

  trebuieActualizat = true;
}

float citesteTemperaturaC() {
  int valoareADC = analogRead(LM35_PIN);
  float tensiune = valoareADC * (5.0 / 1023.0);

  return tensiune * 100.0;
}

void setup() {
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LM35 + Ceas");
  lcd.setCursor(0, 1);
  lcd.print("Se initializeaza");

  configureazaTimer1();
  delay(1000);
  lcd.clear();
}

void loop() {
  bool actualizeaza;
  uint8_t h, m, s;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    actualizeaza = trebuieActualizat;
    trebuieActualizat = false;
    h = oraCurenta;
    m = minutCurent;
    s = secundaCurenta;
  }

  if (actualizeaza) {
    float temperatura = citesteTemperaturaC();

    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatura, 1);
    lcd.print((char)223); 
    lcd.print("C   ");   

    char bufferOra[9];
    snprintf(bufferOra, sizeof(bufferOra), "%02u:%02u:%02u", h, m, s);
    lcd.setCursor(0, 1);
    lcd.print("Ora: ");
    lcd.print(bufferOra);
    lcd.print("   ");

    Serial.print("Temperatura: ");
    Serial.print(temperatura, 1);
    Serial.print(" C | Ora: ");
    Serial.println(bufferOra);
  }
}

