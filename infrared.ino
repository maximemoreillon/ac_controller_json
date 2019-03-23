void IR_send_signal(int IR_signal[]) {
  
  for (int i=0; i < IR_SIGNAL_LENGTH; i++){
    
    int IR_signal_bit = IR_signal[i];

    if (IR_signal_bit > 0) {
      IR_send_pulse(IR_signal_bit);
    }
    else {
      IR_send_pause(-IR_signal_bit);
    }
  }
}


void IR_send_pulse(int pulse_length) {
  // Send one pulse of a given length
  
  int IR_on = 0;
  long startMicros = micros();
  while (micros() < (startMicros + pulse_length)){
    // toggle pin and wait 26 us to make it a pulse
    if (IR_on == 0) {
      IR_on = 1;
    }
    else {
      IR_on = 0;
    }
    digitalWrite(IR_LED_PIN, IR_on);

    // IR LED frequency must be around 37-38kHz
    // Approx pulse period = 26us
    // A period consists of the LED being toggled twice => 13us between toggles
    delayMicroseconds(13);
  }
  
  // turn off IR after pulse is complete
  digitalWrite(IR_LED_PIN, LOW);
}

void IR_send_pause(int pause_length) {
  // A pause is just not doing anything for a given amount of time
  delayMicroseconds(pause_length);
}
