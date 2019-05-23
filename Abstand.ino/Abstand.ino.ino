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

//----- Globale Variablen -----
  int aLen = 100;
  int xLevel_set = 150;  //Schwellwert ab dem gemessen werden soll in [mV]
  unsigned long maxWait = 3000;
  float cm;
  float cm2;

//----- the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  
  // initialize digital pin LED_BUILTIN as an output.
}

//----- Hauptschleife -----
void loop() {
  unsigned long i;
  unsigned long m1;
  unsigned long m2;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, INPUT);


  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(2, HIGH);
  delay(10);
//  noInterrupts();
  digitalWrite(2, LOW);
  m1 = micros();

  while(digitalRead(3) == LOW);
  for (long i = 0; (i < maxWait) && (digitalRead(3) == HIGH) ; i++);
  m2 = micros();
//  interrupts();

/*
 * 5880 = 1,00 m
 * 3080 = 0,50 m
 * 4440 = 0,75 m
 * 1630 = 0,25 m
 * -->
 * 50cm = 2800 
 * 1m = 5600 + 280 
 */


  digitalWrite(LED_BUILTIN, LOW);
  
  float m = 56.05;
  float n = 280.0;
  cm = (m2 - m1 - n) / m;
  if (cm2==0) { cm2 = cm; }
  cm2 = (9 * cm2 + cm) / 10;
  //Zum PC übertragen
  Serial.print("\ni = ");
  Serial.print(i);
  Serial.print("   micros() = ");
  Serial.print(long(m2 - m1));
  Serial.print("   cm = ");
  Serial.print(cm);
  Serial.print("   cm2 = ");
  Serial.print(cm2);

  delay(3000);
}
