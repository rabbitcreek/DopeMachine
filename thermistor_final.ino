// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 100000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000  
#define lightPin 12
#define lightSwitch 13
#define relayPin1 6
#define servoLow 30
#define servoHigh 84

#define tempA 500
bool clue=false;
#include <Adafruit_NeoPixel.h>
float timeNow=0;

#include <Adafruit_TiCoServo.h>
Adafruit_TiCoServo myservo; // create servo object to control a servo 

int pos = 0;    // variable to store the servo position

#define PIN 9  

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, 9, NEO_GRB + NEO_KHZ800);

/*
 5 "flames" of 3 pixels each.
 Each flame can have a brightness of 0 to 254 (play with this scale)
 Eventually, light up center pixel of three first then the sides. This version will just distribute amongst the 3 pixels.

*/

#define NUMBER_OF_FLAMES 4 // depends on number of neopixel triplets. 5 for 16 NeoPixel ring. 4 for 12 NeoPixel ring
#define FLICKER_CHANCE 5 // increase this to increase the chances an individual flame will flicker

uint32_t rez_range = 256*3;
#define D_ false

// console buttons:
struct flame_element{
  int brightness;
  int step;
  int max_brightness;
  long rgb[3];
  byte state;
  } flames[5];
  
  int new_brightness = 0;
  unsigned long rgb[3]; //reusable temporary array
  uint8_t scaleD_rgb[3];
  byte acc;
 
 #define SCALERVAL 256*3
 const int flamecolors[22][3] = {
{ SCALERVAL, 0,  0},
{ SCALERVAL, 0,  0},
{ SCALERVAL, 0,  0},
{ SCALERVAL, 0,  0},
{ SCALERVAL, 0,  0},
{ SCALERVAL, 0,  0},
{ SCALERVAL, 0,  0},
{ SCALERVAL, 0,  0},
{ SCALERVAL, SCALERVAL*.4,  },
{ SCALERVAL, SCALERVAL*.4,  0},
{ SCALERVAL, SCALERVAL*.4,  0},
{ SCALERVAL, SCALERVAL*.4,  0},
{ SCALERVAL, SCALERVAL*.3,  0},
{ SCALERVAL, SCALERVAL*.3,  0},
{ SCALERVAL, SCALERVAL*.3,  0},
{ SCALERVAL, SCALERVAL*.3,  0},
{ SCALERVAL, SCALERVAL*.3,  0},
{ SCALERVAL, SCALERVAL*.3,  0},
{ SCALERVAL, SCALERVAL*.3,  },
{ SCALERVAL, 0,  0}, // white
{ SCALERVAL, 0,  SCALERVAL}, // that one blue flame
{ SCALERVAL,  SCALERVAL*.3,  SCALERVAL*.5}
};

  int servoPos;
 
 
int samples[NUMSAMPLES];
 
void setup(void) {
  myservo.attach(10);
  pinMode(lightPin,OUTPUT); 
  pinMode(lightSwitch,INPUT_PULLUP);
  pinMode(relayPin1,OUTPUT);
  
  digitalWrite(relayPin1,LOW);
  
  Serial.begin(9600);
  analogReference(EXTERNAL);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  randomSeed(analogRead(4));
  InitFlames();
  myservo.write(servoLow);
  servoPos=1;
  
}
 
void loop(void) {
 
  
  
 
  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;
 
  Serial.print("Average analog reading "); 
  Serial.println(average);
 
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  Serial.print("Thermistor resistance "); 
  Serial.println(average);
 
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
 steinhart=steinhart*1.8+32;
  Serial.print("Temperature "); 
  Serial.print(steinhart);
  Serial.println(" *F");

  delay(50);
  if (!digitalRead(lightSwitch))
  {
    if (servoPos==1) {
      for (int i=servoLow;i<servoHigh;i++)
      {
        myservo.write(i);
        delay(15);
      }
      
      delay(100);
      digitalWrite(lightPin,HIGH);
      servoPos=0;
    }
  if(steinhart<tempA)
  {
    digitalWrite(relayPin1, HIGH);
  }
  else if(steinhart>tempA)
  {
    clue=true;
    digitalWrite(relayPin1,LOW);
  }
  
  if(clue)
  { 
    timeNow=millis();
    while(millis()-timeNow<5000)
    {
    for(byte flame_count=0; flame_count<NUMBER_OF_FLAMES; flame_count++) {
    switch(flames[flame_count].state){
      case 0: // reset
        CreateNewFlame(flame_count);
      break;
      
      case 1: //increasing
        new_brightness = flames[flame_count].brightness + flames[flame_count].step;
        if (new_brightness > flames[flame_count].max_brightness){
          UpdateFlameColor(flame_count, flames[flame_count].max_brightness);
          flames[flame_count].brightness = flames[flame_count].max_brightness;
          flames[flame_count].step = GetStepSize(); // pick a different speed for flame going out
          flames[flame_count].state = 2;
        } else {
          UpdateFlameColor(flame_count, new_brightness);
          flames[flame_count].brightness = new_brightness;
        }

      break;
      
      
      case 2: //decreasing
        new_brightness = flames[flame_count].brightness - flames[flame_count].step;
        // chance to flicker/rekindle:
        if (random(new_brightness) < FLICKER_CHANCE){
          // rekindle:
          flames[flame_count].state = 1; //increase again
          flames[flame_count].brightness = max(GetMaxBrightness(), flames[flame_count].brightness);
          flames[flame_count].step = GetStepSize(); 
        
        } else {
          if (new_brightness <1){
            flames[flame_count].state = 0; // bottomed out - reset to next flame
            flames[flame_count].brightness = 0;
             UpdateFlameColor(flame_count, 0);
          } else {
            UpdateFlameColor(flame_count, new_brightness);
             flames[flame_count].brightness = new_brightness;
          }
        }
      break;
    }
  
  }
   strip.show();
   delay(22);
  

    }
    

  }
  
  else 
  {
    int colorTemp0=constrain(steinhart,70,tempA);
    int colorTemp1=map(colorTemp0,70,tempA,255,0);
    int colorTemp2=map(colorTemp0,70,tempA,0,255);
    
    for (int j=0; j<colorTemp1; j=j+5)
    {
    for (int i=0; i<12;i++)
    { 
       strip.setPixelColor(i,colorTemp2,0,j);
       
    }
    strip.show();
    delay(50);
    }
    
    for (int j=colorTemp1; j>0; j=j-5)
    {
    for (int i=0; i<12;i++)
    { 
       strip.setPixelColor(i,colorTemp2,0,j);
       
    }
    strip.show();
    delay(50);
    }
    
    //for(int i=0;i<12;i++)strip.setPixelColor(i,0,0,0);
    //strip.show();
    
  }
  }
  else if(digitalRead(lightSwitch))
  {
    if(!servoPos){
      for(int i=servoHigh;i>servoLow;i--){
         myservo.write(i);
         delay(15);
      }
     
      delay(100);
      servoPos=1;
    }
    for(int j=0;j<128;j++){
      
    
    for(int i=0;i<12;i++){
      strip.setPixelColor(i,j,j,0);
    }
    strip.show();
    delay(10);
    }
     for(int j=128;j>0;j--){
      
    
    for(int i=0;i<12;i++){
      strip.setPixelColor(i,j,j,0);
    }
    strip.show();
    delay(10);
    }
    clue=false;
    digitalWrite(relayPin1,LOW);
    digitalWrite(lightPin,HIGH);
    delay(500);
      
    
    
      digitalWrite(lightPin,LOW);
      delay(50);
    
    
  }
  
}


void InitFlames(){
// Sets initial states in flames array
  for(byte i=0; i<NUMBER_OF_FLAMES; i++) {
    flames[i].state=0;
    
  }
}


void UpdateFlameColor(byte flame_num, int new_brightness){
// 
  uint32_t c = 0;
  uint32_t color_channel_value;
  byte rgb_channel;
  
  new_brightness = min(new_brightness, flames[flame_num].max_brightness);
  

  for(byte rgb_channel=0; rgb_channel<3; rgb_channel++) {
    color_channel_value = flames[flame_num].rgb[rgb_channel];
    color_channel_value = color_channel_value * (uint32_t)new_brightness; // keep it long
    color_channel_value = color_channel_value/(uint32_t)rez_range;
    rgb[rgb_channel] = max(0L,color_channel_value);
  } // step through R G B



  // spread possible values of 0 -768 across 3 pixels
  for(byte sub_pixel=0; sub_pixel<3; sub_pixel++) {
    for(byte i=0; i<3; i++) { // rgb
      acc = rgb[i]/3;
      byte d = rgb[i]%3;
      if (sub_pixel < d){
        acc++;
      }
      scaleD_rgb[i] = acc;
      
    }
    c = strip.Color(scaleD_rgb[0],scaleD_rgb[1], scaleD_rgb[2]);
    strip.setPixelColor(flame_num*3 + sub_pixel, c);
  }
  
}


void CreateNewFlame(byte flame_num){
  flames[flame_num].step = GetStepSize();
 flames[flame_num].max_brightness = GetMaxBrightness();

  flames[flame_num].brightness = 0;
  flames[flame_num].state = 1;
  byte color_index = random(22);
  for(byte i=0; i<3; i++) {
    flames[flame_num].rgb[i] = flamecolors[color_index][i];
  }
 
}

int GetStepSize(){
   return random(70)+1;
}
int GetMaxBrightness(){
  int retVal;
//  retVal = random(rez_range/4) +  random(rez_range/4) + random(rez_range/4) + rez_range/4 +1; // bell curve
//  retVal = random(rez_range*3/4) +  rez_range/4; // flat distribution
    retVal = random(rez_range/2) +  rez_range/2; // brighter flat distribution
    return retVal;
    
}

 
