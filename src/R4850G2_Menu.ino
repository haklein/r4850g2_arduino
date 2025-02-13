/*
   Huawai R4850G2 rectifier canbus control


   Based on:

   https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
   https://github.com/craigpeacock/Huawei_R4850G2_CAN

   Improvements from dl1bz

   IMPORTANT:

   - adjust CAN.setClockFrequency() to match the crystal on the MCP2515 Board
   - put a termination jumper on J1, didn't work without for me
   - be careful with any Serial.print in the onReceive handler. This will lead to lost CAN frames.
   - this has been tested with an Arduino Mega 2560 and Nano
   - libraries used:
        Adafruit can library (forked from sandeepmistry/arduino-CAN):
        https://github.com/adafruit/arduino-CAN
        ArduinoMenu
        SSD1306Ascii
        ClickEncoder

   Mega 2560 SPI pins:

   2 - int
   10 - CS
   50 - SO
   51 - SI
   52 - SCK
*/

#define menuFont X11fixed7x14
#define fontW 7  // 7
#define fontH 15   // 15

#define statusFont Adafruit5x7

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3c ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

#define ROTARY_PIN_CLK 3
#define ROTARY_PIN_DT 4
#define ROTARY_PIN_SW 5

#define VOLT_LIMIT_LOW 41.5
#define VOLT_LIMIT_HIGH 58.5
#define AMP_LIMIT_LOW 0.0
#define AMP_LIMIT_HIGH 60.0

#define VOLT_DEFAULT 55.2 
#define AMP_DEFAULT 1.0

#define MENU_TIMEOUT 10  // sketch doesn't send CAN frames while in menu, so timeout early to prevent
                         // rectifier from defaulting to offline values
                         
#define MAX_DEPTH 1 // menu depth

#include <CAN.h>

#include <SPI.h>
#include <Wire.h>

#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

#include <TimerOne.h>

#include <ClickEncoder.h>

#include <menu.h>
#include <menuIO/SSD1306AsciiOut.h>
#include <menuIO/clickEncoderIn.h>
#include <menuIO/keyIn.h>
#include <menuIO/chainStream.h>

#define DEBUG 1

#define MAX_CURRENT_MULTIPLIER    20

#define R48xx_DATA_INPUT_POWER    0x70
#define R48xx_DATA_INPUT_FREQ   0x71
#define R48xx_DATA_INPUT_CURRENT  0x72
#define R48xx_DATA_OUTPUT_POWER   0x73
#define R48xx_DATA_EFFICIENCY   0x74
#define R48xx_DATA_OUTPUT_VOLTAGE 0x75
#define R48xx_DATA_OUTPUT_CURRENT_MAX 0x76
#define R48xx_DATA_INPUT_VOLTAGE  0x78
#define R48xx_DATA_OUTPUT_TEMPERATURE 0x7F
#define R48xx_DATA_INPUT_TEMPERATURE  0x80
#define R48xx_DATA_OUTPUT_CURRENT 0x81
#define R48xx_DATA_OUTPUT_CURRENT1  0x82

struct RectifierParameters
{
  float input_voltage;
  float input_frequency;
  float input_current;
  float input_power;
  float input_temp;
  float efficiency;
  float output_voltage;
  float output_current;
  float max_output_current;
  float output_power;
  float output_temp;
  float amp_hour;
};

using namespace Menu;

SSD1306AsciiWire oled;

ClickEncoder clickEncoder(ROTARY_PIN_CLK, ROTARY_PIN_DT, ROTARY_PIN_SW, 4); // last parameter: steps per notch
ClickEncoderStream encStream(clickEncoder, 1);

struct RectifierParameters rp;

unsigned long g_Time1000 = 0;

bool doPrint = false;

float volts = VOLT_DEFAULT;
float amps = AMP_DEFAULT;

result persist(eventMask e) {
  Serial.println(F("persist executed")); Serial.flush();
  Serial.println(volts);
  Serial.println(amps);
  r4850_set_voltage(volts, true);
  delay(250);
  r4850_set_current(amps, true);
  delay(250);
  return proceed;
}

result apply(eventMask e) {
  Serial.println(F("apply executed")); Serial.flush();
  Serial.println(volts);
  Serial.println(amps);
  r4850_set_voltage(volts, false);
  delay(250);
  r4850_set_current(amps, false);
  delay(250);
  return quit;
}

result doAlert(eventMask e, prompt &item);

MENU_INPUTS(in, &encStream);

MENU(mainMenu, "R4850 Settings", Menu::doNothing, Menu::noEvent, Menu::wrapStyle
     , FIELD(volts, "Volts", "V", VOLT_LIMIT_LOW, VOLT_LIMIT_HIGH, 1, 0.1, doNothing, noEvent, wrapStyle)
     , FIELD(amps, "Amps", "A", AMP_LIMIT_LOW, AMP_LIMIT_HIGH, 1, 0.1, doNothing, noEvent, wrapStyle)
     , OP(">Apply", apply, enterEvent)
     , OP(">Persist", persist, enterEvent)
     , EXIT("<Back")
    );

const panel panels[] MEMMODE = {{0, 0, 128 / fontW, 64 / fontH}};
navNode* nodes[sizeof(panels) / sizeof(panel)]; //navNodes to store navigation status
panelsList pList(panels, nodes, 1); //a list of panels and nodes
idx_t tops[MAX_DEPTH] = {0}; //store cursor positions for each level
SSD1306AsciiOut outOLED(&oled, tops, pList, 8, 1+((fontH - 1) >> 3) ); //oled output device menu driver
menuOut* constMEM outputs[] MEMMODE = {&outOLED}; //list of output devices
outputsList out(outputs, 1); //outputs list

NAVROOT(nav, mainMenu, MAX_DEPTH, in, out);

int r4850_ack(uint8_t *frame)
{
  bool error = frame[0] & 0x20;
  uint32_t value = __builtin_bswap32(*(uint32_t *)&frame[4]);

  switch (frame[1]) {
    case 0x00:
      Serial.print(F("Setting on-line voltage: "));
      break;
    case 0x01:
      Serial.print(F("Setting off-line voltage: "));
      break;
    case 0x02:
      Serial.print(F("Setting OVP: "));
      break;
    case 0x03:
      Serial.print(F("Setting on-line current: "));
      break;
    case 0x04:
      Serial.print(F("Setting off-line current: "));
      break;
    default:
      Serial.print(F("Setting unknown parameter: "));
      break;
  }
  Serial.println(error ? F("ERROR") : F("SUCCESS"));
}

int r4850_data(uint8_t *frame, struct RectifierParameters *rp)
{
  uint32_t value = __builtin_bswap32(*(uint32_t *)&frame[4]);

  switch (frame[1]) {
    case R48xx_DATA_INPUT_POWER:
      rp->input_power = value / 1024.0;
      break;

    case R48xx_DATA_INPUT_FREQ:
      rp->input_frequency = value / 1024.0;
      break;

    case R48xx_DATA_INPUT_CURRENT:
      rp->input_current = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_POWER:
      rp->output_power = value / 1024.0;
      break;

    case R48xx_DATA_EFFICIENCY:
      rp->efficiency = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_VOLTAGE:
      rp->output_voltage = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_CURRENT_MAX:
      rp->max_output_current = value / MAX_CURRENT_MULTIPLIER;
      break;

    case R48xx_DATA_INPUT_VOLTAGE:
      rp->input_voltage = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_TEMPERATURE:
      rp->output_temp = value / 1024.0;
      break;

    case R48xx_DATA_INPUT_TEMPERATURE:
      rp->input_temp = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_CURRENT1:
      // Serial.println("R48xx_DATA_OUTPUT_CURRENT1");
      //printf("Output Current(1) %.02fA\r\n", value / 1024.0);
      //rp->output_current = value / 1024.0;
      break;

    case R48xx_DATA_OUTPUT_CURRENT:
      rp->output_current = value / 1024.0;

      /* This is normally the last parameter received. Print */
      doPrint = true;
      break;

    default:
      //printf("Unknown parameter 0x%02X, 0x%04X\r\n",frame[1], value);
      break;

  }
}


void onCANReceive(int packetSize)
{
  if (!CAN.packetExtended())
    return;
  if (CAN.packetRtr())
    return;

  uint32_t msgid = CAN.packetId();
  uint8_t data[packetSize];

  CAN.readBytes(data, sizeof(data));

  switch (msgid & 0x1FFFFFFF) {

    case 0x1081407F:
      // Serial.print("Data frame: ");
      r4850_data((uint8_t *)&data, &rp);
      // Serial.println();
      break;
    case 0x1081407E:
      /* Acknowledgment */
      // Serial.println("Ack frame");
      break;
    case 0x1081D27F:
      // Serial.println("Description");
      // r4850_description((uint8_t *)&frame.data);
      break;
    case 0x1081807E:
      // Serial.println("Ack frame");
      r4850_ack((uint8_t *)&data);
      break;
    case 0x1001117E:
      // Serial.println("Amp hour frame");
      break;
    case 0x100011FE:
      // Serial.println("Normally  00 02 00 00 00 00 00 00 ");
      break;
    case 0x108111FE:
      // Serial.println("Normally 00 03 00 00 00 0s 00 00, s=1 when output enabled ");
      break;
    case 0x108081FE:
      // Serial.println("Normally 01 13 00 01 00 00 00 00 ");
      break;

    default:
      Serial.println(F("ERROR: Unknown Frame!"));
      break;
  }
}

int r4850_request_data()
{
  uint8_t data[8];
  data[0] = 0x00;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x00;

  return sendCAN( 0x108040FE , data, 8, true);
}

int r4850_set_voltage(float voltage, bool nonvolatile)
{
  uint16_t value = voltage * 1024;
  //printf("Voltage = 0x%04X\n",value);

  uint8_t command;

  if (nonvolatile) command = 0x01;  // Off-line mode
  else     command = 0x00;  // On-line mode

  uint8_t data[8];
  data[0] = 0x01;
  data[1] = command;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = (value & 0xFF00) >> 8;
  data[7] = value & 0xFF;

  return sendCAN( 0x108180FE , data, 8, false);
}

int r4850_set_current(float current, bool nonvolatile)
{
  uint16_t value = current * MAX_CURRENT_MULTIPLIER;
  uint8_t command;

  if (nonvolatile) command = 0x04;  // Off-line mode
  else     command = 0x03;  // On-line mode

  uint8_t data[8];
  data[0] = 0x01;
  data[1] = command;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = (value & 0xFF00) >> 8;
  data[7] = value & 0xFF;

  return sendCAN( 0x108180FE , data, 8, false);

}
int sendCAN(uint32_t msgid, uint8_t *data, uint8_t len, bool rtr)
{
  CAN.beginExtendedPacket(msgid, len, rtr);

  CAN.write(data, len);
  if (!CAN.endPacket()) {
    Serial.println(F("ERROR: Couldn't send CAN packet"));
    oled.setFont(statusFont);
    oled.clear();
    oled.set2X();
    oled.println("CAN err");
    oled.set1X();
    oled.println(millis());
    oled.setFont(menuFont);
    return -1;
  }
  return 0;
}
void timerIsr() {
  clickEncoder.service();
}
void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println(F("INFO: Booting up..."));

  pinMode(ROTARY_PIN_SW, INPUT_PULLUP);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); // used for rotary decoder

  CAN.setClockFrequency(8E6); // 8 MHz for Adafruit MCP2515 CAN Module

  if (!CAN.begin(125E3)) // 125kbit
  {
    Serial.println(F("ERROR: Starting CAN failed! Aborting."));
    while (1)
      ;
  }
  CAN.onReceive(onCANReceive);

  Serial.println(F("INFO: CAN setup done."));

  Wire.begin();
  // Wire.setClock(400000L); # might be required on some boards
  oled.begin(&Adafruit128x64, SCREEN_ADDRESS);
  //  oled.begin(&SH1106_128x64, SCREEN_ADDRESS); # use this instead on AZDelivery v3 nano board due to space constraints
  oled.setFont(menuFont);
  oled.clear();
  oled.println(F("R4850G2 Inverter"));
  oled.println("Starting..");

  delay(500);
  nav.timeOut = MENU_TIMEOUT;
  nav.idleOn();
}

void loop() {
  nav.poll();
  if (nav.sleepTask) { // menu is idle, do our business

    if ((millis() - g_Time1000) > 1000)
    {
      g_Time1000 = millis();
      Serial.println(F("req data..."));
      r4850_request_data();
    }
    if (doPrint) {
      doPrint = false;

      delay(10);
      oled.setFont(statusFont);

      oled.home();
      oled.print(F("I:"));
      oled.print(int(rp.input_voltage));
      oled.print(F("V;"));
      oled.print(int(rp.input_current));
      oled.print(F("A;"));
      oled.print(int(rp.input_power));
      oled.print(F("W"));
      oled.println(F("     "));
      oled.print(F("T:"));
      oled.print(int(rp.input_temp));
      oled.print(F("C;"));
      oled.print(int(rp.output_temp));
      oled.print(F("C;"));
      oled.print(int(rp.input_frequency));
      oled.print(F("Hz"));
      oled.println(F("     "));
      oled.set2X();
      oled.print(rp.output_voltage);
      oled.println(F("V     "));
      oled.print(rp.output_current);
      oled.println(F("A           "));
      oled.set1X();
      oled.print(F("/"));
      oled.print(rp.max_output_current);
      oled.println(F("A;"));
      oled.print(rp.output_power);
      oled.print(F("W           "));
      oled.setFont(menuFont);

      Serial.println(F("RECV:"));
      Serial.print(F("INPUT_POWER: "));
      Serial.println(rp.input_power);
      Serial.print(F("INPUT_FREQ: "));
      Serial.println(rp.input_frequency);
      Serial.print(F("INPUT_CURRENT: "));
      Serial.println(rp.input_current);
      Serial.print(F("OUTPUT_POWER: "));
      Serial.println(rp.output_power);
      Serial.print(F("EFFICIENCY: "));
      Serial.println(rp.efficiency);
      Serial.print(F("OUTPUT_VOLTAGE: "));
      Serial.println(rp.output_voltage);
      Serial.print(F("OUTPUT_CURRENT_MAX: "));
      Serial.println(rp.max_output_current);
      Serial.print(F("INPUT_VOLTAGE: "));
      Serial.println(rp.input_voltage);
      Serial.print(F("OUTPUT_TEMPERATURE: "));
      Serial.println(rp.output_temp);
      Serial.print(F("INPUT_TEMPERATURE: "));
      Serial.println(rp.input_temp);
      Serial.println("OUTPUT_CURRENT1");
      Serial.print(F("OUTPUT_CURRENT: "));
      Serial.println(rp.output_current);

    }
  }
}
