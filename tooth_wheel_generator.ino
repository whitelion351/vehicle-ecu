//v1.0

byte test_signal = 4;
byte teeth = 36;
volatile byte tooth = 18;
volatile bool sig_up = false;
int rate_of_change_low = -2;
int rate_of_change_high = -40;
unsigned int compare_value = 10000;

#define portOfPin(P)(((P)>=0&&(P)<8)?&PORTD:(((P)>7&&(P)<14)?&PORTB:&PORTC))
#define ddrOfPin(P)(((P)>=0&&(P)<8)?&DDRD:(((P)>7&&(P)<14)?&DDRB:&DDRC))
#define pinOfPin(P)(((P)>=0&&(P)<8)?&PIND:(((P)>7&&(P)<14)?&PINB:&PINC))
#define pinIndex(P)((uint8_t)(P>13?P-14:P&7))
#define pinMask(P)((uint8_t)(1<<pinIndex(P)))

#define pinAsInput(P) *(ddrOfPin(P))&=~pinMask(P)
#define pinAsInputPullUp(P) *(ddrOfPin(P))&=~pinMask(P);digitalHigh(P)
#define pinAsOutput(P) *(ddrOfPin(P))|=pinMask(P)
#define digitalLow(P) *(portOfPin(P))&=~pinMask(P)
#define digitalHigh(P) *(portOfPin(P))|=pinMask(P)
#define isHigh(P)((*(pinOfPin(P))& pinMask(P))>0)
#define isLow(P)((*(pinOfPin(P))& pinMask(P))==0)
#define digitalState(P)((uint8_t)isHigh(P))


void setup() {
  pinMode(test_signal, OUTPUT);

  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register
  OCR1A = compare_value;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 bit for /1 prescaler
  TCCR1B |= (1 << CS10); //  | (1 << CS12)
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

volatile bool change_compare = false;

ISR(TIMER1_COMPA_vect) {
  if (sig_up) {
    digitalLow(test_signal);
    sig_up = false;
  }
  else {
    if (tooth + 1 == teeth) {
      tooth = 0;
      change_compare = true;
    }
    else {
      tooth += 1;
      digitalHigh(test_signal);
    }
    sig_up = true;
  }
}

void loop() {
  if (change_compare) {
    if (compare_value < 4000) {
      compare_value += rate_of_change_low;
    }
    else {
      compare_value += rate_of_change_high;
    }
    if (compare_value <= 2000 || compare_value >= 12000) {
      rate_of_change_low *= -1;
      rate_of_change_high *= -1;
    }
    OCR1A = compare_value;
    change_compare = false;
  }
}
