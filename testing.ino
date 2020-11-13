// 99 Mustang V6 v1.0a

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

// digital inputs
const byte crank_input = 2; // input from the crankshaft sensor. should already be conditioned.
const byte test_signal = 4; // test signal that can be routed to crank input to test the system
                            //it is generated using timer1 of the atmega328. you'll see below.

//analog inputs
const int throttle_input = A0;  //throttle input 0-5v

// Ignition outputs
unsigned int ign_dwell_time = 3000;
const byte coila = 5;
bool coila_enabled = false;
const byte coilb = 6;
bool coilb_enabled = false;
const byte coilc = 7;
bool coilc_enabled = false;

// Injector outputs
const byte inj1 = 8;
const byte inj2 = 9;
const byte inj3 = 10;
const byte inj4 = 11;
const byte inj5 = 12;
const byte inj6 = 13;

// Other parameters
volatile unsigned long last_crank_time = 0;
volatile unsigned long last_crank_duration = 0;
volatile int current_tooth = 0;
const int tooth_count = 36;
volatile bool crank_toggle = false;
int engine_rpm = 600;
int engine_rpm_max = 5500;
bool over_rev = false;
bool do_engine_calc = false;
bool signal_sync = false;
unsigned long tooth_times[tooth_count - 1] = {};

byte current_coil = 0;
byte inja = 0;
byte injb = 0;

unsigned long ign_advance_in_us = 0;
int ign_advance_trim_in_deg = 0;
unsigned long ign_start = 0;
unsigned long ign_end = 0;

unsigned long inj_duration = 100;
unsigned long inj_trim = 0;
unsigned long inj_start = 0;
unsigned long inj_end = 0;

int throttle_pos = 1;
byte throttle_percent = 0;

byte s_buffer[128];
int bytes_waiting = 0;

//fuel table goes here
int fuel_table[11][11] = {
  // x axis = rpm 500 inc
  // y axis = throttle percent 10 inc
  {1200, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100},
  {1200, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100},
  {1200, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100},
  {1300, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200},
  {1300, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200},
  {1300, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200},
  {1400, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300},
  {1400, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300},
  {1400, 1400, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300},
  {1450, 1450, 1550, 1650, 1750, 1850, 1950, 2050, 2150, 2250, 2350},
  {1500, 1500, 1600, 1700, 1800, 1900, 2000, 2100, 2200, 2300, 2400}
};

//ignition table goes here
int ignition_table[11][11] = {
  // x axis = rpm 500 inc
  // y axis = throttle percent 10 inc
  {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6},
  {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6},
  {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6},
  {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6},
  {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6},
  {2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7},
  {2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7},
  {2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7},
  {2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7},
  {2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7},
  {3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8}
};

void crank_interrupt() {
  if (last_crank_time == 0 && last_crank_duration == 0) {
    last_crank_time = micros();
    current_tooth = 0;
  }
  else {
    unsigned long duration = micros() - last_crank_time;
    last_crank_time = micros();
    current_tooth += 1;
    if (current_tooth == tooth_count) {
      current_tooth = 1;
    }
    tooth_times[current_tooth - 1] = duration;
    if (current_tooth != tooth_count - 1) {
      last_crank_duration = duration;
      if (last_crank_duration > 0) {
        engine_rpm = ((1000000 / (last_crank_duration * 36)) * 60);
        engine_calc();
      }
    }
    else { check_sync(); }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(crank_input, INPUT_PULLUP);
  pinMode(test_signal, OUTPUT);
  digitalLow(test_signal);
  attachInterrupt(digitalPinToInterrupt(crank_input), crank_interrupt, FALLING);

  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register
  OCR1A = 10000;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 bit for /1 prescaler
  TCCR1B |= (1 << CS10); //  | (1 << CS12)
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

byte teeth = 36;
volatile byte tooth = 18;
volatile bool sig_up = false;

ISR(TIMER1_COMPA_vect) {
  if (sig_up) {
    digitalLow(test_signal);
    sig_up = false;
  }
  else {
    if (tooth + 1 == teeth) {
      tooth = 0;
      sig_up = true;
    }
    else {
      tooth += 1;
      digitalHigh(test_signal);
      sig_up = true;
    }
  }
}

unsigned int calculate_advance_from_deg(int deg) {
  return (last_crank_duration / 10) * deg;
}

void check_sync() {
  if (tooth_times[0] > 0) {
    unsigned int tooth_index = 0;
    unsigned long tooth_value = 0;
    for (int i = 0; i < tooth_count - 1; i++) {
      if (tooth_times[i] > tooth_value) {
        tooth_value = tooth_times[i];
        tooth_index = i;
      }
      tooth_times[i] = 0;
    }
    if (tooth_index == current_tooth - 1) {
      if (!signal_sync) {
        // send signal_sync status to laptop
        byte to_send[3] = {0x00, 0x01, 0x00};
        add_to_serial(to_send, 3);
      }
      signal_sync = true;
      if (crank_toggle == false) { crank_toggle = true; }
      else { crank_toggle = false; }
    }
    else {
      if (signal_sync) {
        // send signal_sync status to laptop
        byte to_send[3] = {0x00, 0x00, 0x00};
        add_to_serial(to_send, 3);
      }
      signal_sync = false;
      current_tooth = current_tooth - 1 - tooth_index;
    }
  }
}

void engine_calc() {
  if (engine_rpm >= engine_rpm_max) {
      if (!over_rev) {
        // send status to laptop
        byte to_send[3] = {0x00, 0x01, 0x01};
        add_to_serial(to_send, 3);
        over_rev = true;
      }
  }
  else {
      if (over_rev) {
        // send status to laptop
        byte to_send[3] = {0x00, 0x00, 0x01};
        add_to_serial(to_send, 3);
        over_rev = false;
      }
  }
  // get throttle input
  throttle_pos = analogRead(throttle_input);
  throttle_percent = ((throttle_pos + 1) / 1024.0) * 100;

  // get fuel needed from throttle and rpm
  byte throttle_index = throttle_percent / 10;
  byte rpm_index = engine_rpm / 500;
  inj_duration = fuel_table[throttle_index][rpm_index] + inj_trim;
  ign_advance_in_us = calculate_advance_from_deg(ignition_table[throttle_index][rpm_index] + ign_advance_trim_in_deg);

  if (signal_sync && !over_rev) {
    unsigned long tdc = last_crank_time + (last_crank_duration * 7);
    if (current_tooth == tooth_count - 2) {
      current_coil = coila;
      inja = inj1;
      injb = inj5;
      set_times(tdc);
    }
    else if (current_tooth == 11) {
      current_coil = coilc;
      inja = inj3;
      injb = inj4;
      set_times(tdc);
    }
    else if (current_tooth == 23) {
      current_coil = coilb;
      inja = inj2;
      injb = inj6;
      set_times(tdc);
    }
  }
}

void set_times(unsigned long tdc) {
  ign_start = tdc - ign_advance_in_us - ign_dwell_time;
  ign_end = tdc - ign_advance_in_us;
  inj_start = tdc - inj_duration;
  inj_end = tdc;
  if (micros() > inj_start) { inj_end += micros() - inj_start; }
}

void loop() {
  if (last_crank_duration == 0) {
    engine_rpm = 0;
    return;
  }

  //deal with serial comms
  else if (Serial.available() > 2) { get_serial(); }
  else if (bytes_waiting > 0) { send_serial(); }

  if (signal_sync) { do_coils_and_inj(micros()); }
  else { press_the_panic_button(); }
}

void press_the_panic_button() {
  digitalLow(coila);
  digitalLow(coilb);
  digitalLow(coilc);
  digitalLow(inj1);
  digitalLow(inj2);
  digitalLow(inj3);
  digitalLow(inj4);
  digitalLow(inj5);
  digitalLow(inj6);
}

bool coil_on = false;
bool inj_on = false;

void do_coils_and_inj(unsigned long time_now) {
  if (time_now >= ign_start && time_now < ign_end) {
    if (!coil_on) {
      digitalHigh(current_coil);
      coil_on = true;
    }
  }
  else {
    if (coil_on) {
      digitalLow(current_coil);
      coil_on = false;
    }
  }

  if (crank_toggle && time_now >= inj_start && time_now < inj_end) {
    if (!inj_on) {
      digitalHigh(inja);
      digitalHigh(injb);
      inj_on = true;
    }
  }
  else {
    if (inj_on) {
      digitalLow(inja);
      digitalLow(injb);
      inj_on = false;
    }
  }
}

void get_serial() {
  // Commands to be received by Arduino from monitor software
  //
  // 0x00 - Request for data
  //        following byte shows which data was requested
  //        0x00 - flags
  //        0x01 - fuel_duration
  //        0x02 - fuel_trim
  //        0x03 - ignition_advance
  //        0x04 - ignition_advance_trim
  //        0x05 - throttle_pos
  //        0x06 - engine_rpm
  //        0x07 - engine_rpm_max
  // 0x01 - adjust fuel_trim
  // 0x02 - adjust_ignition_advance
  // 0x03 - adjust_engine_rpm_max
  byte input = Serial.read();
  if (input == 0x00) { // requesting data
    byte input1 = Serial.read();
    byte input2 = Serial.read();
    if (input2 == 0x00) { // flags
      if (input1 == 0x00) { // signal_sync
        byte to_send[3] = {input2, byte(signal_sync), 0x00};
        add_to_serial(to_send, 3);
      }
      else if (input1 == 0x01) { // over_rev
        byte to_send[3] = {input2, byte(over_rev), 0x01};
        add_to_serial(to_send, 3);
      }
    }
    else if (input2 == 0x01) { // fuel_duration
      byte to_send[3] = {input2, byte(inj_duration), byte(inj_duration >> 8)};
      add_to_serial(to_send, 3);
    }
    else if (input2 == 0x02) { // fuel_trim
      byte to_send[3] = {input2, byte(inj_trim), byte(inj_trim >> 8)};
      add_to_serial(to_send, 3);
    }
    else if (input2 == 0x03) { // ignition_advance
      byte to_send[3] = {input2, byte(ign_advance_in_us), byte(ign_advance_in_us >> 8)};
      add_to_serial(to_send, 3);
    }
    else if (input2 == 0x04) { // ignition_advance_trim
      byte to_send[3] = {input2, byte(ign_advance_trim_in_deg), byte(ign_advance_trim_in_deg >> 8)};
      add_to_serial(to_send, 3);
    }
    else if (input2 == 0x05) { // throttle_pos
      byte to_send[3] = {input2, byte(throttle_percent), byte(throttle_percent >> 8)};
      add_to_serial(to_send, 3);
    }
    else if (input2 == 0x06) { // engine_rpm
      byte to_send[3] = {input2, byte(engine_rpm), byte(engine_rpm >> 8)};
      add_to_serial(to_send, 3);
    }
    else if (input2 == 0x07) { // engine_rpm_max
      byte to_send[3] = {input2, byte(engine_rpm_max), byte(engine_rpm_max >> 8)};
      add_to_serial(to_send, 3);
    }
  }
  else if (input == 0x01) {
    // adjusting fuel trim
    byte input2 = Serial.read();
    input2 = Serial.read();
    if (input2 == 0x00) {
      inj_trim -= 100;
    }
    else if (input2 == 0x01) {
      inj_trim += 100;
    }
    byte to_send[3] = {0x02, byte(inj_trim), byte(inj_trim >> 8)};
    add_to_serial(to_send, 3);
  }
  else if (input == 0x02) {
    // adjusting ignition trim
    byte input2 = Serial.read();
    input2 = Serial.read();
    if (input2 == 0x00) {
      ign_advance_trim_in_deg -= 1;
    }
    else if (input2 == 0x01) {
      ign_advance_trim_in_deg += 1;
    }
    byte to_send[3] = {0x04, byte(ign_advance_trim_in_deg), byte(ign_advance_trim_in_deg >> 8)};
    add_to_serial(to_send, 3);
  }
  else if (input == 0x03) {
    // adjusting rpm limit
    byte input2 = Serial.read();
    input2 = Serial.read();
    if (input2 == 0x00) {
      engine_rpm_max -= 500;
    }
    else if (input2 == 0x01) {
      engine_rpm_max += 500;
    }
    byte to_send[3] = {0x07, byte(engine_rpm_max), byte(engine_rpm_max >> 8)};
    add_to_serial(to_send, 3);
  }
}

void add_to_serial(byte bytes_to_add[], int amount) {
  for (int b = 0; b < amount; b++) {
    s_buffer[bytes_waiting + b] = bytes_to_add[b];
  }
  bytes_waiting += amount;
}


void send_serial() {
  // Sends data back to monitor software
  byte next_byte = s_buffer[0];
  for (int b = 0; b < sizeof(s_buffer) - 1; b++) {
    s_buffer[b] = s_buffer[b + 1];
  }
  bytes_waiting--;
  Serial.write(next_byte);
}
