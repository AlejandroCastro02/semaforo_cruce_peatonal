#include <MD_MAX72xx.h>
#include <SPI.h>
#include <avr/pgmspace.h>

// ================= MATRIX CONFIG =================
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 1
#define CS_PIN 10

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// ================= 74HC595 =================
#define SR_DATA  2   // SDI
#define SR_CLK   3   // SCLK
#define SR_LATCH 4   // LOAD

// Tabla para display 7 segmentos (común cátodo)
const byte numeros[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

// ================= LEDS =================
#define ROJO_LED     PB0  // D8
#define AMARILLO_LED PB1  // D9
#define VERDE_LED    PD6  // D6

// ================= TIEMPOS =================
const unsigned long T_VERDE_NORMAL = 60000;
const unsigned long T_VERDE_MIN    = 20000;
const unsigned long T_AMARILLO     = 5000;
const unsigned long T_ROJO_PEATON  = 20000;
const unsigned long T_FRAME        = 100;

volatile bool solicitudPeatonal = false;

unsigned long tiempoCambio = 0;
unsigned long tiempoFrame = 0;
unsigned long tiempoConteo = 0;

int contador = 0;
uint8_t frameActual = 0;

enum Semaforo { VERDE, AMARILLO, ROJO };
Semaforo estado = VERDE;

// ================= BITMAPS =================
const uint8_t quieto[8] PROGMEM = {
  B00111000,B00111000,B00010000,B01111100,
  B00010000,B00010000,B00101000,B01000100
};

const uint8_t caminar[4][8] PROGMEM = {
{
  B00110000,B00110000,B00011100,B01111010,
  B00011000,B00100100,B00100010,B00100000
},
{
  B00110000,B00110000,B00011000,B00011000,
  B00111000,B00011000,B00011100,B00010000
},
{
  B00110000,B00110000,B00011000,B00011000,
  B00011000,B00011000,B00001000,B00001000
},
{
  B00110000,B00110000,B00011000,B00011100,
  B00111100,B00111000,B00100100,B00000100
}
};

// ================= FUNCIONES MATRIX =================
void mostrarBitmap_P(const uint8_t *imagen)
{
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t i = 0; i < 8; i++)
    mx.setRow(i, pgm_read_byte(&imagen[i]));
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

void actualizarMatrix()
{
  if (estado == ROJO)
  {
    if (millis() - tiempoFrame >= T_FRAME)
    {
      tiempoFrame = millis();
      mostrarBitmap_P(caminar[frameActual]);
      frameActual = (frameActual + 1) % 4;
    }
  }
  else
  {
    mostrarBitmap_P(quieto);
  }
}

// =====================================================
// ======== FUNCIONES 74HC595 ===========================
// =====================================================

void mostrarNumero(int numero)
{
  int unidades = numero % 10;
  int decenas = numero / 10;

  digitalWrite(SR_LATCH, LOW);

  shiftOut(SR_DATA, SR_CLK, MSBFIRST, numeros[decenas]);
  shiftOut(SR_DATA, SR_CLK, MSBFIRST, numeros[unidades]);

  digitalWrite(SR_LATCH, HIGH);
}

void apagarDisplay()
{
  digitalWrite(SR_LATCH, LOW);

  shiftOut(SR_DATA, SR_CLK, MSBFIRST, 0x00);
  shiftOut(SR_DATA, SR_CLK, MSBFIRST, 0x00);

  digitalWrite(SR_LATCH, HIGH);
}

// ================= INTERRUPCIÓN =================
ISR(PCINT2_vect)
{
  if (!(PIND & (1 << PD7)))
    solicitudPeatonal = true;
}

// ================= SETUP =================
void setup()
{
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5);

  DDRB |= (1 << ROJO_LED) | (1 << AMARILLO_LED);
  DDRD |= (1 << VERDE_LED);

  DDRD &= ~(1 << PD7);
  PORTD |= (1 << PD7);

  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT23);

  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_CLK, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);

  apagarDisplay();

  tiempoCambio = millis();
}

// ================= LOOP =================
void loop()
{
  unsigned long ahora = millis();

  switch (estado)
  {
    case VERDE:
      PORTB &= ~((1 << ROJO_LED) | (1 << AMARILLO_LED));
      PORTD |= (1 << VERDE_LED);

      apagarDisplay();

      if ((ahora - tiempoCambio >= T_VERDE_MIN && solicitudPeatonal) ||
          (ahora - tiempoCambio >= T_VERDE_NORMAL))
      {
        estado = AMARILLO;
        tiempoCambio = ahora;
      }
      break;

    case AMARILLO:
      PORTB |= (1 << AMARILLO_LED);
      PORTB &= ~(1 << ROJO_LED);
      PORTD &= ~(1 << VERDE_LED);

      apagarDisplay();

      if (ahora - tiempoCambio >= T_AMARILLO)
      {
        estado = ROJO;
        tiempoCambio = ahora;
        contador = T_ROJO_PEATON / 1000;
        tiempoConteo = ahora;
      }
      break;

    case ROJO:
      PORTB |= (1 << ROJO_LED);
      PORTB &= ~(1 << AMARILLO_LED);
      PORTD &= ~(1 << VERDE_LED);

      if (ahora - tiempoConteo >= 1000 && contador > 0)
      {
        tiempoConteo = ahora;
        contador--;
      }

      // Parpadeo en últimos 3 segundos
      if (contador <= 3)
      {
        if ((ahora / 300) % 2 == 0)
          mostrarNumero(contador);
        else
          apagarDisplay();
      }
      else
      {
        mostrarNumero(contador);
      }

      if (contador == 0)
      {
        estado = VERDE;
        tiempoCambio = ahora;
        solicitudPeatonal = false;
      }
      break;
  }

  actualizarMatrix();
}