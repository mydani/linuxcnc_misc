#include <Wire.h>
#include <SoftwareSerial.h>
#include <SPI.h>

volatile char in[4];
volatile char out[4];
volatile int rxstate = 0;

const int latchPin = 5;
const int spiModePin = 10;


// the setup routine runs once when you press reset:
void setup() {
  cli();                            // disable all interrupts
  TCCR1A = 0;                       // set TCCR1A register to 0
  TCCR1B = 0;                       // same for TCCR1B
  TCNT1 = 0;                        // initialize counter value to 0
  TCCR1B |= (1 << WGM12);           // turn on CTC mode
  TCCR1B |= (1 << CS12) | (1 << CS10);        // Set CS10 and CS12 bits for 1024 prescaler
  unsigned int ms = 50;
  float freq = 1.0/(float(ms)/1000.0);        // Interrupt frequency
  float f = freq * 1024;                      // Multiply by prescaler
  long int i = 16000000ul / (long int)f;      // Divide clock speed by that
  i -= 1;                                     // Subtract 1
  OCR1A = int(i);                             // Set OCR1A register
  TIMSK1 |= (1 << OCIE1A);                    // Enable timer compare ints
  sei();                                      // re-enable all interrupts
  pinMode(latchPin, OUTPUT);
  pinMode (spiModePin, OUTPUT); // needed for device to be SPI master, if an input and it goes low device becomes SPI slave
  digitalWrite (latchPin, HIGH);
  SPI.begin();
  Serial.begin(115200);
}

// the loop routine runs over and over again forever:
void loop() 
{
  // read and sync to magic pattern "OUT"
  while(Serial.available())
  {
    char rx = Serial.read();
    switch(rxstate)
    {
      case 0:
        if(rx == 'O')
          rxstate++;
      break;
      case 1:
        if(rx == 'U')
          rxstate++;
      break;
      case 2:
        if(rx == 'T')
          rxstate++;
      break;
      case 3:
        out[0]=rx;
        rxstate++;
      break;
      case 4:
        out[1]=rx;
        rxstate++;
      break;
      case 5:
        out[2]=rx;
        rxstate++;
      break;
      case 6:
        out[3]=rx;
        rxstate=0;
      break;
    }
  }
}


ISR(TIMER1_COMPA_vect, ISR_NOBLOCK)
{
  // prepare input data
  {
    SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
    digitalWrite (latchPin, LOW);
    in[0]=SPI.transfer(out[0]);
    in[1]=SPI.transfer(out[1]);
    in[2]=SPI.transfer(out[2]);
    in[3]=SPI.transfer(out[3]);
    digitalWrite (latchPin, HIGH);
    SPI.endTransaction();
    //ToDo: Latch-Pin 
  }

  // send input data
  Serial.write('I');
  Serial.write('N');
  Serial.write(':');
  Serial.write(in[0]);
  Serial.write(in[1]);
  Serial.write(in[2]);
  Serial.write(in[3]);
  Serial.flush();
 
}
