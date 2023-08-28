Huawai R4850G2 rectifier canbus control

   Based on:

   https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
   https://github.com/craigpeacock/Huawei_R4850G2_CAN

   IMPORTANT:

   - adjust CAN.setClockFrequency() to match the crystal on the MCP2515 Board
   - put a termination jumper on J1, didn't work without for me
   - be careful with any Serial.print in the onReceive handler. This will lead to lost CAN frames.
   - this has been tested with an Arduino Mega 2560 and Nano
   - libraries used:
      * Adafruit can library (forked from sandeepmistry/arduino-CAN):
        https://github.com/adafruit/arduino-CAN
      * ArduinoMenu
      * SSD1306Ascii
      * ClickEncoder
