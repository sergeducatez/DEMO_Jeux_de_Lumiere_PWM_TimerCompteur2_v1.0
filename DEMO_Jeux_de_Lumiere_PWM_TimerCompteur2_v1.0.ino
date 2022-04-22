//**************************************************************************************************************************************************************************
// DEMO Jeux de Lumière PWM 12 LED avec le Timer/Compteur2 - v1.0 - Carte Nano (com28)
//**************************************************************************************************************************************************************************
// Auteur : Serge Ducatez 04/2022
//**************************************************************************************************************************************************************************
// https://www.youtube.com/channel/UCyGEFYW18IZKpe4uPgp9b8g/videos
// https://www.facebook.com/profile.php?id=100077558149994
//**************************************************************************************************************************************************************************
// Configurez l'affichage de l'IDE à 150%.
//**************************************************************************************************************************************************************************
// But :
// -----
// Faire varier la luminosité de 12 LED avec des valeurs de seuils PWM initiales différentes pour chaque LED.
// Ces valeurs de seuils PWM sont placées dans 2 tableaux :
// Tableau des seuils PWM initiaux du PORTD (D2 à D7).
// Tableau des seuils PWM initiaux du PORTB (D8 à D13).
// Les valeurs des seuils PWM peuvent être choisies par l'utilisateur ou de façon aléatoire.
// Configuration du Timer/Compteur2 :
// ----------------------------------
// Mode 3 : Fast PWM 8 bits => WGM22 = 0, WGM21 = WGM20 = 1 (TOP = 0xFF).
// Pré-diviseur de fréquence : N = 8 => CS22 = CS20 = 0, CS21 = 1.
// Autorisation des interruptions de débordement du compteur (TCNT2 = TOP = 0xFF) => TOIE2 = 1.
// Autorisation des interruptions de correspondance de comparaison (TCNT2 = OCR2A) => OCIE2A = 1.
// Les sorties OC2A et OC2B fonctionnent normalement => COM2A1 = COM2A0 = COM2B1 = COM2B0 = 0.
// OCR2A = 9.
// ISR exécutée lors de l'interruption déclenchée par la correspondance de comparaison entre TCNT2 et OCR2A du Timer/Compteur2 :
// -----------------------------------------------------------------------------------------------------------------------------
// L'ISR exécutée par le déclenchement de l'interruption lorsque TCNT2 = OCR2A positionne à "1" ou à "0" les sorties des LED en fonction :
// D'un compteur de correspondances de comparaison entre TCNT2 et OCR2A.
// Et de la valeur courante du seuil PWM de chaque LED.
// Si compteur < Seuil  => Sortie = "1".
// Si compteur >= Seuil => Sortie = "0".
// ISR exécutée lors de l'interruption déclenchée par le débordement du Timer/Compteur2 :
// --------------------------------------------------------------------------------------
// L'ISR exécutée lors du débordement du compteur (TOP = 0xFF) permet de faire varier les valeurs des seuils PWM :
// Si le compteur de correspondances de comparaison a atteint une certaine valeur max :
// Le compteur de correspondances de comparaison est réinitialisé à "0".
// Les seuils PWM sont incrémentés de "1" s'ils sont inférieurs à une certaine valeur max de seuil.
// Les seuils PWM sont réinitialisés à "0" s'ils sont supérieur à cette valeur max de seuil.
// Les seuils PWM peuvent être redéfinis aléatoirement s'ils sont supérieur à cette valeur max de seuil.
// Calculs :
// ---------
// Fckl_I/O = Fcpu si les bits CLKPS3 = CLKPS2 = CLKPS1 = CLKPS0 = 0 du registre "CLKPR" et si le fusible "CKDIV8" n'est pas programmé (= 1).
//
// Rappel : N = 8.
//
// F_TC2 = Fcpu / N => T_TC2 = 1 / (Fcpu / N) => T_TC2 = N / Fcpu => F_TC2 = 2MHz et T_TC2 = 0.5µs
// => Fréquence et période pour que TCNT2 = TCNT2 + 1 (Fréquence et période du PAS (Tick)).
//
// F_OVF = (Fcpu / N) / (TOP + 1) = F_OVF = Fcpu / (N x 256) => T_OVF = (N x 256) / Fcpu => F_OVF = 7812.5Hz et T_OVF = 128µs.
// => Fréquence et période de débordement du Timer/Compteur2 en mode 3 Fast PWM de 0x00 à 0xFF (TOP).
//
// F_ISRCOMPA = (Fcpu / N) / (OCR2A + 1) = F_ISRCOMPA = Fcpu / (N x (OCR2A + 1)) => T_ISRCOMPA = (N x (OCR2A + 1)) / Fcpu => F_ISRCOMPA = 200KHz et F_ISRCOMPA = 5µs.
// => Fréquence et période d'appel de l'ISR de correspondance de comparaison de l'unité de capture de sortie A.
//
// Période et Fréquence de changement de la valeur des seuils PWM :
// T = Compteur de correspondances de comparaisons maximum * 128µs :
// Ex : T = CompteurCOMPA_MAX * 128µs => T = 10.24ms et F = 97.656Hz.
//
// Période et Fréquence pour une variation des seuils PWM de "0" à "100" :
// T = Compteur de correspondances de comparaisons maximum * 128µs * 100 :
// Ex : T = CompteurCOMPA_MAX * 128µs * 100 => T = 1024ms et F = 0.976Hz.
//**************************************************************************************************************************************************************************
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Paramètres modifiables
//**************************************************************************************************************************************************************************
volatile int CompteurCOMP2A = 0;                                                                            // Compteur de correspondances de comparaison du Timer/Compteur2
volatile int CompteurCOMPA_MAX = 100; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Compteur de correspondances de comparaison maximum du Timer/Compteur2

//volatile int TableauSeuilsPDx [] = {0, 0, 0, 0, 0, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM aléatoires du PORTD (Variation PWM aléatoire)
//volatile int TableauSeuilsPBx [] = {0, 0, 0, 0, 0, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM aléatoires du PORTB

//volatile int TableauSeuilsPDx [] = {10, 70, 50, 40, 80, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTD (Variation PWM choisie par l'utilisateur)
//volatile int TableauSeuilsPBx [] = {65, 30, 5, 25, 80, 75}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTB

//volatile int TableauSeuilsPDx [] = {0, 8, 16, 24, 32, 40}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTD (Variation PWM droite à gauche)
//volatile int TableauSeuilsPBx [] = {48, 56, 64, 72, 80, 88}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTB

//volatile int TableauSeuilsPDx [] = {88, 80, 72, 64, 56, 48}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTD (Variation PWM gauche à droite)
//volatile int TableauSeuilsPBx [] = {40, 32, 24, 16, 8, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTB

//volatile int TableauSeuilsPDx [] = {0, 8, 16, 24, 32, 40}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTD (Variation PWM du centre vers les extrémités)
//volatile int TableauSeuilsPBx [] = {40, 32, 24, 16, 8, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTB

volatile int TableauSeuilsPDx [] = {40, 32, 24, 16, 8, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTD (Variation PWM du des extrémités vers le centre)
volatile int TableauSeuilsPBx [] = {0, 8, 16, 24, 32, 40}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTB

//volatile int TableauSeuilsPDx [] = {40, 32, 24, 16, 8, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTD (Variation PWM gauche à droite du PORTD)
//volatile int TableauSeuilsPBx [] = {40, 32, 24, 16, 8, 0}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTB (Variation PWM gauche à droite du PORTB)

//volatile int TableauSeuilsPDx [] = {0, 8, 16, 24, 32, 40}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTD (Variation PWM droite à gauche du PORTD)
//volatile int TableauSeuilsPBx [] = {0, 8, 16, 24, 32, 40}; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> // Tableau des seuils PWM initiaux du PORTB (Variation PWM droite à gauche du PORTB)

//**************************************************************************************************************************************************************************
//**************************************************************************************************************************************************************************
void setup()
{
  DDRD |= 0b11111100;                                                                                       // Configure les broches D2 à D13 en sorties
  DDRB |= 0b00111111;
  
  /*for (int indice = 0; indice < 6; indice++) // *********************************************************** // Parcourt les tableaux des seuils aléatoires PWM du PORTD et du PORTB
  {
    TableauSeuilsPDx[indice] = random(0, 50);                                                               // Définit un seuil PWM aléatoire initial de rang "indice" du tableaux des seuils PWM du PORTD
    delay(1);                                                                                               // Temporise 1ms
    TableauSeuilsPBx[indice] = random(0, 50);                                                               // Définit un seuil PWM aléatoire initial de rang "indice" du tableaux des seuils PWM du PORTB
    delay(1);                                                                                               // Temporise 1ms
  }*/
  
  cli();                                                                                                    // Désactive les interruptions globales
  
  // Focnx = 7812.5Hz => T = 128µs (F = 16000000 / (256 * 8))
  
  TCCR2A = 0b00000000;                                                                                      // Initialise le registre "TCCR2A" du Timer/Compteur2
  TCCR2B = 0b00000000;                                                                                      // Initialise le registre "TCCR2B" du Timer/Compteur2
  
  TIFR2  = 0b00000111;                                                                                      // Réinitialise le registre "TIFR2" du Compteur/Timer2 pour effacer toutes les interruptions en suspens
  TCCR2A = bit(WGM21) | bit(WGM20);                                                                         // Configure le mode 3 Fast PWM 8 bits du Timer/Compteur2 (TOP = 0xFF)
  TIMSK2 = bit(OCIE2A);                                                                                     // Active l'interruption déclenchée par la correspondance de comparaison du Timer/Compteur2 (TCNT2 = OCR2A)
  TIMSK2 |= bit(TOIE2);                                                                                     // Active l'interruption déclenchée par le débordement du Timer/Compteur2 (TCNT2 = MAX = 0xFF)
  TCNT2 = 0;                                                                                                // Initialise le registre "TCNT2" du Timer/Compteur2
  OCR2A = 9;                                                                                                // Charge la valeur "9" dans le registre "OCR2A" du Timer/Compteur2
  TCCR2B = bit(CS21);                                                                                       // Définit le pré-diviseur de fréquence N égal à "8" du Timer/Compteur2 => Démarre le Timer/Compteur2
  
  sei();                                                                                                    // Active les interruptions globales
}

//**************************************************************************************************************************************************************************
//**************************************************************************************************************************************************************************
void loop()
{

//**************************************************************************************************************************************************************************
//**************************************************************************************************************************************************************************
}

//**************************************************************************************************************************************************************************
//*** Fonction ISR exécutée lors de l'interruption déclenchée par le débordement du Timer/Compteur2 ************************************************************************
//**************************************************************************************************************************************************************************
ISR (TIMER2_OVF_vect)
{
  if (CompteurCOMP2A == CompteurCOMPA_MAX) // ************************************************************* // Si le compteur de correspondances de comparaison du Timer/Compteur2 est égal au compteur de correspondances de comparaison maximum du Timer/Compteur2
  {
     CompteurCOMP2A = 0;                                                                                    // Réinitialise le compteur de correspondances de comparaison du Timer/Compteur2
     
     //CompteurCOMPA_MAX = random(50, 100);                                                                   // Définit une valeur aléatoire du compteur de correspondances de comparaison maximum du Timer/Compteur2
     
     for (int PDx = 2; PDx < 8; PDx++) // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ // Parcourt le tableau des seuils PWM du PORTD
     {
       TableauSeuilsPDx[PDx - 2]++;                                                                         // Incrémente le seuil PWM de rang "PDx - 2" du tableaux des seuils PWM du PORTD
       //if (TableauSeuilsPDx[PDx - 2] > 100) {TableauSeuilsPDx[PDx - 2] = 0;} // --------------------------- // Si le seuil PWM de rang "PDx - 2" est supérieur à "100" => Réinitialise le seuil PWM de rang "PDx - 2" du tableaux des seuils PWM du PORTD
       if (TableauSeuilsPDx[PDx - 2] > 150) {TableauSeuilsPDx[PDx - 2] = 0;} // --------------------------- // Si le seuil PWM de rang "PDx - 2" est supérieur à "150" => Réinitialise le seuil PWM de rang "PDx - 2" du tableaux des seuils PWM du PORTD
       //if (TableauSeuilsPDx[PDx - 2] > 100) {TableauSeuilsPDx[PDx - 2] = random(0, 50);} // --------------- // Si le seuil PWM de rang "PDx - 2" est supérieur à "100" => Définit un seuil PWM aléatoire de rang "PDx - 2" du tableaux des seuils PWM du PORTD
       //if (TableauSeuilsPDx[PDx - 2] > random(100, 150)) {TableauSeuilsPDx[PDx - 2] = random(0, 50);} // -- // Si le seuil PWM de rang "PDx - 2" est supérieur à une valeur aléatoire => Définit un seuil PWM aléatoire de rang "PDx - 2" du tableaux des seuils PWM du PORTD
       //if (TableauSeuilsPDx[PDx - 2] > random(100, 150)) {TableauSeuilsPDx[PDx - 2] = 0;} // -------------- // Si le seuil PWM de rang "PDx - 2" est supérieur à une valeur aléatoire => Réinitialise le seuil PWM de rang "PDx - 2" du tableaux des seuils PWM du PORTD
     }

     for (int PBx = 0; PBx < 6; PBx++) // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ // Parcourt le tableau des seuils PWM du PORTB
     {
       TableauSeuilsPBx[PBx]++;                                                                             // Incrémente le seuil PWM de rang "PBx" du tableaux des seuils PWM du PORTB
       //if (TableauSeuilsPBx[PBx] > 100) {TableauSeuilsPBx[PBx] = 0;} // ----------------------------------- // Si le seuil PWM de rang "PBx" est supérieur à "100" => Réinitialise le seuil PWM de rang "PBx" du tableaux des seuils PWM du PORTB
       if (TableauSeuilsPBx[PBx] > 150) {TableauSeuilsPBx[PBx] = 0;} // ----------------------------------- // Si le seuil PWM de rang "PBx" est supérieur à "150" => Réinitialise le seuil PWM de rang "PBx" du tableaux des seuils PWM du PORTB
       //if (TableauSeuilsPBx[PBx] > 100) {TableauSeuilsPBx[PBx] = random(0, 50);} // ----------------------- // Si le seuil PWM de rang "PBx" est supérieur à "100" => Définit un seuil PWM aléatoire de rang "PBx" du tableaux des seuils PWM du PORTB
       //if (TableauSeuilsPBx[PBx] > random(100, 150)) {TableauSeuilsPBx[PBx] = random(0, 50);} // ---------- // Si le seuil PWM de rang "PBx" est supérieur à une valeur aléatoire => Définit un seuil PWM aléatoire de rang "PBx" du tableaux des seuils PWM du PORTB
       //if (TableauSeuilsPBx[PBx] > random(100, 150)) {TableauSeuilsPBx[PBx] = 0;} // ---------------------- // Si le seuil PWM de rang "PBx" est supérieur à une valeur aléatoire => Réinitialise le seuil PWM de rang "PBx" du tableaux des seuils PWM du PORTB
     }
  }
}

//**************************************************************************************************************************************************************************
//*** Fonction ISR exécutée lors de l'interruption déclenchée par la correspondance de comparaison entre TCNT2 et OCR2A du Timer/Compteur2 *********************************
//**************************************************************************************************************************************************************************
ISR (TIMER2_COMPA_vect)
{
  CompteurCOMP2A++;                                                                                         // Incrémente le compteur de correspondances de comparaison du Timer/Compteur2
  
  for (int PDx = 2; PDx < 8; PDx++) // ******************************************************************** // Parcourt les broches D2 à D7
  {
    if (CompteurCOMP2A < TableauSeuilsPDx[PDx - 2]) // ++++++++++++++++++++++++++++++++++++++++++++++++++++ // Si le compteur de correspondances de comparaison du Timer/Compteur2 est inférieur au seuil PWM de rang "PDx - 2"
    {
      PORTD |= (1 << PDx);                                                                                  // Allume la LED de rang "PDx"
    }
    else if (CompteurCOMP2A >= TableauSeuilsPDx[PDx - 2]) // ++++++++++++++++++++++++++++++++++++++++++++++ // Si le compteur de correspondances de comparaison du Timer/Compteur2 est supérieur ou égal au seuil PWM de rang "PDx - 2"
    {
      PORTD &= ~(1 << PDx);                                                                                 // Eteint la LED de rang "PDx"
    }
  }
  
  for (int PBx = 0; PBx < 6; PBx++) // ******************************************************************** // Parcourt les broches D8 à D13
  {
    if (CompteurCOMP2A < TableauSeuilsPBx[PBx]) // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++ // Si le compteur de correspondances de comparaison du Timer/Compteur2 est inférieur au seuil PWM de rang "PBx"
    {
      PORTB |= (1 << PBx);                                                                                  // Allume la LED de rang "PBx"
    }
    else if (CompteurCOMP2A >= TableauSeuilsPBx[PBx]) // ++++++++++++++++++++++++++++++++++++++++++++++++++ // Si le compteur de correspondances de comparaison du Timer/Compteur2 est supérieur ou égal au seuil PWM de rang "PBx"
    {
      PORTB &= ~(1 << PBx);                                                                                 // Eteint la LED de rang "PBx"
    }
  }
}
