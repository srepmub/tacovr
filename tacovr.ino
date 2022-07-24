#include <nI2C.h>
#include <AccelStepper.h>

CI2C::Handle pixyR;
CI2C::Handle pixyL;

AccelStepper stepperR(AccelStepper::DRIVER, 3, 2);
AccelStepper stepperL(AccelStepper::DRIVER, 5, 4);
AccelStepper stepperZ(AccelStepper::DRIVER, 7, 6);

uint8_t buf[256];
uint8_t buf2[256];

uint8_t getbl[6];
uint8_t lampon[6];

int CALIBXL = 143; // fill in before use!!
int CALIBYL = 63; // fill in before use!!
int CALIBXR = 147; // fill in before use!!
int CALIBYR = 68; // fill in before use!!

//int triggerxl = CALIBXL;
//int triggeryl = CALIBYL;
//int triggerxr = CALIBXR;
//int triggeryr = CALIBYR;

int MAXSPEED = 2000; // max 2800
int MAXACCEL = 500; // max 25000
int MAXRSPEED = 500; // max 1000
int MAXRACCEL = 500; // max 1000

int newxr = 0;
int newyr = 0;
int newxl = 0;
int newyl = 0;

int INIT_L = 1;
int INIT_R = 2;
int WAIT_L = 3;
int STEP_L = 4;
int WAIT_R = 5;
int STEP_R = 6;
int state = 0;

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

    stepperL.setMaxSpeed(MAXSPEED);
    stepperL.setAcceleration(MAXACCEL);

    stepperR.setMaxSpeed(MAXSPEED);
    stepperR.setAcceleration(MAXACCEL);

    stepperZ.setMaxSpeed(MAXRSPEED);
    stepperZ.setAcceleration(MAXRACCEL);

    Serial.println("Let's go");

    state = INIT_L;
    Serial.println("INIT_L");
}

void RxCallbackL(const uint8_t status) {

    if (status == 0)
    {
        //Serial.println("R: received:");
        if(buf2[2] != 33) {
            Serial.println("L: unexpected payload type!");
            Serial.println(buf2[2]);
        }
        else if(buf2[3] != 14) {
            if(buf2[3] != 0) {
                Serial.println("L: unexpected payload size!");
                Serial.println(buf2[3]);
            }
        }
        else {
            uint8_t newy = buf2[11]<<8|buf2[10];
            newyl = newy;
            int posy = stepperZ.currentPosition();

            uint8_t newx = buf2[9]<<8|buf2[8];
            newxl = newx;
            int posx = stepperL.currentPosition();

//            if (state == WAIT_L && (abs(newy-triggeryl) > 5 || abs(newx-triggerxl) > 5)) {
            if (state == WAIT_L && (abs(newy-CALIBYL) > 8 || abs(newx-CALIBXL) > 8)) {
  
                state = STEP_L;
                Serial.println("STEP_L");
            }

            if(state == STEP_L || (state == INIT_L && abs(newy-CALIBYL) <= 20 && abs(newx-CALIBXL) <= 20)) {

                // stop condition
                if(abs(newy-CALIBYL) <= 5 && abs(newx-CALIBXL) <= 5) {
                    stepperL.moveTo(posx);
                    stepperZ.moveTo(posy);

                    if(state == INIT_L) {
                        state = INIT_R;
                        Serial.println("INIT_R");
                    } else {
                        state = WAIT_R;
                        Serial.println("WAIT_R");

//                        triggerxr = newxr;
//                        triggeryr = newyr;
                    }
                }

                // otherwise move
                else {
                    int newposy = posy-10*(newy-CALIBYL); // TODO more precise factor
                    stepperZ.moveTo(newposy);

                    int newposx = posx+10*(CALIBXL-newx); // TODO more precise factor
                    if(newposx > 900)
                      newposx = 900;
                    else if (newposx < -900)
                      newposx = -900;
                    stepperR.moveTo(-newposx);
                    stepperL.moveTo(newposx);
                }
            }
        }
    }
    else
    {
        Serial.print("R: rx error: ");
        Serial.println(status);
    }
}

void RxCallbackR(const uint8_t status) {
    if (status == 0)
    {
        //Serial.println("R: received:");
        if(buf[2] != 33) {
            Serial.println("R: unexpected payload type!");
            Serial.println(buf[2]);
        }
        else if(buf[3] != 14) {
            if(buf[3] != 0) {
                Serial.println("R: unexpected payload size!");
                Serial.println(buf[3]);
            }
        }
        else {
            uint8_t newy = buf[11]<<8|buf[10];
            int posy = stepperZ.currentPosition();
            newyr = newy;

            uint8_t newx = buf[9]<<8|buf[8];
            int posx = stepperR.currentPosition();
            newxl = newx;

//            if (state == WAIT_R && (abs(newy-triggeryr) > 5 || abs(newx-triggerxr) > 5)) {
            if (state == WAIT_R && (abs(newy-CALIBYR) > 8 || abs(newx-CALIBXR) > 8)) {
  
                state = STEP_R;
                Serial.println("STEP_R");
            }

            if (state == STEP_R || (state == INIT_R && abs(newy-CALIBYR) <= 20 && abs(newx-CALIBXR) <= 20)) {

                // check stop condition
                if(abs(newy-CALIBYR) <= 5 && abs(newx-CALIBXR) <= 5) { // TODO add speed (close to) 0 check
                    stepperR.moveTo(posx);
                    stepperZ.moveTo(posy);

                    state = WAIT_L;
                    Serial.println("WAIT_L");

//                    triggerxl = newxl;
//                    triggeryl = newyl;
                }

                // otherwise move
                else {
                    int newposy = posy-10*(CALIBYR-newy); // TODO more precise factor
                    stepperZ.moveTo(newposy);

                    int newposx = posx+10*(CALIBXR-newx); // TODO more precise factor
                    if(newposx > 900)
                        newposx = 900;
                    else if (newposx < -900)
                        newposx = -900;
                    stepperR.moveTo(newposx);
                    stepperL.moveTo(-newposx);
                }
            }

        }
    }
    else
    {
        Serial.print("R: rx error: ");
        Serial.println(status);
    }
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

        status = nI2C->Read(pixyL, (uint8_t*)buf2, 20, RxCallbackL);
        if (status) {
            Serial.print("L: read status: ");
            Serial.println(status);
        }

    }

    stepperR.run();
    stepperL.run();
    stepperZ.run();
}
