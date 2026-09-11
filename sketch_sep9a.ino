#include <string.h>
#define mqpin A0//pin analogique 
#define rl 20.0//resistance interne du capteur 
#define cleanair 3.6//variation rs/ro en air pur 
//parametre de la courbe avec log-log
#define alcohol_cuve_a 77.25
#define alcohol_cuve_b -3.18

#define NB_ECHANTILLONS 50 //seuil normal du ppm en air pur 
//le ppm est la partie d'air par million il permet d'avoir la concentration de l'alcool par million d'air inspire 
//ppm = a* (rs/ro)^b
const float seuilPPM = 50.0;//seuil normal de ppm 
float Ro = 10.0;//valeur de la resistance Ro en air pure 


float readRS(){ //cette fonction permet de determiner la valeur de rs 
    int raw =analogRead(mqpin);//lecteur de la valeu de la tension 
    float volt = raw *(5.0/ 1023.0);//conversion de la valeur analogique en valeur mumeriaue 10bits
    if (volt ==0) volt ==0.01;//evite les division par zero
    float rp = 5.0 * rl;//detrermination de la tension en fonction 
    float RS = ( rp/ volt) - rl;//determination de la resistance 
    return RS;//valeur de rs stocke en memoir
    
} 
float calibrateRo(){//cette fonction permet d'avoior la valeur reel de Ro
  Serial.println("calibrage de Ro en air pur...");//affiche dans le moniteur serie calibrage de R0 en air pur...
  float rsSum = 0;//initialisation des valeur de rs pour le calibrage 
  for (int i = 0 ; i < NB_ECHANTILLONS; i++ ){//boucle for pour incrementer la valeur de rssum
    rsSum += readRS();//ajoute la valeur incrementer a la valeur de rs lue 
    delay(2000);//delai de 2000ms
  }
  float rsA = rsSum / 50.0;//ration rs / 50.0 avec 50.0 le ppm en air pur 
  float ro = rsA / cleanair;//determination de la valeurt reel de ro au moment de l'analyse
  Serial.println("valeur de Ro :");//affiche la valeur vde ro dans le moniteur serie
  Serial.println(ro);
  return ro;//stocke ro en memoir 
}

float getAlcoholppm(){//cette fonction permet d'avoir la quantite d'alcool en ppm 
  float rs = readRS();//lis la valeur de rs 
  float ratio = rs / Ro;//fais le ration rs /ro 
  float ppm =  alcohol_cuve_a * pow(ratio, alcohol_cuve_b);//determine la valeur de la quantite d'alcool en ppm
  return ppm;//stocke ppm en memoir 
}


void setup() {//initialiation des differentes variables 
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);//connexion entre l'ordinateur et l'arduino
  Serial.println("chauffage du capteur 20s...");//affiche dans le moniteur serie chauffage du capteur 20s...
  delay(200);//delai de 200ms
  Ro = calibrateRo();//valeur de Ro avec appel de la fonction calibrateRo() 
  Serial.println(Ro); // affiche la valeur de ro dans le moniteur serie 

}
String etat(float ppm){
  #define buzzer 8//cette fonction permet de determiner clairement l'etat d'ivresse de la personne 
  if (ppm <100.0 ){
    noTone(buzzer);//si le ppm est < 100.0 affiche sobre 
    return "Sobre";
  }
  else if (ppm <250 ){//si le ppm est <250 affiche trace d'alcool detecte
    noTone(buzzer); 
    return "trace d'alcool detecte";
  }
  else if (ppm < 450){
    tone(buzzer, 1000);//si le ppm est <450 affiche seil legal 
    return "seil legal ";
  }
  else if (ppm <1000){//si le ppm est <1000 affiche ivresse...
    tone(buzzer, 2000);
    return "ivresse...";
  }
  else {//sinon affiche intoxication severe 
    tone(buzzer ,3000);
    return "intoxication severe";
  }
}

void loop() {
  reset_system();
  //fonction qui permet l'execution du programme  
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);  
  float ppm =getAlcoholppm();//appel de la fonction getAlcoholppm()
  String ivresse = etat(ppm);//appel de la fonction etat(ppm) avec ppm comme arguments de la fonction 
  Serial.println("estimation d'alcool:");//affiche estimation d'alcool: dqns le moniteur serie 
  Serial.println(ppm);//affiche la valeur de la variable ppm dans le moniteur serie 
  Serial.println("ppm");// affiche la chaine ppm 
  Serial.println(ivresse);//afficher ivresse 
  delay(1000);//delai de 1000ms 

  // put your main code here, to run repeatedly:

}

void reset_system(){
  #define buttom 7
  if (digitalRead(buttom) == LOW) {
    Serial.println("reset demande");
    noTone(buzzer);
    Ro = calibrateRo();
    delay(300);
  }
}
