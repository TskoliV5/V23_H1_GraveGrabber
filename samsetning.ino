#include "tdelay.h" // sambærilegt og import í python
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include <Servo.h>

// ---------- Servo mótorar ----------
Servo servo1;
const int S1 = 7; // pinninn
const int S1_MAX = 180; // hversu mikið á servo-inn að hreyfast
const int S1_MIN = 0;

// ---------- Ultrasonic skynjarinn ----------
const int ECHO = 2;
const int TRIG = 3;

int fjarlaegd(); // fall sem sér um mælinguna, útfært hér fyrir neðan

// ---------- DC Mótorinn (L298N) ----------
const int HRADI = 6; // Verður að vera PWM pinni
const int STEFNA_A = 5;
const int STEFNA_B = 4;

const int HRATT = 127; // skilgreini hraða
const int HAEGT = 65;

/*  motórinn á að bíða í 1 sek
    síðan að fara áfram HRATT í 2 sek
    síðan að stoppa í 1 sek
    síðan að fara áfram HAEGT í 4 sek 
    síðan að stoppa í 1 sek
    síðan að fara áfram HRATT í 6 sek
    síðan að bakka og endurtaka */
// bý til senu (e. scene) með viðburðum
// ákveð: 0 er stopp, 1 er áfram hratt og -1 er áfram hægt
int sena[] = {0, 0, 1, 0, 1, 0, 1};
// hversu lengi á hver viðburður að vara
int senu_timi[] = {1000, 2000, 2000, 2000, 4000, 2000, 6000}; 
// teljari sem veit hvar senan er stödd
int senu_teljari = 0;
// breyta sem veit hversu margir viðburðir eru í senunni
const int FJOLDI_VIDBURDA = 7;

TDelay motor_delay(2000); // hversu lengi er fyrsti viðburður

// Föll sem stjórna hraða og átt, útfærð hér fyrir neðan
void afram(int hradi);
void stoppa();

// ---------- MP3 spilarinn ----------
SoftwareSerial mySoftwareSerial(11, 12);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// --------------- LED perur -----------------
const int LED1 = 7; // breytur fyrir led perurnar
const int LED2 = 8;

bool LED1_on = true; // stöðubreytur, halda utan um hvort kveikt eða slökkt er á perunum
bool LED2_on = true;

TDelay led1_delay; // "delay" breytur fyrir hverja peru
TDelay led2_delay;

int lagmarks_bidtimi = 250;
int hamarks_bidtimi = 750;

bool syning_i_gangi = false;

void setup() {
  // Servo
  servo1.attach(S1); 
  servo1.write(S1_MIN);
  
  // Ultrasonic 
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
   
  // DC mótorinn 
  pinMode(HRADI, OUTPUT);
  pinMode(STEFNA_A, OUTPUT);
  pinMode(STEFNA_B, OUTPUT);
  stoppa(); // upphafsstaðan, verður stopp þar til annað er ákveðið 

  // LED
  pinMode(LED1, OUTPUT); // skilgreina hvernig pinninn virkar sem peran er tengd í
  pinMode(LED2, OUTPUT);
  
  randomSeed(analogRead(A0)); // frumstilla random fallið

  led1_delay.setBidtimi(random(lagmarks_bidtimi, hamarks_bidtimi)); // setja upphafsbiðtíma fyrir hverja peru
  led2_delay.setBidtimi(random(lagmarks_bidtimi, hamarks_bidtimi));

  // MP3 spilarinn
  mySoftwareSerial.begin(9600);  // samskiptin
  // Use softwareSerial to communicate with mp3.
  if (!myDFPlayer.begin(mySoftwareSerial)) {  
    while(true);
  }
}

void loop() {
  if(fjarlaegd() < 50 || syning_i_gangi) {
        syning_i_gangi = true;
        // keyra dc í gang
        if(motor_delay.timiLidinn() == true) {
            senu_teljari = (senu_teljari + 1) % FJOLDI_VIDBURDA;
            motor_delay.setBidtimi(senu_timi[senu_teljari]);
            if (senu_teljari == 0) {
              syning_i_gangi = false;
              LED1_on = false;
              LED2_on = false;
            }
        }
        if(sena[senu_teljari] == 1) {
            afram(HRATT);
            servo1.write(S1_MIN);
        } else if(sena[senu_teljari] == -1) {
            afram(HAEGT);
            servo1.write(S1_MIN);
        } else {
            stoppa();
            myDFPlayer.volume(30);  // Set volume value. From 0 to 30
            myDFPlayer.play(1);     // Play the first mp3

            servo1.write(S1_MAX);
            
            // byrja að sweep-a servo-a
            //if(s1_delay.timiLidinn()) {
            //    if(s1_upp) s1_stefna++;
            //    else s1_stefna--;
            //    if(s1_stefna <= S1_MIN || s1_stefna >= S1_MAX) s1_upp = !s1_upp;
            //}
        }
      
        if(led1_delay.timiLidinn()) { // ef biðtiminn er liðinn
          LED1_on = !LED1_on; // víxla (e. toggle) stöðubreytu perunnar
          led1_delay.setBidtimi(random(lagmarks_bidtimi, hamarks_bidtimi)); // setja nýjan random biðtíma
        }
      
        if(led2_delay.timiLidinn()) {
          LED2_on = !LED2_on;
          led2_delay.setBidtimi(random(lagmarks_bidtimi, hamarks_bidtimi));
        }
      
        digitalWrite(LED1, LED1_on); // kveikir eða slekkur á perunni true jafngildir HIGH (5V)
        digitalWrite(LED2, LED2_on); // false jafngildir LOW (0V)
  }
}

int fjarlaegd() {
    // sendir út púlsa
    digitalWrite(TRIG,LOW);
    delayMicroseconds(2); // of stutt delay til að skipta máli
    digitalWrite(TRIG,HIGH);
    delayMicroseconds(10); // of stutt delay til að skipta máli
    digitalWrite(TRIG,LOW);

    // mælt hvað púlsarnir voru lengi að berast til baka
    return (int)pulseIn(ECHO,HIGH)/58.0; // deilt með 58 til að breyta í cm
}

void afram(int hradi) {
    digitalWrite(STEFNA_A, HIGH);
    digitalWrite(STEFNA_B, LOW);
    analogWrite(HRADI, hradi);
}

void stoppa() {
    digitalWrite(STEFNA_A, LOW);
    digitalWrite(STEFNA_B, LOW);
    analogWrite(HRADI, 0);
}
