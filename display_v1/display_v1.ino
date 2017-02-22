/*
   Hack balanza Systel
  **********
  Created by Facundo Daguerre (aka derfaq)
  38ef2d7a-f3a4-11e6-bc64-92361f002671
  GNU GPL
*/

//See: https://www.appelsiini.net/2012/driving-595-shift-registers
//     https://github.com/tuupola/avr_demo/blob/master/blog/driving_595/spi/main.c

void spi_init(void){
  SPCR &= ~(1<<DORD); //MSB first
  SPCR |= (1<<MSTR);  //Act like you know, master
  SPCR |= (1<<CPHA); //YES!YES IS THE ANSWER
  SPCR |= (1<<CPOL); //depends YES
  
  //FREQUENCY CHEQ FOR INTERFERENCES Table 19-5. Relationship Between SCK and the Oscillator Frequency
  //SPCR |= (1<<SPI2X);
  SPCR |= (1<<SPR1); 
  //SPCR |= (1<<SPR0);
  
  PORTB |= (1<<2);  //PORTB &= ~(1<<2); //set for toggle SS, for latch
  SPCR |= (1<<SPE);   //Enable SPI
}

//Code for transfer16 from: Arduino/hardware/arduino/avr/libraries/SPI/src/SPI.h
//https://github.com/arduino/Arduino/blob/2bfe164b9a5835e8cb6e194b928538a9093be333/hardware/arduino/avr/libraries/SPI/src/SPI.h

inline static uint16_t transfer16(uint16_t data) {
    union { uint16_t val; struct { uint8_t lsb; uint8_t msb; }; } in, out; //WTF!
    in.val = data;
    if (!(SPCR & _BV(DORD))) {
      SPDR = in.msb;
      asm volatile("nop"); // See transfer(uint8_t) function, ASSEMBLER? REALLY?
      while (!(SPSR & _BV(SPIF))) ;
      out.msb = SPDR;
      SPDR = in.lsb;
      asm volatile("nop");
      while (!(SPSR & _BV(SPIF))) ;
      out.lsb = SPDR;
    } else {
      SPDR = in.lsb;
      asm volatile("nop");
      while (!(SPSR & _BV(SPIF))) ;
      out.lsb = SPDR;
      SPDR = in.msb;
      asm volatile("nop");
      while (!(SPSR & _BV(SPIF))) ;
      out.msb = SPDR;
    }
    return out.val;
}

void writeDigit(byte pos,byte value){
  //In digit..
  transfer16(1<<pos);
  //Latch
  PORTB ^= (1<<2);
  PORTB ^= (1<<2);
  //Clear digit
  PORTD &= ~(0b11111100); //clear
  PORTB &= ~(0b00000011); 
  //Write digit
  PORTD |= (value<<2); //set
  PORTB |= (value>>6); //set
}

volatile byte displayRAM[16];
volatile byte count;

void refresh_init(){
  OCR0B = 125; //any number
  TIMSK0 |= (1 << OCIE0B);
}

ISR(TIMER0_COMPB_vect){
  writeDigit(count,displayRAM[count]);
  count++;
  if(count==16) count=0;
}

void setup(){
  //AVR style
  DDRD  |= 0b11111100;
  DDRB  |= 0b00000011;
  //Arduino style
  pinMode(13,OUTPUT);  
  pinMode(11,OUTPUT); 
  pinMode(10,OUTPUT);
  //Function style
  spi_init();
  refresh_init();
  sei();
  Serial.begin(9600);
}

//See: http://www.josepino.com/microcontroller/7-segment-ascii
static const uint8_t charTable[] PROGMEM = 
{ 63,  6, 91, 79,102,109,125,  7,127,111,
 119,124, 57, 94,121,113,111,116, 48, 30,
 118, 56, 21, 84, 63,115,103, 80,109,120,
 62,  28, 42,118,110,91 };
  
void loop(){
  static byte pos;
  static int data;
  if (Serial.available() > 0) {
    // get incoming byte, and put in the next place:
    data = Serial.read();
    Serial.print(data, DEC);  //Remote echo
          
          if((data>47)&&(data<58)){ displayRAM[pos] = pgm_read_byte(&charTable[data-48]);
    }else if((data>64)&&(data<91)){ displayRAM[pos] = pgm_read_byte(&charTable[data-55]);
    }else if((data>96)&&(data<123)){ displayRAM[pos] = pgm_read_byte(&charTable[data-87]);
    }else if(data==32){ displayRAM[pos] = 0;
    }else if(data==33){ displayRAM[pos] = 130;
    }else { ;}
    
    pos++;
    if(pos==16) pos=0;
  }
  
}

