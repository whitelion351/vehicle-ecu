// 99 Mustang V6 with EEC-V
// v1.1a

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
const byte crank_input = 3; // input from the crankshaft sensor. should already be conditioned (0-5v).

//analog inputs
const int throttle_input = A1;  //throttle input 0-5v

// Ignition outputs
unsigned int ign_dwell_time = 3000;
const byte coila = 9;
bool coila_enabled = true;
const byte coilb = 10;
bool coilb_enabled = true;
const byte coilc = 11;
bool coilc_enabled = true;
byte current_coil = 0;

// Injector outputs
const byte inj1 = 5;
const byte inj2 = 6;
const byte inj3 = 7;
const byte inj4 = 7;
const byte inj5 = 5;
const byte inj6 = 6;
byte inja = 0;
byte injb = 0;

// Crankshaft variables
volatile unsigned long last_crank_time = 0;
volatile unsigned long last_crank_duration = 0;
const int tooth_count = 36;
volatile int current_tooth = 0;
unsigned long tooth_times[tooth_count - 1] = {};
volatile bool crank_toggle = false;

// Other parameters
unsigned int engine_rpm = 0;
unsigned int engine_rpm_max = 5500;
bool over_rev = false;
bool signal_sync = false;

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
  {10, 10, 12, 12, 13, 13, 14, 14, 15, 15, 16},
  {10, 10, 12, 12, 13, 13, 14, 14, 15, 15, 16},
  {10, 10, 12, 12, 13, 13, 14, 14, 15, 15, 16},
  {10, 10, 12, 12, 13, 13, 14, 14, 15, 15, 16},
  {12, 10, 12, 12, 13, 13, 14, 14, 15, 15, 16},
  {12, 10, 12, 12, 13, 13, 14, 14, 15, 15, 16},
  {12, 24, 28, 32, 34, 36, 38, 40, 40, 40, 40},
  {12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17},
  {14, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17},
  {14, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17},
  {14, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18}
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
    }
    else { check_sync(); }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(crank_input, INPUT);
  digitalLow(13);
  digitalLow(coila);
  digitalLow(coilb);
  digitalLow(coilc);
  digitalLow(inj1);
  digitalLow(inj2);
  digitalLow(inj3);
  attachInterrupt(digitalPinToInterrupt(crank_input), crank_interrupt, FALLING);
}

int calculate_advance_from_deg(int deg) {
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

void get_inputs() {
  // get throttle input
  throttle_pos = analogRead(throttle_input);
  throttle_percent = ((throttle_pos + 1) / 1024.0) * 100;

  // get fuel needed from throttle and rpm
  byte throttle_index = throttle_percent / 10;
  if (throttle_index > 10) {throttle_index = 10;}
  byte rpm_index = engine_rpm / 500;
  if (rpm_index > 10) {rpm_index = 10;}
  inj_duration = fuel_table[throttle_index][rpm_index] + inj_trim;
  ign_advance_in_us = calculate_advance_from_deg(ignition_table[throttle_index][rpm_index] + ign_advance_trim_in_deg);
  engine_rpm = ((1000000 / (last_crank_duration * 36)) * 60);
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
  if (signal_sync && !over_rev) {
    if (current_tooth == tooth_count - 2) {
      unsigned long tdc = last_crank_time + (last_crank_duration * 7);
      current_coil = coila;
      inja = inj1;
      injb = inj5;
      set_times(tdc);
    }
    else if (current_tooth == 11) {
      unsigned long tdc = last_crank_time + (last_crank_duration * 7);
      current_coil = coilc;
      inja = inj3;
      injb = inj4;
      set_times(tdc);
    }
    else if (current_tooth == 23) {
      unsigned long tdc = last_crank_time + (last_crank_duration * 7);
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
  if (last_crank_duration == 0) { engine_rpm = 0; }

  if (Serial.available() > 3) { get_serial(); }
  else if (bytes_waiting > 0) { send_serial(); }

  if (signal_sync) { do_coils_and_inj(micros()); }
  else { press_the_panic_button(); }
  if (current_tooth == tooth_count - 2) {
    get_inputs();
    engine_calc();
  }
  else if (current_tooth == 11 || current_tooth == 23) {
    engine_calc();
  }

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
  if (input != 0xff) { return; }
  input = Serial.read();
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
  s_buffer[bytes_waiting] = 0xff;
  bytes_waiting += 1;
  for (int b = 0; b < amount; b++) {
    s_buffer[bytes_waiting + b] = bytes_to_add[b];
  }
  bytes_waiting += amount;
}


void send_serial() {
  // grab first byte in buffer then shift "buffer" left
  byte next_byte = s_buffer[0];
  Serial.write(next_byte);
  for (int b = 0; b < sizeof(s_buffer) - 1; b++) {
    s_buffer[b] = s_buffer[b + 1];
  }
  bytes_waiting--;
}
