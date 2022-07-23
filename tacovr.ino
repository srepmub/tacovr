#include <nI2C.h>
#include <AccelStepper.h>

CI2C::Handle pixyR;
CI2C::Handle pixyL;

AccelStepper stepperR(AccelStepper::DRIVER, 3, 2);
AccelStepper stepperL(AccelStepper::DRIVER, 5, 4);
AccelStepper stepperZ(AccelStepper::DRIVER, 7, 6);

uint8_t buf[256];

uint8_t getbl[6];
uint8_t lampon[6];

int CALIBXL = 140; // fill in before use!!
int CALIBYL = 63; // fill in before use!!

int CALIBXR = 146; // fill in before use!!
int CALIBYR = 69; // fill in before use!!

int MAXSPEED = 2000; // max 2800
int MAXACCEL = 700; // max 25000

int MAXRSPEED = 500; // max 1000
int MAXRACCEL = 700; // max 1000
    
uint32_t lastpoll = 0;



void setup()
{
    // getBlocks request
    getbl[0] = 174;
    getbl[1] = 193;
    getbl[2] = 32;
    getbl[3] = 2;
    getbl[4] = 1; // only return sig 1
    getbl[5] = 1; // return max 1 block

    // lights on request
    lampon[0] = 174;
    lampon[1] = 193;
    lampon[2] = 22;
    lampon[3] = 2;
    lampon[4] = 1;
    lampon[5] = 1;

    Serial.begin(115200);

    pixyR = nI2C->RegisterDevice(0x54, 1, CI2C::Speed::FAST);
    pixyL = nI2C->RegisterDevice(0x55, 1, CI2C::Speed::FAST);

    nI2C->Write(pixyR, lampon, 6);
    nI2C->Write(pixyL, lampon, 6);

    stepperR.setMaxSpeed(MAXSPEED);
    stepperR.setAcceleration(MAXACCEL);

    stepperZ.setMaxSpeed(MAXRSPEED);
    stepperZ.setAcceleration(MAXRACCEL);

    Serial.println("Let's go");
}


void RxCallbackL(const uint8_t status) {
   return;
}


void RxCallbackR(const uint8_t status) {
    
    if (status == 0)
    {
        //Serial.println("R: received:");
        if(buf[2] != 33) {
            Serial.println("L: unexpected payload type!");
            Serial.println(buf[2]);
        }
        else if(buf[3] != 14) {
            Serial.println("R: unexpected payload size!");
            Serial.println(buf[3]);
        }
        else {
            uint8_t newy = buf[11]<<8|buf[10];
            int posy = stepperZ.currentPosition();

            uint8_t newx = buf[9]<<8|buf[8];
            int posx = stepperR.currentPosition();

            // stop
            if(abs(newy-CALIBYR) <= 5 && abs(newx-CALIBXR) <= 5) {         
                stepperR.moveTo(posx);
                stepperZ.moveTo(posy);
            }

            // move
            else {
                int newposy = posy-10*(CALIBYR-newy); // TODO more precise factor
                stepperZ.moveTo(newposy);
            
                int newposx = posx+10*(CALIBXR-newx); // TODO more precise factor
                if(newposx > 900)
                    newposx = 900;
                else if (newposx < -900)
                    newposx = -900;
                stepperR.moveTo(newposx);
            }
        }
    }
    else
    {
        Serial.print("R: rx error: ");
        Serial.println(status);
    }
}


void loop_test() 
{
/*    if( stepperZ.currentPosition() >= 490)
        stepperZ.moveTo(0);
    else if (stepperZ.currentPosition() <= 10)
        stepperZ.moveTo(500); */

    stepperZ.moveTo(100);
    stepperZ.run();
}


void loop()
{
    uint32_t m = micros();
    if(m > lastpoll+1000000/60) {
        lastpoll = m;

       
        uint8_t status = nI2C->Write(pixyR, getbl, 6);
        if (status) {
            Serial.print("R: write status: ");
            Serial.println(status);
        }

        status = nI2C->Write(pixyL, getbl, 6);
        if (status) {
            Serial.print("L: write status: ");
            Serial.println(status);
        }

        status = nI2C->Read(pixyR, (uint8_t*)buf, 20, RxCallbackR);
        if (status) {
            Serial.print("R: read status: ");
            Serial.println(status);
        }

        status = nI2C->Read(pixyL, (uint8_t*)buf, 20, RxCallbackL);
        if (status) {
            Serial.print("L: read status: ");
            Serial.println(status);
        }

    }  

    stepperR.run();
    stepperL.run();
    stepperZ.run();
}
