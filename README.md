# Huawai R4850G2 rectifier canbus control

  ## Based on:

   https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
   https://github.com/craigpeacock/Huawei_R4850G2_CAN

  ## IMPORTANT

   - adjust CAN.setClockFrequency() to match the crystal on the MCP2515 Board
   - put a termination jumper on J1, didn't work without for me (AZDelivey MCP2515 board)
   - be careful with any Serial.print statements in the onReceive handler. This will lead to lost CAN frames.

  ## Requirements 
   
   - this has been tested with an Arduino Mega 2560 and a Nano
   - libraries used:
      * Adafruit can library (forked from sandeepmistry/arduino-CAN):
        https://github.com/adafruit/arduino-CAN
      * ArduinoMenu (for Sketch `R4850G2_Menu.ino`, not required for `R4850G2_V2.ino`)
      * SSD1306Ascii (for Sketch `R4850G2_Menu.ino`, not required for `R4850G2_V2.ino`)
      * ClickEncoder (for Sketch `R4850G2_Menu.ino`, not required for `R4850G2_V2.ino`)

  ## PICTURES:

![image](https://github.com/haklein/r4850g2_arduino/assets/4569994/4e9a6961-6cf1-44dc-b249-fee5d6895d06)

![image](https://github.com/haklein/r4850g2_arduino/assets/4569994/0b62c0eb-7f6b-4b83-9882-98f3fad1fb27)
