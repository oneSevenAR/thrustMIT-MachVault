#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

byte up_button = 3;
byte dn_button = 2;
byte ent_button = 4;
byte jog_left = 5;
byte jog_right = 6;

byte move_state = 0;
byte move_state_last = 0;
byte ent_state = 0;
byte ent_state_last = 0;
byte jog_state = 0;

byte program_state = 0;
byte program_state_last = 0;

int n_turns = 100;
long hold_start_time = 0;
int hold_duration = 0;
byte wire_gage_index = 0;

int coil_length = 25;
int turns_per_layer = 0;
long pulse_time = 0;
int time_since_pulse = 0;

int total_counter = 0;
int layer_counter = 0;
int layer_number = 1;
int wind_direction = 1;


byte wire_step_divisor[] = {255, 255, 255, 255, 15, 18, 255, 255, 255, 255, 255}; //255 means i havent calibrated the number yet
byte awg_sizes[] = {16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36};


//added for stepper rotor variant

byte switch_state = 0;
byte switch_state_last = 1;
byte follower_direction = 0;
byte follower_steps_index = 1;


void setup() 
{
  pinMode(up_button, INPUT_PULLUP);
  pinMode(dn_button, INPUT_PULLUP);
  pinMode(ent_button, INPUT_PULLUP);
  pinMode(jog_left, INPUT_PULLUP);
  pinMode(jog_right, INPUT_PULLUP); 

  pinMode(4, INPUT_PULLUP);//LEFT LIMIT
  pinMode(9, INPUT_PULLUP);//RIGHT LIMIT

  pinMode(8, OUTPUT); //Rotor Step
  pinMode(7, OUTPUT); //Rotor direction
  
  pinMode(10, OUTPUT); //Follower direction
  pinMode(11, OUTPUT); //Follower step
  pinMode(13, OUTPUT); //Follower & Rotor RST
  
  //Pulse the sleep / reset on the stepper driver (doesn't work without this on A4988 boards)
  digitalWrite(13, LOW);
  delay(50);
  digitalWrite(13, HIGH);
  Serial.begin(9600);
  digitalWrite(7, HIGH);
  
  lcd.init();                    
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Number of turns:");
}

void loop() 
{  
  move_state = (1-digitalRead(up_button))+(1-digitalRead(dn_button))*2;
  ent_state = 1-digitalRead(ent_button);
  jog_state = (1-digitalRead(jog_left))+(1-digitalRead(jog_right))*2;  //2 = Right button press,  1 = Left button press

  switch (program_state)
  {
      ///==========================================STATE 0: TURN COUNT SELECTION=======================================================================
      case 0:
      lcd.setCursor(0,1);
      lcd.print("      ");
      lcd.setCursor(0,1);
      lcd.print(n_turns);
      
      
      if (move_state == 1)
      {
        if(hold_duration > 5000)
        {
          n_turns = n_turns+50;
        }
        else if (hold_duration > 2000)
        {
          n_turns = n_turns+10;
        }
        else
        {
          n_turns++;
        }

        if(n_turns > 9999)
        {
          n_turns = 9999;
        }
        if(move_state_last != move_state)
        {
          hold_start_time = millis();
        }
        hold_duration = millis() - hold_start_time;
        lcd.print("     ");
      }

      
      else if (move_state == 2 && n_turns > 0)
      {
        if(hold_duration > 5000)
        {
          n_turns = n_turns-50;
        }
        else if(hold_duration > 2000)
        {
          n_turns = n_turns-10;
        }
        else
        {
          n_turns--;
        }
        if(move_state_last != move_state)
        {
          hold_start_time = millis();
        }
        if(n_turns < 0)
        {
          n_turns = 0;
        }
        hold_duration = millis() - hold_start_time;
        lcd.print("     ");
      }

      
      else
      {
        hold_start_time = 0;
        hold_duration = 0;
      }

      if(ent_state == 1 && move_state == 0)
      {
        if(n_turns > 0)
        {
          program_state++;
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("...."); 
          delay(250);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Wire Gauge:"); 
        }
        else 
        {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Invalid number"); 
          lcd.setCursor(0,1);
          lcd.print("of turns!");
          delay(2000);
        }
      }
      
      move_state_last = move_state;
      delay(100);
      break;

      ///==========================================STATE 1: AWG SELECTION=======================================================================
      case 1:
      lcd.setCursor(0,1);
      lcd.print(awg_sizes[wire_gage_index]);    

      if (move_state == 1)
      {
        wire_gage_index++;
        if (wire_gage_index > 10)
        {
          wire_gage_index = 10;
        }
      }
      else if (move_state == 2)
      {
        if(wire_gage_index > 0)
        {
          wire_gage_index--;
        }
        if (move_state_last != move_state)
        {
          hold_start_time = millis();
        }      
        hold_duration = millis() - hold_start_time;   
      }
      else 
      {
        hold_start_time = 0;
        hold_duration = 0;
      }


      if(hold_duration > 1500)
      {
        program_state--;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Going back..."); 
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Number of turns:");
      }

      if(ent_state == 1 && move_state == 0)
      {
        program_state++; 
        lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(n_turns);
      lcd.setCursor(5,0);
      lcd.print("Turns");
      lcd.setCursor(11,0);
      lcd.print(awg_sizes[wire_gage_index]);
      lcd.setCursor(14,0);
      lcd.print("Ga");
      lcd.setCursor(0,1);
      lcd.print(coil_length);
      lcd.setCursor(6,1);
      lcd.print("mm");
      lcd.setCursor(10,1);
      lcd.print("OK?");
      }
      move_state_last = move_state;
      delay(200);
      break;
 
      
      
      ///==========================================STATE 2: VERIFICATION===========================================================================
      case 2:

      
      delay(250);  

      if (move_state == 2)
      {
        if(move_state_last != move_state)
        {
          hold_start_time = millis();
        }
        hold_duration = millis() - hold_start_time;
      }    
      else
      {
        hold_start_time = 0;
        hold_duration = 0;
      }
      if(hold_duration > 1500)
      {
        program_state--;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Going back..."); 
        delay(250);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Coil Length(mm):");
      }

      if(ent_state == 1 && move_state == 0)
      {
        program_state++; 
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("...."); 
        delay(250);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Please position");
        lcd.setCursor(0,1);
        lcd.print("wire guide...");
      }
      move_state_last = move_state;
      break;



      
      ///==========================================STATE 3: MANUAL JOG BEFORE START===============================================================
      case 3:

      if (move_state == 2)
      {
        if(move_state_last != move_state)
        {
          hold_start_time = millis();
        }
        hold_duration = millis() - hold_start_time;
        //Serial.println(hold_duration);
      }    
      else
      {
        hold_start_time = 0;
        hold_duration = 0;
      }
      
      if(hold_duration > 2000)
      {
        program_state--;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Going back..."); 
        delay(500);
        lcd.clear();
      }
 
      if(ent_state == 1 && move_state == 0)
      {
          program_state++; 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Starting coil..."); 
          delay(150);
          for(byte i = 0; i <16; i++)
          {
            lcd.setCursor(i,1);
            lcd.print(".");
            delay(20);
          }
          delay(250);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Winding in");
          lcd.setCursor(0,1);
          lcd.print("progress...");
      }
      move_state_last = move_state;
      break;



      ///==========================================STATE 4: Automated winding====================================================================
      case 4:

      if(total_counter < n_turns)
      {
        for(int j=0; j<n_turns; j++)
        {
          switch_state = (1-digitalRead(4)) + (1-digitalRead(9))*2; //1 = left switch, 2 = right switch
        }   
  
        if(switch_state == 2 && switch_state_last == 1)
        {
          digitalWrite(10, HIGH);
          follower_direction = 1-follower_direction;
          switch_state_last = switch_state;
        }

        if(switch_state == 1 && switch_state_last == 2)
        {
          digitalWrite(10, LOW);
          follower_direction = 1-follower_direction;
          switch_state_last = switch_state;
        }
      for(int i = 0; i < 1600; i++)
      {

        follower_steps_index++;
        
        if(follower_steps_index == wire_step_divisor[wire_gage_index])
        {
            digitalWrite(11, HIGH);
            digitalWrite(8, HIGH);
            delayMicroseconds(100);
            digitalWrite(11, LOW);
            digitalWrite(8, LOW);
            delayMicroseconds(100);
            follower_steps_index = 1; 
          }
          
        
        else
        {
          digitalWrite(8, HIGH);
          delayMicroseconds(100);
          digitalWrite(8, LOW);
          delayMicroseconds(100);
        }
     }
     total_counter++;
     }


     
      else
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(n_turns);
        lcd.print(" Turns");
        lcd.setCursor(0,1);
        lcd.print("Complete");
        program_state++;
        layer_counter = 0;
        layer_number = 1;
        total_counter = 0;
        wind_direction = 1;
      }
     break;
     ///==========================================STATE 6: Waiting for return to start====================================================================
     case 5:
     if(ent_state == 1 && move_state == 0)
      {
          program_state = 0; 
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Returning..."); 
          delay(500);
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Number of turns:");
      }
     break;
  }

  
  if(program_state != 4) //Lock out manual controls when winding is in process
  {
    if(jog_state == 1)
    {
      //manual jog
    }
    else if (jog_state == 2)
    {
      //manual jog
    }
  }
  

  ent_state_last = ent_state;
  program_state_last = program_state;
  delayMicroseconds(10);

}
