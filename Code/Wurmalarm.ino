#include <Servo.h>
Servo dreh_servo;
Servo kipp_servo;
Servo laser_servo;

const uint16_t treffer_schwelle=500;                  //Schwellenwert für das Durchschalten der Fototransistoren
const uint16_t schwellen[4]={450, 750, 960, 1050};    //Schwellenwerte für die Schwierigkeitsstufenfeststellung bei 15, 4.7, 1.5 und 0 kOhm
const uint8_t anzahl[4]={5, 10, 15, 20};              //Anzahl Ziele je nach Schwierigkeitsstufe
const uint8_t m_dreh=92, m_kipp=114;                  //Mittelstellungswerte für Drehen und Kippen
const uint8_t laser_ein=84, laser_aus=98;             //Einstellwerte für Laserservo
uint16_t s_dreh, s_kipp, schwellen_wert;
const uint16_t dreh_min=80, dreh_max=96, kipp_min=104, kipp_max=118;    //Minimal- und Maximalwerte des Dreh- und Kippservos
uint8_t j, i_s, ziele[20], i_treffer, i_fehler, i_speich;
boolean ziel_anzeige=true;
uint32_t start_zeit, end_zeit; 
float spiel_zeit; 
#include <LiquidCrystal.h>
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
/*   digitale Pinbelegung:
 *   14-18  Ziel-Led´s 
 *   8-13   LCD
 *   7      Buzzer
 *   6      Laserservo 
 *   5      Kippservo 
 *   4      Drehservo
 *   3      Startknopf
 *   2      Spielmodus Folge/Memory
 *   analoge Pinbelegung:  
 *   A0     Schwierigkeitsstufe 1-4
 *   A1     Sensor für Kippbewegung
 *   A2     Sensor für Drehbewegung
 *   A3-A7  Fototransistoren
 *   A8     Rauschen für die Zufallzahlenerzeugung
 *   
*/
void setup() {
 pinMode(2, INPUT);             //Spielmodus
 pinMode(3, INPUT);             //Startknopf
 pinMode(7, OUTPUT);            //Buzzer
 for (uint8_t i=14; i<19; i++){ //Ziel-Led´s
  pinMode(i, OUTPUT);
 }
 //Startbildschirm
 lcd.begin(16, 2);
 lcd.clear();
 lcd.setCursor(0, 0);
 lcd.print("  WURMALARM!!!  ");
 lcd.setCursor(0,1);
 lcd.print("  Viel Erfolg!  ");
 //Starttonfolge
 for (uint8_t i=0; i<3; i++){
  tone(7,250);
  delay(500);
  noTone(7); 
 }
  tone(7,400);
  delay(600);
  noTone(7);
 //Kurzes Aufblinken aller LED´s rückwärts
  for (uint8_t i=18; i>13; i--) {
    digitalWrite(i, HIGH);
    delay(300);
    digitalWrite(i, LOW);
    delay(300);
  } 
  dreh_servo.attach(4);
  kipp_servo.attach(5);
  laser_servo.attach(6);
  laser_servo.write(laser_aus);       //Laser ausschalten  
  delay(3000);                        //Drei Sekunden warten
}
/*
 *        Spielschleife
 */
void loop() {
  digitalWrite(2, HIGH);            //Spielmodus zurücksetzen
  j=0;                              //Schwellenwertzähler zurücksetzen
  schwellen_wert=analogRead(A0);    //Wiederholungszahlen bestimmen
  while (schwellen[j]<schwellen_wert) {j +=1;} 
  Zufallszahlen(anzahl[j]);         //Reihenfolge der Ziele bestimmen
  i_treffer=0;                      //Trefferzähler zurücksetzen
  i_fehler=0;                       //Fehltrefferzähler zurücksetzen
  Servostartwerte();                //Durchschnittswerte für Startposition der Servos bestimmen
  ziel_anzeige=true;                //Für Modus "Memory" die Zielanzeige der LED´s aktivieren
  digitalWrite(3, HIGH);            //Starttaster zurücksetzen 
  lcd.clear();
  lcd.setCursor(0, 0);
  if (digitalRead(2) == HIGH) {lcd.print("Modus: Folge");}  //Spielmodus anzeigen
  else {lcd.print("Modus: Memory");}
  lcd.setCursor(0,1);
  lcd.print("Ziele: ");                                     //Anzahl Ziele anzeigen 
  lcd.print(anzahl[j]); 
  delay(100);
/*
 *          Spielbeginn
 */
  if (digitalRead(3) == LOW) {      //Starttaster gedrückt?   
    delay(200); 
    laser_servo.write(laser_ein);   //Laser einschalten
    start_zeit=millis();            //Startzeit speichern
/*     
 *          Spielmodus "Serie"
 */
    if(digitalRead(2) == HIGH) {   //Serienmodus 
      while (i_treffer+i_fehler<anzahl[j]) {
        digitalWrite(ziele[i_treffer+i_fehler]+13, HIGH);
        Zielen();
        Auswerten(i_treffer+i_fehler);
      } 
    }
/*    
 *          Spielmodus "Memory"
 */
    else {                        //Memory-Modus
      uint8_t ii=0, k;     
      while (i_fehler == 0 && ii < anzahl[j]) {   //Schleife für Fehlertest und Gesamtdurchlauf
        if (ziel_anzeige) {
          for (uint8_t i=0; i<ii+1; i++){   //Zielreihenfolge anzeigen
            digitalWrite(ziele[i]+13, HIGH);     
            delay(1000);
            digitalWrite(ziele[i]+13, LOW);  
            ziel_anzeige=false;                       
          }            
        }
        k=0;
        while (k <= ii && i_fehler == 0) {                  //Schleife für Fehlertest und Wiederholung bereits
          i_speich=i_treffer;                               //dargestellter und getroffener Ziele
          while (i_fehler == 0 && i_speich == i_treffer) {  //Schleife für Fehlertest und Prüfung des richtig getroffenen Zieles
            Zielen();          
            Auswerten(k);
          }
          if (i_treffer > i_speich) {k +=1;}                //Prüfung auf neuen Treffer
        }
        ii +=1;                                             //Durchlaufzähler erhöhen
      }
      if (i_fehler == 0) {i_treffer=ii;}                  
      else {
        i_treffer=ii-1;
        for (uint8_t i=0; i<3; i++){                        //Nach Fehler das richtige Ziel kurz blinkend anzeigen
          digitalWrite(ziele[i_treffer]+13, HIGH);
          delay(500);
          digitalWrite(ziele[i_treffer]+13, LOW);
          delay(500);      
        }                          
      }
    }
    end_zeit=millis(); 
    spiel_zeit=(end_zeit-start_zeit)*0.001;   //Spielzeit in Sekunden berechnen   
    dreh_servo.write(90);                     //Servos in Mittenposition bringen
    kipp_servo.write(111);        
    laser_servo.write(laser_aus); 
    lcd.clear();                              //Anzeige des Spielergebnisses
    lcd.setCursor(0, 0);
    lcd.print("Zeit: ");  
    lcd.print(spiel_zeit);
    lcd.setCursor(0,1);
    lcd.print("Tr: ");                                     //Anzahl Ziele anzeigen 
    lcd.print(i_treffer);
    lcd.setCursor(8,1);     
    lcd.print("F: ");
    lcd.print(i_fehler);
    for (uint8_t i=14; i<19; i++) {          //Kurz alle Ziel-LED´s zum Spielende aufleuchten lassen 
      digitalWrite(i, HIGH);
      tone(7,300);                           //Trefferton 
      delay(500);
      digitalWrite(i, LOW);
      noTone(7);    
      delay(500); 
    }
    digitalWrite(3, HIGH);
    while (digitalRead(3) == HIGH) {          //Auf Starttaster warten  
      delay (100);             
    }    
  }  
}
/*
 *    Ton für richtig getroffenes Ziel
 */
void Getroffen(){
  i_treffer +=1;  
  if (digitalRead(2) == LOW) {ziel_anzeige=true;}
  tone(7,300);
  delay(500);
  noTone(7); 
}  
/*
 *    Ton für falsch getroffenes Ziel
 */
void Fehltreffer() {
  i_fehler +=1;
  tone(7,100);
  delay(500);
  noTone(7);   
}
  
/*
 *    Zurücksetzen der Ziel-LED´s und Laserstellung
 */
void Zuruecksetzen() {
  laser_servo.write(laser_aus);   
  digitalWrite(ziele[i_treffer+i_fehler]+13, LOW); 
  dreh_servo.write(90);
  kipp_servo.write(111);  
  delay(500);
  laser_servo.write(laser_ein);   
}
/*
 *    Liegen Treffer oder Fehler vor?
*/       
void Auswerten(uint8_t lfd_nr) {
  if (analogRead(A3)>treffer_schwelle) {        //rotes Ziel
    Zuruecksetzen();
    if (ziele[lfd_nr]==1) {
      Getroffen();
    }
    else {
      Fehltreffer();
    }
  }
  if (analogRead(A4)>treffer_schwelle) {        //blaues Ziel
    Zuruecksetzen();
    if (ziele[lfd_nr]==2){
      Getroffen();    
    }
    else {
      Fehltreffer();
    }
  }  
  if (analogRead(A5)>treffer_schwelle) {        //gelbes Ziel
    Zuruecksetzen();
    if (ziele[lfd_nr]==3) {
      Getroffen();    
    }
    else {
      Fehltreffer();   
    }
  }  
  if (analogRead(A6)>treffer_schwelle) {        //grünes Ziel
    Zuruecksetzen();
    if (ziele[lfd_nr]==4) {
      Getroffen();    
    }
    else {
      Fehltreffer();
    } 
  }
  if (analogRead(A7)>treffer_schwelle) {        //weißes Ziel
    Zuruecksetzen();
    if (ziele[lfd_nr]==5) {
      Getroffen();    
    }
    else {
      Fehltreffer();
    }  
  }     
}
/*
 *      Steuerung der Servos zum Zielen
 */
void Zielen() {
  s_dreh=analogRead(A2);
  s_kipp=analogRead(A1);
  dreh_servo.write(map(s_dreh, 0, 1023, dreh_max, dreh_min));
  kipp_servo.write(map(s_kipp, 0, 1023, kipp_max, kipp_min));
}
/*
 *      Erzeugung der Zufallsreihenfolge der Ziele
*/
void Zufallszahlen(uint8_t anzahl) {
  uint8_t i=0;
  randomSeed(analogRead(A8));  
  while (i<anzahl) {
    ziele[i]=random(1,6);
    if (i>0 && ziele[i]==ziele[i-1]) {i -=1;}  //Ausschließen, dass zwei
    else {i +=1;}                              //aufeinanderfolgende Zahlen
 }                                            //gleich sind
}
/*
 *      Erzeugung der Startmittelwerte für die Servopositionen     
*/ 
void Servostartwerte(){
  dreh_servo.write(m_dreh);
  kipp_servo.write(m_kipp);
  } 
