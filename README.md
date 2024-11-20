# Memento Firmware
This is the hardware/firmware used for this Memento project, which includes a [Seeed Studio XIAO ESP32S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) and was coded in the [Arduino IDE](https://www.arduino.cc/en/software). To reproduce this project connect the expansion board/camera and the WiFi antenna to the ESP32S3 board

## Configuration
To run this firmware on the ESP32S3 Sense device you must connect the device with your computer via a USB-C cable and have [Arduino IDE](https://www.arduino.cc/en/software) downloaded.

 - Open your Arduino IDE go to "Settings" go to "Additional boards   
   manager URLs" and paste this link https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json and click "Ok"
 - Then go to "Tools" > "Board: ..." > "Boards Manager".    Type esp32 and install "esp32 by Espressif"
 - Go back to "Tools" > "Board: XIAO_ESP32S3" > "esp32" > Select "XIAO_ESP32S3"
 - In "Tools" make sure that "Port: ..." is the location where your
   ESP32S3 is connected to on your computer
 - In the IDE click the drop down next to the "Verify", "Upload", and
   "Start Debugging" buttons in the top left corner and make sure
   that "XIAO_ESP32S3" is selected and the port displayed is correct.
 - Finally go to "Tools" > click "PSRAM: Disabled" and switch it to
   "PSRAM: OPI PSRAM".
 - Optionally, if you want to check if everything is configured correctly and ready to run, go to "File" > "Examples" and select one of the Built-in examples like "01.Basics" > "Blink"

If you still need help or want a quick understanding of the ESP32S3 Sense board and what is capable of, watch this quick [video](https://www.youtube.com/watch?v=_wvuOsRgmt4).

## Run Code
Once you clone the repository, go to your "credentials.h" file and input your WiFi SSID and Password so that the device can connect to our server when it runs. Finally, you can open the "Memento.ino" file from this repository and click "Verify" to check for any errors and then "Upload" to run the code on the ESP32S3 device. Once the code has uploaded you can see the status and output in "Serial Monitor". Now, whenever the device is powered on via USB-C, the code will automatically run, and preform its instructions. 
