//----- Settings -----
#define HOEHE 195         //Höhe Sensor über Boden in cm
#define PEGEL_MAX 175     //Höhe max. Wasserstand in cm bis Überlauf
#define PEGEL_S1_ON 15    //Pegelstand in cm, unter dem eingeschaltet wird
#define PEGEL_S1_OFF 18   //Pegelstand in cm, über dem ausgeschaltet wird
#define D_OUT_PING 2
#define D_IN_ECHO 3

// Kalibrierung: Umrechnung Mikrosekunden in Entfernung
// 340 m/s =   68 cm / ms      1 us = 0,68 mm
#define CAL_M 56.05       // CAL_M = (Zeit[us] - CAL_N) / Entfernung[cm]
#define CAL_N  280.0      // CAL_N = Zeit[us] - (CAL_M * Entfernung[cm])


//----- Konstanten -----
#define GR_LEN_X  128     //Breite und Höhe der Graphik in Pixel
#define GR_LEN_Y  20
#define GR_X0     0       //Eckpunkte der Grafik in Pixel
#define GR_Y0     43
#define GR_X1     127
#define GR_Y1     63

#define S1        5       //Dig.Ausgang für Schaltsignal Nachspeisung

#define ANZ_AVG 20    //Anzahl Werte für gleitenden Mittelwert
#define WAIT 3        //Dauer in s zwischen einzelnen Pings
#define ANZ_STEPS1   (60 / WAIT)         //Historie minütlich = 2 Std
#define ANZ_STEPS2   (3600 / WAIT)       //Historie stündlich = 5 Tage
#define ANZ_STEPS3   (24 * 3600 / WAIT)  //Historie täglich = 4 Monate
#define ANZ_STEPS4   2                   //Historie test
#define VIEW_STEPS   (9 / WAIT)         //Historie Toggle Intervall delay in sec.



//----- Definitionen für Display -----
#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

#include <Adafruit_SSD1306.h>
#include <splash.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//----- Globale Variablen -----
float val[ANZ_AVG];
byte a[5 * GR_LEN_X + 1];  //Messwerthistorie je Intervall
int xLevel_set = 150;  //Schwellwert ab dem gemessen werden soll in [mV]
unsigned long maxWait = 5000;
float cm, cm2, pegel, abstand;
unsigned long liter;
int pct;
int avg_cnt = 0;
int stepsX = 1;   //Variante für Messhistorie
int steps1 = 0;
int steps2 = 0;
int steps3 = 0;
int steps4 = 0;
int view_steps_i = 0;   //Variante für Messhistorie


//----- the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);


  //pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D_OUT_PING, OUTPUT);
  pinMode(D_IN_ECHO, INPUT);
  pinMode(S1, OUTPUT);


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(1000);
  display.clearDisplay();
  display.drawPixel(10,10,WHITE);
  display.display();

  
  // initialize digital pin LED_BUILTIN as an output.
}

//----- Hauptschleife -----
void loop() {
  unsigned long i, i0, i1, m1, m2;
  byte ping_ok;
  byte s1_on;

  display.setTextColor(WHITE);
  

  //----- Sensor aktivieren + auslesen
//  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(D_OUT_PING, HIGH);
  delay(10);
  digitalWrite(D_OUT_PING, LOW);
  m1 = micros();

  while (digitalRead(D_IN_ECHO) == LOW);  //Warten auf Antwort
  for (long i = 0; (i < maxWait) && (digitalRead(D_IN_ECHO) == HIGH) ; i++);
  m2 = micros();

  //Gleitenden Mittelwert berechnen
  cm = (m2 - m1 - CAL_N) / CAL_M;
  ping_ok = 1; 
  if (cm<0) { 
    cm = 0;
    ping_ok = 0; 
  }
  else if (cm > HOEHE) { 
    cm = HOEHE; 
    ping_ok = 0; 
  }

  val[avg_cnt] = cm;
  avg_cnt++;
  if (avg_cnt >= ANZ_AVG) { 
    avg_cnt = 0; 
  }
  
  cm2 = 0;
  for (int i = 0; i < ANZ_AVG; i++) {
    cm2 = cm2 + val[i];
  }


  //Anzeigewerte 
  abstand = cm2 / ANZ_AVG;
  pegel = HOEHE - abstand;
  pct = (100 * pegel) / PEGEL_MAX;
  liter = (314.0 * pegel) / 10;           //Grundfläche * Pegel in liter
  byte a1 = (GR_LEN_Y * pegel) / PEGEL_MAX;   //Für Grafik normiert

  //Nachspeisung abh. vom Pegel an-/abschalten
  if (pegel > PEGEL_S1_OFF) { 
    s1_on = 0; 
    digitalWrite(S1, LOW);
  }
  if (pegel < PEGEL_S1_ON && pegel > 0)  {
    s1_on = 1; 
    digitalWrite(S1, HIGH);
  }


  // Messwerthistorie je Intervall
  if (steps1 > 0) {
    steps1 --; 
  }  else {
    steps1 = ANZ_STEPS1 - 1;
    for (int i = 1; i < GR_LEN_X; i++) { a[i-1] = a[i]; }
    a[GR_LEN_X - 1] = a1;
  }

  if (steps2 > 0) {
    steps2 --; 
  }  else {
    steps2 = ANZ_STEPS2 - 1;
    for (int i = GR_LEN_X + 1; i < 2 * GR_LEN_X; i++) { a[i-1] = a[i]; }
    a[2 * GR_LEN_X - 1] = a1;
  }

  if (steps3 > 0) {
    steps3 --; 
  }  else {
    steps3 = ANZ_STEPS3 - 1;
    for (int i= 2 * GR_LEN_X + 1; i < 3 * GR_LEN_X; i++) { a[i-1] = a[i]; }
    a[3 * GR_LEN_X - 1] = a1;
  }

  if (steps4 > 0) {
    steps4 --; 
  }  else {
    steps4 = ANZ_STEPS4 - 1;
    for (int i= 3 * GR_LEN_X + 1; i < 4 * GR_LEN_X; i++) { a[i-1] = a[i]; }
    a[4 * GR_LEN_X - 1] = a1;
  }

  
  //Zum PC übertragen
  Serial.print("\ni = ");           Serial.print(i);
  Serial.print("  micros() = ");    Serial.print(long(m2 - m1));
  Serial.print("  cm = ");          Serial.print(cm);
  Serial.print("  abstand = ");     Serial.print(abstand);
  Serial.print("  pegel = ");       Serial.print(pegel);
  Serial.print("  a1 = ");          Serial.print(a1);
  Serial.print("  stepsX = ");      Serial.print(stepsX);


  // Ausgabe an Display: Text
  display.fillRect(0, 0, GR_LEN_X, GR_Y0, BLACK);
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print(liter);
  display.print(F(" Liter"));
  display.setTextSize(1);
  display.setCursor(0,20);
  display.print(F("Pegel: "));
  display.print(round(pegel));
  display.print(F(" cm   "));
  display.print(pct);
  display.print(F("%"));
  
  if (ping_ok == 1) {
    display.fillRect(GR_LEN_X - 1, 2, GR_LEN_X - 1, 2, WHITE);  
  }
  if (s1_on == 1) {
    display.setCursor(116, 25);
    display.setTextSize(2);
    display.print(F("+"));
  }

  
  // Ausgabe an Display: Grafik
  view_steps_i ++;
  if (view_steps_i > VIEW_STEPS) {
    view_steps_i = 1;    //Intervall erst mal laufend toggeln
    stepsX ++;
    if (stepsX > 4) {   //4=Test, 3=Normalbetrieb
      stepsX = 1;    //Intervall erst mal laufend toggeln
    }
  }
  display.setTextSize(1);
  display.setCursor(20,30);
  if (stepsX == 1) {
    display.print(F("(Stunden)"));
  }
  if (stepsX == 2) {
    display.print(F("(Tage)"));
  }
  if (stepsX == 3) {
    display.print(F("(Monate)"));
  }
  if (stepsX == 4) {
    display.print(F("(test)"));
  }

  if (stepsX == 1) {
    i0 = 0;
    i1 = 60;
  }
  if (stepsX == 2) {
    i0 = GR_LEN_X;
    i1 = 24;
  }
  if (stepsX == 3) {
    i0 = 2 * GR_LEN_X;
    i1 = 30;
  }
  if (stepsX == 4) {
    i0 = 3 * GR_LEN_X;
    i1 = 50;
  }

  display.drawLine(GR_X0, GR_Y0 - 2, GR_X1, GR_Y0 - 2, WHITE);
  display.fillRect(GR_X0, GR_Y0, GR_X1, GR_Y1, BLACK);  
  for (i = 0; i < GR_LEN_X; i++) {
    display.drawLine(i, GR_Y1 - a[i + i0], i, GR_Y1, WHITE);
  }
  for (int i = GR_LEN_X - i1; i > 0; i = i - i1) {
    display.drawLine(i, GR_Y0, i, GR_Y1, INVERSE);
  }

  
  display.display();
  delay(200);
  if (ping_ok == 1) {
    display.fillRect(GR_LEN_X - 1, 2, GR_LEN_X - 1, 2, INVERSE);  
  }
  display.display();

//  delay(10);
//  digitalWrite(LED_BUILTIN, LOW);

  delay(WAIT * 1000 - 200);
}


/*
  Versorgungsspannung: 5 Volt
  Stromaufnahme: < 2mA
  Triggerung: über fallende Flanke (TTL-Pegel, >= 10µs);
  nach ca. 250µs wird ein 40kHz Burst-Signal ausgesendet (t = ca. 200µs);
  danach geht der Echo-Pin Ausgang auf H-Pegel und wartet auf das Rücklaufsignal.
  Sobald dies eintrifft, geht der Echo-Pin auf L-Pegel.
  Nach ca. 20ms kann die nächste Messung erfolgen. Trifft kein Echosignal ein,
  bleibt der Echo-Pin für ca. 200ms auf H-Pegel.
  Messdistanz: 2 cm ... 300 cm
  Messungen pro Sekunde: max. 50
*/




/*
   5880 = 1,00 m
   3080 = 0,50 m
   4440 = 0,75 m
   1630 = 0,25 m
   -->
   50cm = 2800
   1m = 5600 + 280
*/
