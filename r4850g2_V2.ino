/*
 * Huawai R4850G2 rectifier canbus control
 * 
 
   Based on:

   https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
   https://github.com/craigpeacock/Huawei_R4850G2_CAN

   IMPORTANT:

   - adjust CAN.setClockFrequency() to match the crystal on he MCP2515 Board
   - put a termination jumper on J1, didn't work without for me
   - be careful with any Serial.print in the onReceive handler. This will lead to lost CAN frames.
   - this has been tested with an Arduino Mega 2560 and the Adafruit can library (forked from sandeepmistry/arduino-CAN):
   https://github.com/adafruit/arduino-CAN

   Mega 2560 SPI pins:

   2 - int
   10 - CS
   50 - SO
   51 - SI
   52 - SCK
*/

#include <CAN.h>

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

struct RectifierParameters rp;

unsigned long g_Time1000 = 0;

bool doPrint = false;

int r4850_ack(uint8_t *frame)
{
  bool error = frame[0] & 0x20;
  uint32_t value = __builtin_bswap32(*(uint32_t *)&frame[4]);

  switch (frame[1]) {
    case 0x00:
      Serial.print("Setting on-line voltage: "); Serial.println(error ? "ERROR" : "SUCCESS");
      break;
    case 0x01:
      Serial.print("Setting off-line voltage: "); Serial.println(error ? "ERROR" : "SUCCESS");
      break;
    case 0x02:
      Serial.print("Setting OVP: "); Serial.println(error ? "ERROR" : "SUCCESS");
      break;
    case 0x03:
      Serial.print("Setting on-line current: "); Serial.println(error ? "ERROR" : "SUCCESS");
      break;
    case 0x04:
      Serial.print("Setting off-line current: "); Serial.println(error ? "ERROR" : "SUCCESS");
      break;
    default:
      Serial.print("Setting unknown paramter: "); Serial.println(error ? "ERROR" : "SUCCESS");
      break;
  }
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
      Serial.println("Description");
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
      Serial.println("ERROR: Unknown Frame!");
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
    Serial.println("ERROR: Couldn't send CAN packet");
    return -1;
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("INFO: Booting up...");
  CAN.setClockFrequency(8E6); // 8 MHz for Adafruit MCP2515 CAN Module

  if (!CAN.begin(125E3)) // 125kbit 
  {
    Serial.println("ERROR: Starting CAN failed! Aborting.");
    while (1)
      ;
  }
  CAN.onReceive(onCANReceive);

  Serial.println("INFO: CAN setup done.");

  delay(2000);
  r4850_set_current(28.0, false);

  delay(5000);
  r4850_request_data();

  delay(5000);
  r4850_set_voltage(55.2, false);
  delay(5000);

}

void loop() {
  if ((millis() - g_Time1000) > 1000)
  {
    g_Time1000 = millis();
    r4850_request_data();
  }
  if (doPrint) {
    doPrint = false;

    delay(10);
    Serial.println("DATA RECEIVED");
    Serial.print("R48xx_DATA_INPUT_POWER: ");
    Serial.println(rp.input_power);
    Serial.print("R48xx_DATA_INPUT_FREQ: ");
    Serial.println(rp.input_frequency);
    Serial.print("R48xx_DATA_INPUT_CURRENT: ");
    Serial.println(rp.input_current);
    Serial.print("R48xx_DATA_OUTPUT_POWER: ");
    Serial.println(rp.output_power);
    Serial.print("R48xx_DATA_EFFICIENCY: ");
    Serial.println(rp.efficiency);
    Serial.print("R48xx_DATA_OUTPUT_VOLTAGE: ");
    Serial.println(rp.output_voltage);
    Serial.print("R48xx_DATA_OUTPUT_CURRENT_MAX: ");
    Serial.println(rp.max_output_current);
    Serial.print("R48xx_DATA_INPUT_VOLTAGE: ");
    Serial.println(rp.input_voltage);
    Serial.print("R48xx_DATA_OUTPUT_TEMPERATURE: ");
    Serial.println(rp.output_temp);
    Serial.print("R48xx_DATA_INPUT_TEMPERATURE: ");
    Serial.println(rp.input_temp);
    // Serial.println("R48xx_DATA_OUTPUT_CURRENT1");
    Serial.print("R48xx_DATA_OUTPUT_CURRENT: ");
    Serial.println(rp.output_current);

  }
}
