# Huawai R4850G2 rectifier canbus control

  ## Based on:

   https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
   https://github.com/craigpeacock/Huawei_R4850G2_CAN

  ## IMPORTANT

   - adjust CAN.setClockFrequency() to match the crystal on the MCP2515 Board
   - put a termination jumper on J1, didn't work without for me (AZDelivery MCP2515 board)
   - be careful with any Serial.print statements in the onReceive handler. This will lead to lost CAN frames.
   - no regular status request CAN frames are being sent while the menu is active, so the rectifier will
     fall back to offline values at some point (menu will hence timeout after 10s by default)

  ## Requirements 
   
   - this has been tested with an Arduino Mega 2560 and a Nano
   - libraries used:
      * Adafruit can library (forked from sandeepmistry/arduino-CAN):
        https://github.com/adafruit/arduino-CAN
      * ArduinoMenu (for Sketch `R4850G2_Menu.ino`, not required for `R4850G2_V2.ino`)
      * SSD1306Ascii (for Sketch `R4850G2_Menu.ino`, not required for `R4850G2_V2.ino`)
      * ClickEncoder (for Sketch `R4850G2_Menu.ino`, not required for `R4850G2_V2.ino`)

  ## Usage

Pressing the rotary button opens the menu. Voltage and current can be set with the rotary encoder. The "Apply" menu item sets the power supply to the chosen values. Using the "Persist" menu item (scroll down below "Apply") will permanently store the settings in the power supply. Those will then be the default startup values. The power supply will also fall back to those values when the CAN commuication is interrupted.
    
  ## PICTURES:

![image](https://github.com/user-attachments/assets/6b1efe15-7531-4c83-ac09-217468b4d0bf)

![image](https://github.com/user-attachments/assets/ebad8baa-e086-44e9-afd9-ef9afe57da40)

![image](https://github.com/haklein/r4850g2_arduino/assets/4569994/4e9a6961-6cf1-44dc-b249-fee5d6895d06)

![image](https://github.com/haklein/r4850g2_arduino/assets/4569994/0b62c0eb-7f6b-4b83-9882-98f3fad1fb27)

  ## PINOUT

  @Roturbo has provided a nice pinout for a nano: 

  ![316427349-3ad49603-3def-4ec2-9e40-f8289db90cfa](https://github.com/haklein/r4850g2_arduino/assets/4569994/0a200d5f-f5de-4887-b59d-5bd5942bd7a0)
