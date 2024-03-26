# Huawai R4850G2 rectifier canbus control

  ## Based on:

   https://www.beyondlogic.org/review-huawei-r4850g2-power-supply-53-5vdc-3kw/
   https://github.com/craigpeacock/Huawei_R4850G2_CAN

  ## IMPORTANT

   - adjust CAN.setClockFrequency() to match the crystal on the MCP2515 Board
   - put a termination jumper on J1, didn't work without for me (AZDelivey MCP2515 board)
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

  ## PICTURES:

![image](https://github.com/haklein/r4850g2_arduino/assets/4569994/4e9a6961-6cf1-44dc-b249-fee5d6895d06)

![image](https://github.com/haklein/r4850g2_arduino/assets/4569994/0b62c0eb-7f6b-4b83-9882-98f3fad1fb27)

  ## PINOUT

  @Roturbo has proovided a nice pinout for a nano: ![image](https://private-user-images.githubusercontent.com/91867260/316427349-3ad49603-3def-4ec2-9e40-f8289db90cfa.jpg?jwt=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJnaXRodWIuY29tIiwiYXVkIjoicmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbSIsImtleSI6ImtleTUiLCJleHAiOjE3MTE0NDExNTEsIm5iZiI6MTcxMTQ0MDg1MSwicGF0aCI6Ii85MTg2NzI2MC8zMTY0MjczNDktM2FkNDk2MDMtM2RlZi00ZWMyLTllNDAtZjgyODlkYjkwY2ZhLmpwZz9YLUFtei1BbGdvcml0aG09QVdTNC1ITUFDLVNIQTI1NiZYLUFtei1DcmVkZW50aWFsPUFLSUFWQ09EWUxTQTUzUFFLNFpBJTJGMjAyNDAzMjYlMkZ1cy1lYXN0LTElMkZzMyUyRmF3czRfcmVxdWVzdCZYLUFtei1EYXRlPTIwMjQwMzI2VDA4MTQxMVomWC1BbXotRXhwaXJlcz0zMDAmWC1BbXotU2lnbmF0dXJlPWE1NWVjNDBmMDNmYWY2ODA2YWU4ZTQ3NzI5YzQxNGJhZWY4ZDM2YTk4YTFiNjBhZWE3NDU1OWVjNTU3MzU5MGQmWC1BbXotU2lnbmVkSGVhZGVycz1ob3N0JmFjdG9yX2lkPTAma2V5X2lkPTAmcmVwb19pZD0wIn0._52__4E6ABxy4m5-wNx6zQMBjiQYpunLwO_Q8CIsdxQ)
