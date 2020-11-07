  // 99 Mustang V6
  
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
const byte crank_input = 2;
const byte test_signal = 4;
  
  //analog inputs
const int throttle_input = A0;

  // Ignition outputs
unsigned int ign_dwell_time = 50000;
const byte coila = 5;
bool coila_enabled = false;
const byte coilb = 6;
bool coilb_enabled = false;
const byte coilc = 7;
bool coilc_enabled = false;
int coil_order[3] = {coila, coilc, coilb};
int current_coil = 0;
  
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
volatile unsigned long last_mark = 0;
volatile int current_tooth = 0;
const int tooth_count = 36;
int current_cylinder = 0;
int engine_rpm = 600;
int engine_rpm_max = 5500;
bool do_engine_calc = false;
bool signal_sync = false;
long tooth_times[tooth_count-1] = {};
  
unsigned long ign_advance_in_us = 0;
int ign_advance_in_deg = 0;
int ign_advance_trim_in_deg = 0;
unsigned long ign_times[6][2] = {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}};
  
unsigned long inj_duration = 100;
unsigned long inj_trim = 0;
unsigned long inj_times[6][2] = {{0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}};

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
    last_crank_duration = 0;
    last_crank_time = micros();
    current_tooth = 0;
    return;
  }
  else {
    unsigned long duration = micros() - last_crank_time;
    last_crank_time = micros();
    current_tooth += 1;
    if (current_tooth >= tooth_count) {
      current_tooth = 1;
    }
    tooth_times[current_tooth-1] = duration;
    if (current_tooth != tooth_count - 1) {
      last_crank_duration = duration;
      if (last_crank_duration > 0) { engine_rpm = ((1000000 / (last_crank_duration * 36)) * 60); }
    }
    do_engine_calc = true;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(crank_input, INPUT_PULLUP);
  pinMode(test_signal, OUTPUT);
  digitalLow(test_signal);
  attachInterrupt(digitalPinToInterrupt(crank_input), crank_interrupt, FALLING);
}

unsigned int calculate_advance_from_deg(int deg) {
  return (last_crank_duration / 10) * deg;
}

void engine_calc() {
  if (current_tooth == tooth_count - 1 && tooth_times[0] > 0) {
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
      signal_sync = true;
    }
    else {
      signal_sync = false;
      current_tooth = current_tooth + (current_tooth - tooth_index) - 1;
      if (current_tooth > tooth_count - 1) {
        current_tooth -= tooth_count - 1;
      }
      else if (current_tooth < 1) {
        current_tooth += tooth_count - 1;
      }
    }
  }

  if (signal_sync) {
    unsigned long time_now = micros();
    unsigned long tdc = 0;
    if (current_tooth == tooth_count - 1) {
      tdc = time_now + (last_crank_duration * 5);
      current_coil = 0;
      ign_times[current_coil][0] = tdc - ign_advance_in_us - ign_dwell_time;
      ign_times[current_coil][1] = tdc - ign_advance_in_us;
      inj_times[1][0] = tdc - inj_duration;
      inj_times[1][1] = tdc;
      inj_times[5][0] = tdc - inj_duration;
      inj_times[5][1] = tdc;
    }
    else if (current_tooth == 12) {
      tdc = time_now + (last_crank_duration * 6);
      current_coil = 1;
      ign_times[current_coil][0] = tdc - ign_advance_in_us - ign_dwell_time;
      ign_times[current_coil][1] = tdc - ign_advance_in_us;
      inj_times[2][0] = tdc - inj_duration;
      inj_times[2][1] = tdc;
      inj_times[6][0] = tdc - inj_duration;
      inj_times[6][1] = tdc;
    }
    else if (current_tooth == 24) {
      tdc = time_now + (last_crank_duration * 6);
      ign_times[current_coil][0] = tdc - ign_advance_in_us - ign_dwell_time;
      ign_times[current_coil][1] = tdc - ign_advance_in_us;
      inj_times[3][0] = tdc - inj_duration;
      inj_times[3][1] = tdc;
      inj_times[4][0] = tdc - inj_duration;
      inj_times[4][1] = tdc;
    }
  }
  do_engine_calc = false;
}

unsigned long t_timer = 0;
unsigned long t_timer_current = 0;
unsigned long t_timer_max = 1250;
byte teeth = 36;
byte tooth = 30;
bool sig_up = false;

void timer_func() {
  if (t_timer_current != 0) {
    t_timer += micros() - t_timer_current;
    if (t_timer >= t_timer_max) {
      t_timer = 0;
      if (sig_up) {
        digitalLow(test_signal);
        sig_up = false;
      }
      else {
        tooth += 1;
        if (tooth == teeth) {
          tooth = 0;
        }
        else {
          digitalHigh(test_signal);
        }
        sig_up = true;
      }
    }
  }
  t_timer_current = micros();  
}

void loop() {
  timer_func();
  if (last_crank_duration == 0) { engine_rpm = 0; return;}
  else if (do_engine_calc) {
    // get throttle input
    throttle_pos = analogRead(throttle_input);
    throttle_percent = ((throttle_pos + 1) / 1024.0) * 100;
    
    // get fuel needed from throttle and rpm
    inj_duration = fuel_table[throttle_percent / 10][engine_rpm / 500] + inj_trim;
    ign_advance_in_us = calculate_advance_from_deg(ignition_table[throttle_percent / 10][engine_rpm / 500] + ign_advance_trim_in_deg);
    engine_calc();
  }

  if (signal_sync) {
    do_coils();
    do_inj(1, inj1);
    do_inj(2, inj2);
    do_inj(3, inj3);
    do_inj(4, inj4);
    do_inj(5, inj5);
    do_inj(6, inj6);
  }
  else {
    press_the_panic_button();
  }

  //deal with serial comms
  if (Serial.available() > 2) {
    get_serial();
  }
  if (bytes_waiting > 0) {
    send_serial();
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

void do_coils() {
  if (micros() > ign_times[current_coil][0] && micros() < ign_times[current_coil][1]) {
    digitalHigh(coil_order[current_coil]);
  }
  else { digitalLow(coil_order[current_coil]); }
}

void do_inj(int cyl, int inj_pin) {
  if (micros() > inj_times[cyl-1][0] && micros() < inj_times[cyl-1][1]) {
    digitalHigh(inj_pin);
  }
  else { digitalLow(inj_pin); }
}

void get_serial() {
  // Commands to be received by Arduino from monitor software
  //
  // 0x00 - Request for data
  //        following byte shows which data was requested
  //        0x00 - engine_rpm
  //        0x01 - fuel_base
  //        0x02 - fuel_trim
  //        0x03 - ignition_advance
  //        0x04 - ignition_advance_trim
  //        0x05 - throttle_pos
  //        0x06 - whatever the f*** i want >:|
  // 0x01 - adjust fuel_trim
  // 0x02 - adjust_ignition_advance
  byte input = Serial.read();
  if (input == 0x00) { // requesting data
    byte input2 = Serial.read();
    input2 = Serial.read();
    if (input2 == 0x00) { // engine_rpm
      byte to_send[] = {0x00, byte(engine_rpm), byte(engine_rpm >> 8)};
      add_to_serial(to_send, sizeof(to_send));
    }
    else if (input2 == 0x01) { // fuel_base
      byte to_send[] = {input2, byte(inj_duration), byte(inj_duration >> 8)};
      add_to_serial(to_send, sizeof(to_send));
    }
    else if (input2 == 0x02) { // fuel_trim
      byte to_send[] = {input2, byte(inj_trim), byte(inj_trim >> 8)};
      add_to_serial(to_send, sizeof(to_send));
    }
    else if (input2 == 0x03) { // ignition_advance
      byte to_send[] = {input2, byte(ign_advance_in_us), byte(ign_advance_in_us >> 8)};
      add_to_serial(to_send, sizeof(to_send));
    }
    else if (input2 == 0x04) { // ignition_advance_trim
      byte to_send[] = {input2, byte(ign_advance_trim_in_deg), byte(ign_advance_trim_in_deg >> 8)};
      add_to_serial(to_send, sizeof(to_send));
    }
    else if (input2 == 0x05) { // throttle_pos
      byte to_send[] = {input2, byte(throttle_percent), byte(throttle_percent >> 8)};
      add_to_serial(to_send, sizeof(to_send));
    }
    else if (input2 == 0x06) { // status
      byte to_send[] = {input2, byte(current_tooth), byte(current_tooth >> 8)};
      add_to_serial(to_send, sizeof(to_send));
    }
  }
  else if (input == 0x01) {
    byte input2 = Serial.read();
    input2 = Serial.read();
    if (input2 == 0x00) {
      inj_trim -= 100;
      t_timer_max -= 50;
    }
    else if (input2 == 0x01) {
      inj_trim += 100;
      t_timer_max += 50;
    }
//    byte to_send[] = {0x02, byte(inj_trim), byte(inj_trim >> 8)};
//    add_to_serial(to_send, sizeof(to_send));
  }
  else if (input == 0x02){
    byte input2 = Serial.read();
    input2 = Serial.read();
    if (input2 == 0x00) {
      ign_advance_trim_in_deg -= 1;
    }
    else if (input2 == 0x01) {
      ign_advance_trim_in_deg += 1;
    }
//    byte to_send[] = {0x04, byte(ign_advance_in_deg), byte(ign_advance_in_deg >> 8)};
//    add_to_serial(to_send, sizeof(to_send));
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
