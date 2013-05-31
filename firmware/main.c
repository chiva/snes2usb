#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "usbdrv.h"

/* Pin assignment:
 * 
 * PB0 (5) = D- (2 ws)
 * PB2 (7) = D+ (3 gn)
 * 
 * PB1 (6) = CLOCK (4)
 * PB3 (2) = LATCH (3)
 * PB4 (3) = DATA  (2)
*/

#define SNES_B       1
#define SNES_Y       2
#define SNES_SELECT  3
#define SNES_START   4
#define SNES_A       5
#define SNES_X       6
#define SNES_L       7
#define SNES_R       8

const char snesLatch = 1<<PB3;
const char snesClock = 1<<PB1;
const char snesData  = 1<<PB4;
// Customize the order in which buttons will be reported
char buttonOrder[]   = {SNES_X, SNES_A, SNES_B, SNES_Y,
                       SNES_L, SNES_R, SNES_SELECT, SNES_START};

static inline void snesLatchLow()  { PORTB &= ~snesLatch; }
static inline void snesLatchHigh() { PORTB |= snesLatch; }
static inline void snesClockLow()  { PORTB &= ~snesClock; }
static inline void snesClockHigh() { PORTB |= snesClock; }
static inline unsigned char snesGetData()   { return PINB & snesData; }

struct{
    unsigned char x;
    unsigned char y;
    unsigned char buttons;
}reportBuffer;                   /* buffer for HID reports */

static unsigned char lastRead[2];
static unsigned char lastReported[2];
static unsigned char idleRate;           /* in 4 ms units */

static void readSNES(void);
static char changedSNES(void);

const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = { /* USB report descriptor */
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,        // USAGE (Game Pad)
    0xa1, 0x01,        // COLLECTION (Application)
    0x09, 0x01,        //   USAGE (Pointer)
    0xa1, 0x00,        //   COLLECTION (Physical)
    0x09, 0x30,        //     USAGE (X)
    0x09, 0x31,        //     USAGE (Y)
    0x15, 0x00,        //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,  //     LOGICAL_MAXIMUM (255)
    0x75, 0x08,        //     REPORT_SIZE (8)
    0x95, 0x02,        //     REPORT_COUNT (2)
    0x81, 0x02,        //     INPUT (Data,Var,Abs)
    0xc0,              //   END_COLLECTION
    0x05, 0x09,        //   USAGE_PAGE (Button)
    0x19, 0x01,        //   USAGE_MINIMUM (Button 1)
    0x29, 0x08,        //   USAGE_MAXIMUM (Button 8)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,        //   REPORT_SIZE (1)
    0x95, 0x08,        //   REPORT_COUNT (8)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)
    0xc0               // END_COLLECTION
};

static void readSNES(void) {
    // Emulate SNES gamepad protocol to read the status of the buttons
    // http://www.gamefaqs.com/snes/916396-snes/faqs/5395
    unsigned char i=16, tmp=0;

    snesLatchHigh();
    _delay_us(12);
    snesLatchLow();

    while(i) {
        i--;

        _delay_us(6);
        snesClockLow();
        
        tmp <<= 1;	
        if(!snesGetData()) tmp++; 

        _delay_us(6);		
        snesClockHigh();

        if(i == 8) lastRead[0] = tmp;
    }
    lastRead[1] = tmp;
}

static char changedSNES(void) {
    if(lastRead[0] == lastReported[0] &&
       lastRead[1] == lastReported[1]) return 0;
    return 1;
}

static void buildReport(void) {
    char x, y;
    unsigned char lrcb1, lrcb2, button, i, temp, buttonNew=0;

    lastReported[0] = lrcb1 = lastRead[0];  // B,Y,Select,Start,Up,Down,Left,Right
    lastReported[1] = lrcb2 = lastRead[1];  // A,X,L,R,-,-,-,-

    y = x = 128;
    if(lrcb1 & 0x01) x = 255;
    if(lrcb1 & 0x02) x = 0;
    if(lrcb1 & 0x04) y = 255;
    if(lrcb1 & 0x08) y = 0;

    // Discard axis info and put 8 buttons state in a byte
    button = (lrcb1 & 0xF0) | (lrcb2 >> 4);

    // Order the buttons as stated at the beginning
    for(i=0; i<8; i++) {
        temp = button & (1<<((8-buttonOrder[i])));  // Take the state of the button
        temp >>= 8-buttonOrder[i];                  // Bring it to the first bit
        temp <<= i;                                 // Put it at the desired bit
        buttonNew |= temp;
    }

    reportBuffer.x = x;
    reportBuffer.y = y;
    reportBuffer.buttons = buttonNew;
}

usbMsgLen_t usbFunctionSetup(unsigned char data[8])
{
    usbRequest_t *rq = (void *)data;

    /* The following requests are never used. But since they are required by
     * the specification, we implement them in this example.
     */
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            usbMsgPtr = (void *)&reportBuffer;
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

// Called by V-USB after device reset
// Calibrate RC clock with the 1ms frame on USB reset
void hadUsbReset() {
    unsigned char step = 128;
    unsigned char trialValue = 0, optimumValue;
    int   x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
 
    /* do a binary search: */
    do{
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    // proportional to current real frequency
        if(x < targetValue)             // frequency still too low
            trialValue += step;
        step >>= 1;
    }while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; // this is certainly far away from optimum
    for(OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if(x < 0)
            x = -x;
        if(x < optimumDev){
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}

int __attribute__((noreturn)) main(void) {
    unsigned char i;

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */

    //Init SNES-Controller
    DDRB = snesLatch | snesClock;

    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i){             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
    sei();

    for(;;) { //main event loop
        wdt_reset();
        usbPoll();
        readSNES();
        // Only report if buttons state have changed
        if(usbInterruptIsReady() && changedSNES()){
            buildReport();
            usbSetInterrupt((void *)&reportBuffer, sizeof(reportBuffer));
        }
    }
}
