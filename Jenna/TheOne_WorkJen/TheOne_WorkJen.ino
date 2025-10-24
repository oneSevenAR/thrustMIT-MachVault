#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bounce2.h>

// Initialize I2C LCD (0x27 is a common address for 16x2 I2C LCD modules)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- INPUT PINS ---
byte up_button = 3;
byte dn_button = 2;
byte ent_button = 4;
byte jog_left = 5;
byte jog_right = 6;

// Bounce2 objects for each button
Bounce upBounce = Bounce();
Bounce dnBounce = Bounce();
Bounce entBounce = Bounce();
Bounce jogLeftBounce = Bounce();
Bounce jogRightBounce = Bounce();

const unsigned long DEBOUNCE_MS = 25; // debounce interval

// --- OUTPUT PINS (Stepper Drivers) ---
byte rotor_step_pin = 8;
byte rotor_dir_pin = 7;
byte rotor_ms1 = A3;
byte rotor_ms2 = 9;
byte rotor_ms3 = 12;

byte follower_dir_pin = 10;
byte follower_step_pin = 11;
byte follower_ms1 = A0;
byte follower_ms2 = A1;
byte follower_ms3 = A2;

byte reset_sleep_pin = 13;

// --- CONTROL STATES AND COUNTERS ---
byte program_state = 0; // State Machine: 0-Menu, 5-Jog/Home, 6-Winding

// Tracks the state of the move buttons in the previous loop
byte move_state_last = 0;
// Tracks the current debounced state of the Enter button
byte ent_state = 0;

// Winding Parameters
int n_turns = 3;
byte wire_gage_index = 3;
int core_diameter_mm = 25;
int coil_length_mm = 80 ;

// Timing for long press detection
unsigned long hold_start_time = 0;
unsigned long hold_duration = 0;
// Timer to control the auto-repeat speed when a button is held
unsigned long last_menu_action = 0;
const unsigned long HOLD_DELAY_MS = 500; // Delay before auto-repeat starts
const unsigned long REPEAT_RATE_MS = 100; // Speed of auto-repeat (fast changes)
const int BACKLASH_STEPS = 250;

// Winding Logic Variables
const long STEPS_PER_MM = 200;
const int ROTOR_STEPS_PER_TURN = 200;
const int ROTOR_STEP_DELAY_US = 1200; // Your new, stable rotor speed.
const int ROTOR_MICROSTEP_FACTOR = 8; // Eighth-step
const int FOLLOWER_STEP_DELAY = 1000;
const int FOLLOWER_MICROSTEP_FACTOR = 2;

long follower_max_steps = 0;
long current_follower_steps = 0;
int total_counter = 0;
byte wind_direction = 0;

// OLLOWER COORDINATION VARIABLES
// --- ACCUMULATOR SETUP ---
// Approximate wire pitch per turn (mm per layer), based on AWG
float wire_pitch_mm = 0.0;
int FOLLOWER_STEP_NUMERATOR = 0;
int FOLLOWER_STEP_DENOMINATOR = 0;

int accumulator = 0;

// Wire gauge step divisors (Rotor Steps * Microstep Factor / Follower Step)
byte wire_step_divisor[] = {255, 255, 16, 10, 15, 18, 255, 255, 255, 255, 255};
byte awg_sizes[] = {16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36};

static int rotor_step_count = 0; // count rotor steps
static unsigned long lastRotorStepTime = 0;
static bool rotorStepState = LOW;

//LCD
unsigned long lastLCDUpdate = 0;      // Last time LCD was updated
const unsigned long LCD_UPDATE_MS = 100; // Update LCD every 100 ms

void setRatio(){
    switch (awg_sizes[wire_gage_index]) {
      case 22: wire_pitch_mm = 0.64; break;
      case 24: wire_pitch_mm = 0.51; break;
      case 26: wire_pitch_mm = 0.40; break;
      case 28: wire_pitch_mm = 0.32; break;
      default: wire_pitch_mm = 0.50; break; // default fallback
    }

    // Numerator: how many follower microsteps per rotor microstep
    FOLLOWER_STEP_NUMERATOR = (long)(STEPS_PER_MM * wire_pitch_mm * FOLLOWER_MICROSTEP_FACTOR*1.75);
    FOLLOWER_STEP_DENOMINATOR = (long)(ROTOR_STEPS_PER_TURN * ROTOR_MICROSTEP_FACTOR);

    accumulator = 0;
}

void setMicrostepping(int mode, int ms1, int ms2, int ms3) {
  pinMode(ms1, OUTPUT);
  pinMode(ms2, OUTPUT);
  pinMode(ms3, OUTPUT);

  switch (mode) {
    case 1:  digitalWrite(ms1, LOW);  digitalWrite(ms2, LOW);  digitalWrite(ms3, LOW);  break;
    case 2:  digitalWrite(ms1, HIGH); digitalWrite(ms2, LOW);  digitalWrite(ms3, LOW);  break; // Follower: Half-step
    case 4:  digitalWrite(ms1, LOW);  digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW);  break;
    case 8:  digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, LOW);  break; // Rotor: Eighth-step
    case 16: digitalWrite(ms1, HIGH); digitalWrite(ms2, HIGH); digitalWrite(ms3, HIGH); break;
    default: digitalWrite(ms1, LOW);  digitalWrite(ms2, LOW);  digitalWrite(ms3, LOW);  break;
  }
}

// Function to move the follower motor by one step
void follower_step() {
  digitalWrite(follower_step_pin, HIGH); delayMicroseconds(FOLLOWER_STEP_DELAY);
  digitalWrite(follower_step_pin, LOW);  delayMicroseconds(FOLLOWER_STEP_DELAY);

  if (digitalRead(follower_dir_pin) == HIGH) {
    current_follower_steps++;
  } else {
    current_follower_steps--;
  }
}

void setup() {
  // 1. Setup Input Pins (Buttons)
  pinMode(up_button, INPUT_PULLUP);
  pinMode(dn_button, INPUT_PULLUP);
  pinMode(ent_button, INPUT_PULLUP);
  pinMode(jog_left, INPUT_PULLUP);
  pinMode(jog_right, INPUT_PULLUP);

  // Attach Bounce2 objects
  upBounce.attach(up_button);
  upBounce.interval(DEBOUNCE_MS);
  dnBounce.attach(dn_button);
  dnBounce.interval(DEBOUNCE_MS);
  entBounce.attach(ent_button);
  entBounce.interval(DEBOUNCE_MS);
  jogLeftBounce.attach(jog_left);
  jogLeftBounce.interval(DEBOUNCE_MS);
  jogRightBounce.attach(jog_right);
  jogRightBounce.interval(DEBOUNCE_MS);

  // 2. Setup Output Pins (Motor Control)
  pinMode(rotor_step_pin, OUTPUT);
  pinMode(rotor_dir_pin, OUTPUT);
  pinMode(follower_dir_pin, OUTPUT);
  pinMode(follower_step_pin, OUTPUT);
  pinMode(reset_sleep_pin, OUTPUT);

  // 3. Stepper Driver Enable/Reset
  digitalWrite(reset_sleep_pin, LOW);
  delay(50);
  digitalWrite(reset_sleep_pin, HIGH);
  Serial.begin(9600);

  // 4. Set Microstepping Modes
  setMicrostepping(FOLLOWER_MICROSTEP_FACTOR, follower_ms1, follower_ms2, follower_ms3);
  setMicrostepping(ROTOR_MICROSTEP_FACTOR, rotor_ms1, rotor_ms2, rotor_ms3);
  setRatio();
  digitalWrite(rotor_dir_pin, LOW);

  // 5. Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Number of turns:");
}

// Helper function to handle menu value changes and auto-repeat
bool handle_menu_change(int& value, int min_val, int max_val, int single_step) {
  bool changed = false;
  unsigned long current_time = millis();

  // Check for UP press/hold
  if (upBounce.read() == LOW) {
    if (upBounce.fell()) { // Single press action
      value += single_step;
      hold_start_time = current_time;
      hold_duration = 0;
      changed = true;
    }
    // Handle auto-repeat after HOLD_DELAY_MS
    else if (current_time - hold_start_time > HOLD_DELAY_MS && current_time - last_menu_action > REPEAT_RATE_MS) {
      unsigned long current_hold_duration = current_time - hold_start_time;
      value += (current_hold_duration > 5000) ? 50 : (current_hold_duration > 2000) ? 10 : single_step;
      last_menu_action = current_time;
      changed = true;
    }
  }

  // Check for DOWN press/hold
  else if (dnBounce.read() == LOW) {
    if (dnBounce.fell()) { // Single press action
      value -= single_step;
      hold_start_time = current_time;
      hold_duration = 0;
      changed = true;
    }
    // Handle auto-repeat after HOLD_DELAY_MS
    else if (current_time - hold_start_time > HOLD_DELAY_MS && current_time - last_menu_action > REPEAT_RATE_MS) {
      unsigned long current_hold_duration = current_time - hold_start_time;
      value -= (current_hold_duration > 5000) ? 50 : (current_hold_duration > 2000) ? 10 : single_step;
      last_menu_action = current_time;
      changed = true;
    }
  }

  // Reset hold timing if button is released or no buttons are active
  if (upBounce.rose() || dnBounce.rose() || (upBounce.read() == HIGH && dnBounce.read() == HIGH)) {
    if (upBounce.read() == HIGH && dnBounce.read() == HIGH) {
      last_menu_action = 0;
    }
    // hold_start_time and hold_duration are managed in the loop() function for go-back logic
  }
  // Constrain value
  if (value > max_val) value = max_val;
  if (value < min_val) value = min_val;

  return changed;
}

void loop() {
  // --- IMPORTANT: UPDATE ALL BOUNCE OBJECTS ---
  upBounce.update();
  dnBounce.update();
  entBounce.update();
  jogLeftBounce.update();
  jogRightBounce.update();

  // Read debounced press/release for single actions
  bool up_fell = upBounce.fell();
  bool ent_fell = entBounce.fell();

  // Read debounced current state for jogging and hold detection
  byte up_is_pressed = (upBounce.read() == LOW) ? 1 : 0;
  byte dn_is_pressed = (dnBounce.read() == LOW) ? 1 : 0;
  byte ent_is_pressed = (entBounce.read() == LOW) ? 1 : 0;
  byte jog_left_pressed = (jogLeftBounce.read() == LOW) ? 1 : 0;
  byte jog_right_pressed = (jogRightBounce.read() == LOW) ? 1 : 0;

  // Define current state variables for long-press timing
  byte move_state_current = up_is_pressed + dn_is_pressed * 2;
  byte ent_state_current = ent_is_pressed;

  // Long press timing for hold-to-go-back or special actions.
  if (move_state_current != 0 || ent_state_current != 0) {
    if (move_state_current != move_state_last || ent_state_current != ent_state) {
      hold_start_time = millis();
      hold_duration = 0;
    } else {
      hold_duration = millis() - hold_start_time;
    }
  } else {
    hold_start_time = 0;
    hold_duration = 0;
  }

  // Update global state trackers for the next loop iteration
  move_state_last = move_state_current;
  ent_state = ent_state_current;

  // --- STATE MACHINE ---
  switch (program_state) {
    case 0: { // Set Turns
      lcd.setCursor(0, 1);
      lcd.print("      ");
      lcd.setCursor(0, 1);
      lcd.print(n_turns);

      // Use helper function for debounced menu changes
      handle_menu_change(n_turns, 0, 9999, 1);

      if (ent_fell) {
        if (n_turns > 0) {
          program_state++;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wire Gauge (AWG):");
        } else {
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("Invalid number");
          lcd.setCursor(0, 1); lcd.print("of turns!");
          delay(2000);
          lcd.clear();
          lcd.setCursor(0, 0); lcd.print("Number of turns:");
        }
      }
      break;
    }

    case 1: { // Set Wire Gauge
      lcd.setCursor(0, 1);
      lcd.print(awg_sizes[wire_gage_index]); lcd.print("   ");

      int temp_index = (int)wire_gage_index;
      if (handle_menu_change(temp_index, 0, 10, 1)) {
        wire_gage_index = (byte)temp_index;
      }

      // Back to previous screen (DN Hold)
      if (dn_is_pressed && hold_duration > 1500) {
        program_state = 0;
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Going back...");
        delay(1000);
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Number of turns:");
        hold_duration = 0;
      }

      if (ent_fell) {
        program_state++;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Core Diameter (mm):");
      }
      break;
    }

    case 2: { // Set Core Diameter
      lcd.setCursor(0, 1);
      lcd.print(core_diameter_mm); lcd.print("mm    ");

      handle_menu_change(core_diameter_mm, 1, 99, 1);

      // Back to previous screen (DN Hold)
      if (dn_is_pressed && hold_duration > 1500) {
        program_state--;
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Going back...");
        delay(500);
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Wire Gauge (AWG):");
        hold_duration = 0;
      }
      if (ent_fell) {
        program_state++;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Coil Length (mm):");
      }
      break;
    }

    case 3: { // Set Coil Length
      lcd.setCursor(0, 1);
      lcd.print(coil_length_mm); lcd.print("mm    ");

      handle_menu_change(coil_length_mm, 5, 200, 1);

      // Back to previous screen (DN Hold)
      if (dn_is_pressed && hold_duration > 1500) {
        program_state--;
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Going back...");
        delay(500);
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Core Diameter (mm):");
        hold_duration = 0;
      }
      if (ent_fell) {
        follower_max_steps = 5500;//(long)coil_length_mm * STEPS_PER_MM;
        program_state++;
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("T:"); lcd.print(n_turns);
        lcd.setCursor(7, 0); lcd.print("AWG:"); lcd.print(awg_sizes[wire_gage_index]);
        lcd.setCursor(0, 1); lcd.print("L:"); lcd.print(coil_length_mm); lcd.print("mm");
        lcd.setCursor(9, 1); lcd.print("D:"); lcd.print(core_diameter_mm); lcd.print("mm");
      }
      break;
    }

    case 4: { // Confirm Settings
      lcd.setCursor(14, 0); lcd.print("OK?");
      // Back to previous screen (DN Hold)
      if (dn_is_pressed && hold_duration > 1500) {
        program_state--;
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Going back...");
        delay(500);
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Coil Length (mm):");
        hold_duration = 0;
      }
      if (ent_fell) {
        program_state++;
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("JOG Guide: D5/D6");
        lcd.setCursor(0, 1); lcd.print("SET HOME: D3 (UP)");
      }
      break;
    }

    case 5: { // Jogging and Manual Home Position Setup (PRE-WINDING)
      // Manual Jogging Logic
      if (jog_left_pressed) {
        digitalWrite(follower_dir_pin, LOW);
        follower_step();
      } else if (jog_right_pressed) {
        digitalWrite(follower_dir_pin, HIGH);
        follower_step();
      }

      // Check for MANUAL HOME SET (UP button, D3, single press)
      if (up_fell) {
        current_follower_steps = 0; // Set current position as zero
        lcd.setCursor(0, 0); lcd.print("HOME SET to ZERO.");
        lcd.setCursor(0, 1); lcd.print("D4(ENT) to Wind.");
      }

      // Back to Settings (DN Hold)
      if (dn_is_pressed && hold_duration > 2000) {
        program_state--;
        lcd.clear(); lcd.setCursor(0, 0); lcd.print("Going back...");
        delay(500);
        lcd.clear();
        hold_duration = 0;
      }

      // Start Winding (Enter single press, D4)
      if (ent_fell) {
        current_follower_steps = 0;
        total_counter = 0;
        wind_direction = 0;
        digitalWrite(follower_dir_pin, HIGH);
        digitalWrite(rotor_dir_pin, LOW);

        // ACCURATE GEARING INITIALIZATION
        accumulator = 0; // Reset fractional step accumulator

        program_state++;
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Starting coil...");
        delay(500);
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Turns:");
        lcd.setCursor(10, 0); lcd.print(n_turns);
        lcd.setCursor(0, 1); lcd.print("Wound: 0/0");
      }
      break;
    }

    case 6: { // Winding in Progress
        
        unsigned long current_micros = micros();
        unsigned long current_millis = millis();

        // --- Task 1: Rotor Stepping (Non-Blocking) ---
        if (current_micros - lastRotorStepTime >= ROTOR_STEP_DELAY_US) {
            lastRotorStepTime = current_micros;
            
            if (rotorStepState == LOW) {
                // Rising Edge: Step the rotor
                rotorStepState = HIGH;
                digitalWrite(rotor_step_pin, HIGH);
                // --- YOUR CORRECT GEARING LOGIC ---
                accumulator += FOLLOWER_STEP_NUMERATOR;
                if (accumulator >= FOLLOWER_STEP_DENOMINATOR) {
                    accumulator -= FOLLOWER_STEP_DENOMINATOR;
                    follower_step();
                    Serial.println(current_follower_steps);
                    

                    // Boundary check and reversal
                    if (current_follower_steps >= follower_max_steps) {
                      rotorStepState = LOW;
                      digitalWrite(rotor_step_pin, LOW);
                      Serial.println("Changing Direction");
                      
                      for (int i = 0; i < BACKLASH_STEPS; i++) {
                            follower_step(); 
                            Serial.println("In Follower ");
                          }
                        current_follower_steps = follower_max_steps;
                        
                          // reverse
                        digitalWrite(follower_dir_pin, LOW);
                        wind_direction = 1;
                    } 
                    else if (current_follower_steps <= 0) {
                      rotorStepState = LOW;
                      digitalWrite(rotor_step_pin, LOW);
                      Serial.println("Changing Direction");
                      
                                for (int i = 0; i < BACKLASH_STEPS; i++) {
                          follower_step(); 
                        }
                        current_follower_steps = 0;
                        digitalWrite(follower_dir_pin, HIGH); 
                        // forward
                   
                        wind_direction = 0;
                    }

                    // Count a turn when a full pass is done
                    if ((wind_direction == 1 && current_follower_steps == 0) ||
                        (wind_direction == 0 && current_follower_steps == follower_max_steps)) {
                        total_counter++;
                    }
                }
                // --- END GEARING LOGIC ---

            } else {
                // Falling Edge: Reset for next step
                rotorStepState = LOW;
                digitalWrite(rotor_step_pin, LOW);
            }
        } // End Task 1

        // --- Task 2: LCD Update (MOVED OUTSIDE - runs independently) ---
        if (current_millis - lastLCDUpdate >= LCD_UPDATE_MS) {
            lastLCDUpdate = current_millis;
            lcd.setCursor(7, 1);
            lcd.print(total_counter); // REMEMBER: This is Layers
            lcd.print("/");
            lcd.print(n_turns); // REMEMBER: This is Layers
        } // End Task 2

        // --- Task 3: Check for Completion ---
        if (total_counter >= n_turns) { // Should be n_layers
            program_state = 7;
        }
        
        // --- Task 4: Check for E-Stop (now runs every loop) ---
        if (ent_fell) {
            program_state = 7; // Or a new "PAUSED" state
        }
        break;
    }

    case 7: { // Finished / Pause State
      digitalWrite(reset_sleep_pin, LOW);
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("WINDING COMPLETE!");
      lcd.setCursor(0, 1); lcd.print("T: "); lcd.print(total_counter); lcd.print(" (MENU-UP)");

      if (up_fell) { // Press UP to restart/go to menu
        digitalWrite(reset_sleep_pin, HIGH);
        program_state = 0;
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Number of turns:");
      }
      break;
    }
  }
}
