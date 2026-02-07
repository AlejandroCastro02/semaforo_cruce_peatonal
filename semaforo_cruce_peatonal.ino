//DDRB maneja de pin D13 a D8  -- -- D13 D12 D11 D10 D9 D8
/* 
D7 - BOTON
D8 - ROJO
D9 - AMARILLO
D10 - VERDE
*/
volatile bool solicitudPeatonal = false; //Variable que cambiara mediante interrupciones

unsigned long tiempoActual;
unsigned long tiempoPrevio;

const unsigned long intervaloPeaton = 20000; // 20 seg
const unsigned long intervalo10s = 60000; // 60 seg
const unsigned long intervalo3s  = 5000;  // 5 seg
const unsigned long tiempominVERDE = 20000; // 20 seg

void setup() {
  DDRB |= B00000111;   //Define D10-D8 como salida

  DDRD &= ~B10000000; //Pin D7 como entrada
  PORTD |= B10000000; //Se establece pull up (en modo entrada)

  PCICR |= B00000100; //Activamos la interrumpciones para PuertoD
  PCMSK2 |= B10000000; //D7 activa la interrumpcion
}

//ISR para Puerto D
ISR (PCINT2_vect){    //Interrupcion del puertoD
  //Ejectura instruciones para PCINT16  PCINT23
  //Pines D0 a D7
  if (!(PIND & B10000000)) { //D7 en bajo
  solicitudPeatonal = true;
  }
}

enum semaforo {
  VERDE,
  AMARILLO,
  ROJO
};

semaforo estado = VERDE;

void loop() {

    tiempoActual = millis();

    switch (estado) {
      case VERDE:
      PORTB = B00000100;    //Verde en alto
      if ((tiempoActual - tiempoPrevio >= tiempominVERDE) && (solicitudPeatonal == true)){
        tiempoPrevio = tiempoActual;
        estado = AMARILLO;
      } else if (tiempoActual - tiempoPrevio >= intervalo10s) {
        tiempoPrevio = tiempoActual;
        estado = AMARILLO;
      }
      break;

      case AMARILLO:
      PORTB = B00000010;    //Amarillo en alto
      if (tiempoActual - tiempoPrevio >= intervalo3s) {
        tiempoPrevio = tiempoActual;
        estado = ROJO;
      }
      break;

      case ROJO:
      PORTB = B00000001;    //Rojo en alto
      if (solicitudPeatonal == true) {
        if (tiempoActual - tiempoPrevio >= intervaloPeaton) {
        tiempoPrevio = tiempoActual;
        solicitudPeatonal = false;
        estado = VERDE;
        }
      } else if (tiempoActual - tiempoPrevio >= intervalo10s) {
        tiempoPrevio = tiempoActual;
        estado = VERDE;
      }
      break;
      
    }

}
