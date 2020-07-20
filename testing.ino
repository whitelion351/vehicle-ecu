  //Injector inputs from ECU
  int injinput1 = 2;
  int injinput2 = 3;
  int injinput3 = 4;
  int injinput4 = 5;
  int injinput5 = 6;
  int injinput6 = 7;

  // Injector outputs to mosfets
  int injoutput1 = 8;
  int injoutput2 = 9;
  int injoutput3 = 10;
  int injoutput4 = 11;
  int injoutput5 = 12;
  int injoutput6 = 13;

  int currentinj = 0;

  // Other injector parameters
  int inj_base_time = 1;
  int inj_offset_time = 0;
  
  // Other parameters
  unsigned long fuel_base_start = 0;
  unsigned long fuel_base_duration = 0;
  unsigned long last_update = 0;
  bool is_on = false;
  int engine_rpm = 0;
  int rpm_toggle = 0;
  unsigned long rpm_start = 0;
  unsigned long rpm_duration = 0;

  //Ignition stuff
  //TODO


void setup() {
  Serial.begin(9600);
  pinMode(injinput1, INPUT_PULLUP);
  pinMode(injinput2, INPUT_PULLUP);
  pinMode(injinput3, INPUT_PULLUP);
  pinMode(injinput4, INPUT_PULLUP);
  pinMode(injinput5, INPUT_PULLUP);
  pinMode(injinput6, INPUT_PULLUP);
}


void loop() {
  //deal with serial comms
  if (Serial.available()) {
    get_serial();
  }
  if (fuel_base_duration > 0) {
    send_serial();
  }

  //get timings for fuel inectors. calculate base duration
  if (!digitalRead(injinput1) && is_on == false) {
    is_on = true;
    fuel_base_start = millis();
    inj_on(injoutput6);
//    if (rpm_toggle == 0) {rpm_start = millis(); rpm_toggle = 1;}
//    else if (rpm_toggle == 1) {
//      rpm_duration = millis() - rpm_start;
//      float temp = ((rpm_duration / 1000) * 60) * 2;
//      rpm_toggle = 0;
//    }
  }
  else if (digitalRead(injinput1) && is_on == true) {
    is_on = false;
//      * 60) * 2;
    engine_rpm = (((1000 / (millis() - last_update)) * 60) * 2);
    last_update = millis();
    fuel_base_duration = millis() - fuel_base_start;
  }
  if (millis() - last_update > 1000) {
    fuel_base_duration = 0;
    rpm_start = 0;
    rpm_duration = 0;
    engine_rpm = 0;
  }
  if (millis() - fuel_base_start > fuel_base_duration + inj_offset_time) {
    inj_off(injoutput6);
  }
  inj_base_time = fuel_base_duration;
}

void inj_on(int inj) {
  digitalWrite(inj, HIGH);
}

void inj_off(int inj) {
  digitalWrite(inj, LOW);
}

void get_serial() {
  // Commands to be received by Arduino from monitor software
  // 0x00 - Increase fuel offset by 10 ms
  // 0x01 - Decrease fuel offset by 10 ms
  byte input = Serial.read();
  if (input == 0x00) {
    inj_offset_time += 10;
  }
  else if (input == 0x01){
    inj_offset_time -= 10;
  }
}

void send_serial() {
  // Sends data back to monitor software
  
  // send fuel base - 3 bytes
  Serial.write(0x00);
  Serial.write(inj_base_time);
  Serial.write(inj_base_time >> 8);
  Serial.println();

  // send fuel offset - 3 bytes
  Serial.write(0x01);
  Serial.write(inj_offset_time);
  Serial.write(inj_offset_time >> 8);
  Serial.println();
  
  // send rpm - 3 bytes
  Serial.write(0x02);
  Serial.write(engine_rpm);
  Serial.write(engine_rpm >> 8);
  Serial.println();
  }
