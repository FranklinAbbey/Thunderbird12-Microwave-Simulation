  //CSC-202 Final Project
  //Franklin Abbey
  //5 / 20 / 19

  // common defines and macros
  #include <hidef.h>
  // derivative information  
  #include <mc9s12dg256.h>     
  #pragma LINK_INFO DERIVATIVE "mc9s12dg256b"
  // interface to the assembly module 
  #include "main_asm.h" 
  #include <math.h>

  //*****************************************VARIABLES
  //switch
  int val_1 = 0;
  //thermistor
  signed int val_2;
  signed int Tf;
  //counter
  int num;
  //RTI
  int flag = 0, clock = 10;
  int sec_count = 100;
  //"i" used for loops
  unsigned int i;
  //"c,d,f, sum" used for getHexpadInput()
  int c, d, f;
  unsigned int sum = 0;
  //latch indicates microwave door openeing
  // 1 = closed (can operate)
  // 0 = open   (can't operate)
  int latch = 0;
  //char arrays used for SCI output to MINI IDE terminal
  char doorOpen[10] = {'D' , 'O' , 'O', 'R', ' ', 'O', 'P', 'E' , 'N' , '\n'};
  char running[8] = {'R','U','N','N','I', 'N','G', '\n'};
  char finished[17] = {'F','I','N','I','S','H','E','D',' ','C','O','O','K','I','N','G','\n'};
  char ready[6] = {'R','E','A','D','Y','\n'};


  //*****************************************BEEP
  //Sounds off active beeper once. 
  void beep() {
    PORTB ^= 0x20;
    ms_delay(50);
    PORTB ^= 0x20;      
  }

  //*****************************************HEXPAD INPUT
  //Receives input from the hexpad plugged into PORTB 
  //bit 0 - 7. If more than one number is given, makes the
  //necessary conversion to display input as one multi-digit
  //number. 
  //Note 1: Displays a message to LCD screen
  //Note 2: Checks if the given number is a valid time, 
  //        and makes a conversion if the user gives a 
  //        value that is greater than 60  
  int getHexpadInput() {

    clear_lcd();
    set_lcd_addr(0x00);
    type_lcd("Cook Time: ");
    //receive a digit from the hexpad
    c = getkey();
    d = 0;

    while(c != 13 && c <= 999) {
     
      c = getkey();
      //sound the beeper after every button press
      beep();
      
      //13 (D) = Enter
      if(c != 13) {
      
        //12 (C) = Clear
        if(c == 12) {
          d = 0;
        }
        //'*'(14)    = 30 sec 
        else if(c == 14) {
          return 30;
        } 
        //'#'(15)   = 1 min
        else if(c == 15) {
          return 100;
        } 
        else{  
          d += c;
        }
        
        set_lcd_addr(0x00);
        type_lcd("Cook Time: ");
        write_int_lcd(d);
        wait_keyup(); 
        
      } 
      //if c == 13 (Enter), end method and display
      //what we have so far
      else {
        f = d / 10;
        
        if(f > 59 && f < 100) {
          f -= 60;
          f += 100;
        }
        
        return f;
      }
      
      d = d * 10;
    
    }
    //make time-wise adjustment to number
    if(c > 59 && c < 100) {
      c -= 60;
      c += 100;
    }
    
    return c;

  }

  //*****************************************SERVO
  //Oscilates LEDs 0 - 3 on PORTB which
  //allows the stepper motor to spin
  void spinPlate() {

    ms_delay(1);
    PORTB ^= 0x05; 
    ms_delay(1);
    PORTB ^= 0x0A;
    
  }

  //*****************************************COUNTDOWN
  //Outputs a number to the LCD and checks if 
  //the number is zero. If so, outputs a message
  //to the LCD 
  void displayTimer(int x) {

    set_lcd_addr(0x00);
    type_lcd("Cook Time: ");
    write_int_lcd(x);
    
    if(x == 0) {
    
      clear_lcd();
      set_lcd_addr(0x28);
      type_lcd("Food is ready");
      
      //SCI output to terminal
      for(i = 0; i < 17; i++) {
        outchar0(finished[i]);
      }
      
      RTI_disable();
      
      //pulse the beeper  
      for(i = 0; i < 6; i++) {
        //bit 5, 2^5 = 32 (20 hex)
        PORTB ^= 0x20;
        ms_delay(500);
      }
      
      PORTB = 0x03;  
      
    }

  }

  //*****************************************INTERRUPT
  //RTI interrupt occurs every 10.4 ms, calls
  //the spinPlate() method to rotate stepper,
  //decrements "sec_count" which is initialized
  //to 100 to symbolize a second elapsing.
  //Note 1: "flag" is set once 1000ms has elapsed,
  //        and main program watches for the change
  void interrupt 7 handler() {

    spinPlate(); 
   
    sec_count--;
    if(sec_count <= 0) {
      flag = 1;
      sec_count = 100;
    }
    
    clear_RTI_flag();
     
  }

  //*****************************************MAIN
  void main(void) {

    // set system clock frequency to 24 MHz 
    PLL_init();
    //initialize servo motor
    servo76_init();
    //initialize LCD screen
    lcd_init();
    //initialize A/D converter
    ad0_enable();
    //set the Baud rate for both SCI ports
    SCI0_init(9600);
    SCI1_init(9600);
    //initialize the hexpad for use
    keypad_enable();
    // Port B lower nibble is output
    DDRB  = 0xFF;  
    //PORTB set for the Servo Motor     
    PORTB = 0x03;

    //over-arching while loop keeps program running 
    while(1) {

      clear_lcd();
      set_servo76(8000);
      
      while(!latch) {
        set_lcd_addr(0x28);
        type_lcd("DOOR OPEN!");
        val_1 = ad0conv(7);
        if(val_1 < 500 && !(latch)) {
          set_servo76(4500);
          latch = 1;
          
          for(i = 0; i < 6; i++) {
            outchar0(ready[i]);
          }
          
        }
      }
      
      //program starts waiting for input
      num = getHexpadInput();
      
      //latch = 1 symbolizes the "door" being closed
      if(latch) {
      
        for(i = 0; i < 8; i++) {
          outchar0(running[i]);
        }
        
        RTI_init();
        
      }
      
      if(num != 0) {
        
        while(num >= 0) {
         
          //reads ~1000 for high (unpressed), ~10 for low (pressed)
          val_1 = ad0conv(7);
          
          //if switch is pressed
          if(val_1 < 500 && latch) {
          
            set_servo76(8000);
            clear_lcd();
            set_lcd_addr(0x28);
            type_lcd("DOOR OPEN!");
            latch = 0;
            //SCI output to terminal
            for(i = 0; i < 10; i++) {   
              outchar0(doorOpen[i]);
            }
            
            RTI_disable();
            //waiting in position to compensate for long button push   
            while(val_1 < 500 ) {
              set_servo76(8000);
              val_1 = ad0conv(7);
            }
            
            
          } 
          //if switch is pressed and was previously pressed
          else if(val_1 < 500 && !(latch)) {
          
            clear_lcd();
            set_servo76(4500);
            //SCI output to terminal
            for(i = 0; i < 8; i++) {
              outchar0(running[i]);
            }
            
            RTI_init();
            latch = 1;
            
          }
            
          //"if flag"  = if one sec has elapsed
          //"if latch" = the door is closed
          if((flag) && (latch)) {
            
            displayTimer(num);
            
            //make time-wise adjustment to number
            if((num - 1) % 100 > 59) {
              num -= 41;
            }
            else {
              num--;
            }
            
            flag = 0;
            
          }
          
          if(latch) {
            
            val_2 = ad0conv(6);
            //Thermister conversion
            Tf = (((val_2 / 10) * -1) + 90);        
            set_lcd_addr(0x28);
            write_int_lcd(Tf + 30);
            type_lcd(" deg F");
          
          }
          
        }//while(num >=0)
        
        //output message to SCI terminal
        for(i = 0; i < 10; i++) {   
          outchar0(doorOpen[i]);
        }
      
      }
      
      //reset latch
      latch = 0;
       
    }//while(1)
    
    
  }//ends main() method





