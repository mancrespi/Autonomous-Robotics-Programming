// Arduino 101 Motor Code
// Members: Emanuelle Crespi, Harsha Cuttari, Nikhil Badhami
//
// University of Maryland
// James A. Clark's School of Engineering
// Department of Electrical and Computer Engineering
//
// The following code is used to communicate logic voltages on the Arduino 101 to 
// the motor driver components on the robot.  Information is processed by the ping sensors
// to dodge obstacles by driving the correct wheel with a sufficient speed. Information from 
// serial is processed to have the wheels react in the same way.  This is so the camera processing
// that is done in from Kangaroo_Client.py may communicate with the wheels with specific messages 
// on where to turn. 
// 

const static int MAX_SPEED = 55;
const static int MAX_DIST = 9;
const static int SPEED_UP = 1;

//left
const static int motor1 = 6;
const static int b1_dig = 13;
const static int a1_dig = 7;
const static int m1_default = 40;
static int m1_speed = m1_default;
static int dir1 = 1;

//right
const static int motor2 = 5;
const static int b2_dig = 11;
const static int a2_dig = 8;
const static int m2_default = 50;
static int m2_speed = m2_default;
static int dir2 = 1;

//flags
static boolean leftObs = 0;
static boolean rightObs = 0;

//will toggle betwwen 2 and 4
static int pingPin = 2;

//logic for scan/escape mode
static boolean scan = false;
static int sFilter = 0;
static boolean escape = false;
static int eFilter = 0;
static boolean backup = false;
static int pFilter = 0;
static boolean pause = false;

void setup() {
  //wheel 1 setup
  pinMode(a1_dig,INPUT);
  pinMode(b1_dig,INPUT);
  pinMode(motor1, INPUT_PULLUP);

  //wheel 2 setup
  pinMode(a2_dig, INPUT);
  pinMode(b2_dig, INPUT);
  pinMode(motor2, INPUT_PULLUP);

  Serial.begin(115200);

  //Serial.begin(9600);
}


/* constantly drives the motor1 and motor2 with initializations in "setup()"
 * while toggling between pin2(left) and pin4(right)
 * on the ping sensors.  Once left and right have been read
 * m1_speed and m2_speed can be modified as needed to keep in line.
 * dir1 and dir2 can be 1/-1 for forward/reverse motion on respective wheels
 */
void loop() {
      static long duration, inches, cm;
      static long left;
      static long right;
      
        //Read values from Python app
        if(Serial.available() > 0) {
              int data = Serial.read();
              char str[2];
              str[0] = data;
              str[1] = '\0';
              //Serial.print("Arduino101: ");
              Serial.print(str);
          
              /*************************************
               * Routines to process for the the wheels
               * speeds & direction deepending on camera
               * 
               * Stop = 0 (48) 
               * Straight = 1 (49)
               * left = 2 (50)
               * right = 3 (51)
               * scan = 4 (52)
               * pause = 5 (53)
               * ***********************************
               * FlAGS/FILTERS Initializations
               * 
               * static boolean scan = false;
               * static int sFilter = 0;
               * 
               * static boolean pause = false;
               * static int pFilter = 0;
               * 
               *************************************/
              if( !leftObs && !rightObs ){ //for the camera
                   //no obstacles on the pings
                   //assume forward motion
                   dir1 = 1;
                   dir2 = 1;
                  
                  if( data == 51 ){             //right
                      m2_speed += SPEED_UP;
                      m1_speed = 0;
                      Serial.print("right");
                  }else if( data == 50 ){       //left
                      m2_speed = 0;
                      m1_speed += SPEED_UP;
                      Serial.print("left");
                  }else if( data == 48 ){       //stop
                      m2_speed = 0;
                      m1_speed = 0;
                      Serial.print("stop");
                  }else if( data == 49 ){       //straight
                      m2_speed = m2_default;
                      m1_speed = m1_default;
                      Serial.print("straight");
                  }else if( data == 52 ){       //scan
                      sFilter++;
                      if( sFilter >= 20 ){
                          if( sFilter >= 40 ){
                            dir2 = -1;
                          }else{
                            dir1 = -1;
                          }
                          
                          if( sFilter == 60 ) 
                            sFilter = 20;
                          
                          m2_speed = m2_default;
                          m1_speed = m1_default;
                      }else{
                          m2_speed = 0;
                          m1_speed = 0;
                      }
                      Serial.print("SCAN");
                  }else if( data == 53 ){     //pause/position
                      pFilter++;
                      if( pFilter >= 20 ){
                          pause = false;

                          //90 degree turn one way
                          if( pFilter < 40 ){
                            dir2 = -1;
                          }

                          //90 degree turn other way
                          if( pFilter >= 60 ){
                            dir1 = -1;
                          }

                          //set flags for logic on state
                          if( pFilter > 80 ){
                            pause = true;
                          }else{
                            pause = false;
                          }
                          
                          if( pause == false ){
                            m2_speed = m2_default;
                            m1_speed = m1_default;
                          }else{
                            m2_speed = 0;
                            m1_speed = 0;
                          }  
                          
                      }else{
                          pause = true;
                          m2_speed = 0;
                          m1_speed = 0;
                      }  
                  }else{
                      m2_speed = 0;
                      m1_speed = 0;
                      Serial.print("sum ting wong");
                  }
              
                  if ( data != 52 ){
                      scan = false;
                      sFilter = 0;
                  }
              }else{
                  //for the pings
                  //assume forward motion
                  dir1 = 1;
                  dir2 = 1;

                  /*************************************
                   * Routines to process for the the wheels
                   * speeds & direction deepending on pings. 
                   * 
                   * ***********************************
                   * //FLAGS/FILTERS Initializations
                   * static boolean leftObs = 0;
                   * static boolean rightObs = 0;
                   *
                   * static boolean escape = false;
                   * static int eFilter = 0;
                   * static boolean backup = false;
                   *************************************/
                  if( leftObs && !rightObs ){
                      //turning right
                      Serial.print("Dodge right");
                      m1_speed += SPEED_UP;
                      m2_speed = 0;
                  }else if( rightObs && !leftObs ){
                      //turning left
                      Serial.print("Dodge left");
                      m1_speed = 0;
                      m2_speed += SPEED_UP;
                  }else if( leftObs && rightObs && eFilter < 40 ){
                      if( data != 48 ) eFilter++;         //process this request for 40 cycles
                      Serial.print("TRAPPED"); //debug on SERIAL
                      m1_speed = 0;
                      m2_speed = 0;
                  }else{
                      m1_speed = 0;//m1_default;
                      m2_speed = 0;//m2_default;
                  }
              
                  //only gets here if it was trapped for 40 cycles (both pings read 9)
                  if ( eFilter >= 40 ){
                      //process this state in the filter
                      eFilter++;
                      m1_speed = m1_default;
                      m2_speed = m2_default;
                
                      //keep track of state flags
                      if( eFilter > 40 && eFilter <= 80 ){
                          //need to backup because I am trapped
                          Serial.print("backup"); //debug on SERIAL
                          backup = true;
                          escape = false;
                      }else if( eFilter > 80 && eFilter != 100 ){
                          //will escape after handling backup routine
                          Serial.print("Escape"); //debug on SERIAL
                          backup = false;
                          escape = true;
                      }else{
                          eFilter = 0;
                          backup = false;
                          escape = false;
                      }
                
                      //routine for backing up
                      if( backup == true ){
                          //reverse both wheels
                          dir1 = -1;
                          dir2 = -1;
                      }
                
                      //routine for escaping
                      if( escape == true ){
                          //reverse only one wheel
                          dir1 = -1;
                      }
                  }
                  //Done    
              }
      }
    
      //before driving the motor want to check that speed isn't over the max
      if( m1_speed == MAX_SPEED ){ m1_speed = m1_default; }
      if( m2_speed == MAX_SPEED ){ m2_speed = m2_default; }
      
      //motion on wheel1 and wheel2
      drive( dir1, motor1, m1_speed, a1_dig, b1_dig );
      drive( dir2, motor2, m2_speed, a2_dig, b2_dig );
    
      //process pingPin 2/pingPin 4
      pinMode(pingPin, OUTPUT);
      digitalWrite(pingPin, LOW);
      delayMicroseconds(2);
      digitalWrite(pingPin, HIGH);
      delayMicroseconds(5);
      digitalWrite(pingPin, LOW);
      pinMode(pingPin, INPUT);
      duration = pulseIn(pingPin, HIGH);
    
      inches = microsecondsToInches(duration);
      cm = microsecondsToCentimeters(duration);
    
      if( pingPin == 2 ){
        right = inches;
      }else if( pingPin == 4 ){
        left = inches;
      }else{
        Serial.print("??????????");     
      }
    
      //setting flag to signal left/right turn
      leftObs = (left <= MAX_DIST);
      rightObs = (right <= MAX_DIST);
      
      // toggles between pin2 and pin4 for the ping
      pingPin ^= 6;
}

/* Accepts appropriate pins for motor a, and b to  
 * power the motor at pwm speed mSpeed.  
 * dir = 1 will indicate forward motion
 * dir = -1 will indicate backward motion 
 */
void drive(int dir, int motor, int mSpeed, int a, int b) {  
  analogWrite(motor,mSpeed);

  switch( dir ){
    case 1:
      digitalWrite(a,HIGH);
      digitalWrite(b,LOW);
      break;

    case -1:
      digitalWrite(a,LOW);
      digitalWrite(b,HIGH);
      break;

    default:
      digitalWrite(a,HIGH);
      digitalWrite(b,LOW);
      Serial.print("Default in drive(...)???");
      break;
  }
  
}

long microsecondsToInches(long microseconds) {
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
  return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}
