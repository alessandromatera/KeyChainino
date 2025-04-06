#include "KeyChainino.h"

#define MATRIX_SIZE 144        // Dimensione del buffer della matrice (12x12)
#define STEPS 50               // Numero di passi per la spirale
#define ANGLE_STEP (M_PI / 8)  // Incremento angolare per la rotazione

byte buffer[MATRIX_SIZE];  // Buffer per un singolo frame

ISR(INT1_vect) {  //BUTTON A INTERRUPT
  //do nothing
}

ISR(INT0_vect) {  //BUTTON B INTERRUPT
  //do nothing
}

// Funzione per generare una spirale per un dato offset angolare
void generateSpiralFrame(uint8_t steps, float angleOffset) {
  const uint8_t centerX = 6;  // Centro X della matrice (12x12)
  const uint8_t centerY = 6;  // Centro Y della matrice (12x12)
  float angle, radius;
  bool validSpiral = true;

  // Pulisce il buffer
  memset(buffer, 0, MATRIX_SIZE);

  for (uint8_t i = 0; i < steps; i++) {
    radius = i * 0.5;                         // Incremento lineare del raggio
    angle = -(i * (M_PI / 8)) + angleOffset;  // Incremento angolare antiorario

    // Calcola le coordinate
    int x = round(centerX + radius * cos(angle));
    int y = round(centerY + radius * sin(angle));

    // Controlla se il punto è fuori matrice
    if (x < 0 || x >= 12 || y < 0 || y >= 12) {
      validSpiral = false;  // Se un punto cade fuori, ferma il resto della spirale
    }

    // Memorizza nel buffer solo se valido
    if (validSpiral) {
      buffer[y * 12 + x] = 1;  // Mappa 2D in buffer lineare
    }
  }
}

// Funzione per aggiornare la spirale in loop
void updateSpiral() {
  static float angleOffset = 0;  // Offset iniziale per la rotazione

  // Genera la spirale per il frame corrente
  generateSpiralFrame(STEPS, angleOffset);

  // Disegna il frame sulla matrice
  KC.drawBitmap(buffer);
  KC.display();

  // Incrementa l'offset per la rotazione
  angleOffset += ANGLE_STEP;

  // Controlla se entrambi i tasti sono premuti per andare in sleep
  if (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {
    delay(50);  // Debounce
    if (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {
      while (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {
        //do nothing
      }
      //delay(500);
      KC.goSleep();  // Va in modalità sleep
      //delay(500);
      while (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {
        //do nothing
      }
    }
  }
}

void setup() {
  KC.init();     // Inizializza la matrice LED
  KC.clear();    // Pulisce la matrice all'avvio
  KC.goSleep();  // Il dispositivo parte in modalità sleep

  // Configura i pulsanti
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
}

void loop() {
  updateSpiral();
}
