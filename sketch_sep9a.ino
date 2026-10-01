#include <string.h>         
#include <LiquidCrystal.h>  

// Définition des broches et initialisation de l'écran LCD
const int rs = 11, en = 10, d4 = 5, d5 = 4, d6 = 3, d7 = 2; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  

#define mqpin A0            
#define rl 20.0             
#define cleanair 3.6        
#define buzzer 8            
#define buttom 9            

#define alcohol_cuve_a 77.25 
#define alcohol_cuve_b -3.18 
#define NB_ECHANTILLONS 50   

const float seuilPPM = 50.0; 
bool alarmeDeclenchee = false; // Permet de bloquer la sonnerie après 3 bips
float Ro = 10.0;               

// Chaîne globale pour reconstruire les commandes série sans bloquer le processeur
String commandeRecue = "";

void reset_system(); 
float readRS(); 
float calibrateRo(); 
float getAlcoholppm(); 
float ppmtogramperlitre(float ppm); 
String etat(float ppm);  
void ecouter_ordinateur();

// CORRECTION SYNTAXE : Retrait de la parenthèse fermante en trop
float readRS(){    
  int raw = analogRead(mqpin);   
  float volt = raw * (5.0 / 1023.0);   
  if (volt == 0) volt = 0.01;    
  float rp = 5.0 * rl;   
  float RS = (rp / volt) - rl;   
  return RS; 
}  

float calibrateRo(){   
  Serial.println("Calibrage de Ro en air pur...");   
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrage Ro...");
  
  float rsSum = 0;   
  for (int i = 0 ; i < NB_ECHANTILLONS; i++ ){     
    rsSum += readRS();     
    
    lcd.setCursor(0, 1);
    lcd.print("Etape: ");
    lcd.print(i + 1);
    lcd.print("/50");
    
    delay(100); 
  }   
  float rsA = rsSum / 50.0;   
  float ro = rsA / cleanair;   
  
  Serial.print("Valeur de Ro calculee : ");   
  Serial.println(ro);   
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ro calcule:");
  lcd.setCursor(0, 1);
  lcd.print(ro);
  delay(2000); 
  
  return ro; 
}  

float getAlcoholppm(){   
  float rs = readRS();   
  float ratio = rs / Ro;   
  float ppm = alcohol_cuve_a * pow(ratio, alcohol_cuve_b);   
  return ppm; 
}  

float ppmtogramperlitre(float ppm) {   
  float gram = ppm * 1000;   
  return gram;                   
} 

void setup() {   
  pinMode(LED_BUILTIN, OUTPUT);   
  pinMode(buzzer, OUTPUT);             
  pinMode(buttom, INPUT_PULLUP);          
  Serial.begin(9600); 
  
  // Configuration du nombre de colonnes et lignes du LCD
  lcd.begin(16, 2); 
  
  Serial.println("Chauffage du capteur 20s...");   
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Chauffage capteur");
  lcd.setCursor(0, 1);
  lcd.print("Veuillez attendre");
  delay(2000); 
  
  Ro = calibrateRo();   
  Serial.print("Initialisation Ro : ");   
  Serial.println(Ro);  
}  

String etat(float ppm){   
  if (ppm < 100.0) { 
    noTone(buzzer); 
    alarmeDeclenchee = false; 
    return "Sobre"; 
  }   
  else if (ppm < 250.0) { 
    noTone(buzzer); 
    alarmeDeclenchee = false; 
    return "Traces detectees"; 
  } 
  
  String libelle = "";
  if (ppm < 450.0)       libelle = "Seuil legal";   
  else if (ppm < 1000.0) libelle = "Ivresse...";   
  else                   libelle = "Intox severe";   

  if (alarmeDeclenchee == false) {
    for (int i = 0; i < 3; i++) {
      tone(buzzer, 1500); 
      delay(200);         
      noTone(buzzer);     
      delay(200);         
    }
    alarmeDeclenchee = true; 
  }

  return libelle; 
}

void loop() { 
  // Analyse immédiate et fluide du port série
  pinMode(buttom, INPUT_PULLUP);
  ecouter_ordinateur();  
  
  reset_system();       
  digitalWrite(LED_BUILTIN, HIGH);     
  delay(250);                         
  digitalWrite(LED_BUILTIN, LOW);      
  delay(250);      
  
  float ppm = getAlcoholppm();   
  float gram = ppmtogramperlitre(ppm); 
  String ivresse = etat(ppm);      
  
  Serial.print("DATA:");   
  Serial.print(ivresse);   
  Serial.print(";");   
  Serial.print(ppm);   
  Serial.print(";");   
  Serial.println(gram);      
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Etat: ");
  lcd.print(ivresse);
  
  lcd.setCursor(0, 1);
  lcd.print("PPM: ");
  lcd.print(ppm, 1); 
  lcd.print(" g/L: ");
  lcd.print(gram, 1);
  
  delay(1000); 
}  

void reset_system(){   
  if (digitalRead(buttom) == HIGH) {     
    Serial.println("Reset demande...");     
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Reset demande...");
    delay(1000);
    
    noTone(buzzer);     
    Ro = calibrateRo();     
    delay(300);   
  } 
}

// CORRECTION LOGIQUE : Réception non bloquante par accumulation de caractères
void ecouter_ordinateur() {
  while (Serial.available() > 0) {
    char c = Serial.read(); // Lit le caractère actuel sans attendre le suivant
    
    if (c == '\n') { // Fin de ligne détectée : la commande est complète
      commandeRecue.trim(); 

      if (commandeRecue == "STOP_BUZZER") {
        noTone(buzzer);
        Serial.println("RPT:Buzzer stoppe manuellement");
      }
      else if (commandeRecue == "SONNER") {
        alarmeDeclenchee = false; // Permet de relancer les 3 bips
        Serial.println("RPT:Demande de sonnerie recue");
      }
      else if (commandeRecue == "RECALIBRER") {
        Serial.println("RPT:Calibrage distant en cours...");
        Ro = calibrateRo();
      }
      else if (commandeRecue.length() > 0) {
        Serial.print("RPT:Commande inconnue (");
        Serial.print(commandeRecue);
        Serial.println(")");
      }
      
      commandeRecue = ""; // Réinitialise le tampon pour la prochaine commande
    } 
    else if (c != '\r') {
      commandeRecue += c; // Ajoute le caractère à la commande en cours (ignore le retour chariot Windows)
    }
  }
}
