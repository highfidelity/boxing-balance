/*
 Balance Platform code for Maple ret6

 Read a number of piezo pressure sensors at analog inputs and send data to PC 
 
*/

const int inputPinX = 0;
const int inputPinY = 1;
const int inputPinZ = 2;

int corners[4];
int sampleCount = 0;

const int debug = 0; 

unsigned int time;

void setup()
{
   pinMode(inputPinX, INPUT_ANALOG);
   pinMode(inputPinY, INPUT_ANALOG);
   pinMode(inputPinZ, INPUT_ANALOG);
   pinMode(BOARD_LED_PIN, OUTPUT);
   
   corners[0] = corners[1] = corners[2] = corners[3] = 0;
}

void loop()
{
   sampleCount++; 
   //  Read the instantaneous value of the pressure sensors
   corners[0] = analogRead(inputPinX);
   corners[1] = analogRead(inputPinY);
   corners[2] = analogRead(inputPinZ);
   
   //  Periodically send averaged value to the PC
   //  Print out the instantaneous deviation from the trailing average
   if (sampleCount % 10000 == 0)
   {
      if (debug)
      {
        SerialUSB.print("Measured = ");
        SerialUSB.print(corners[0]);
        SerialUSB.print(" ");
        SerialUSB.print(corners[1]);
        SerialUSB.print(" ");
        SerialUSB.print(corners[2]);
        SerialUSB.print(" ");
        SerialUSB.print(corners[3]);
        SerialUSB.println("");
      }
   }
}


  
