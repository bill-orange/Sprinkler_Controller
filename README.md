I use an open source sprinkler controller to control the time and duration that my lawn is watered. 
The microcontroller code in the controller attempts to adjust the watering duration based on weather.
It is seldom accurate.  Frequently, I look outside to see drizzling rain and the sprinklers running.

This project is an attempt to save water by more accurately inhibiting watering.  A relay will 
open the ground line to the sprinklers when the project deems that watering is not appropriate.
It does this by obtaining the current weather conditions and the forecast from OpenWeatherMap.  If it 
is raining or expected to start raining shortly, watering will be inhibited.  Simple so far.

If it is not raining and not forecast to start raining in the very near future, watering may still be inappropriate.
The OpenWeatherMap data is sent to OpenAI which is asked for a "Yes" or "No" on inhibiting watering.
If "Yes" watering will run.

My hope is that this setup will be more accurate than the approach taken in the existing sprinkler controller.

The project uses a colorful TFT display and a 3v relay board.  Be sure to configure your TFT_eSPI library for your display.
There are lots of instructions on how to do this on YouTube. The microcontroller is a TTGO T8 v1.7.1.
By minor modifications to pin assignments, you should be able to use your choice of hardware.  Be sure to use a 3v relay board.
ESP32 Dev Boards are not 5v tolerant.  I have included a Fritzing file but it is unliky to work for your part choices.
You will need OpenAI and OpenWeatherMap API keys.  They are easily optained. OpenAI costs 
a small amount of money to run.  Please note, this is not a libray. Put the .c and .h files in the same folder 
as the project folder containing the .ino file.

![IMG_1602](https://github.com/user-attachments/assets/e40e5f5d-cc54-4cad-be74-9623159d1abb)

There are a couple of interesting innovations in this project. All graphics are AI genereated PNG files.
They are not stored in the sketch.  They are downloaded from the GitHub page when needed.  You can have 
as many graphics as you like without using up limited microcontroller memory.  You can change them whenever
you like without recompiling.  The OpenAI prompt is also stored in the GitHub site instead of in the sketch.
This allows the prompt to be refined without recompiling.

This project could be adapted to other applications, like controlling outdoor lights or running pool heaters.
Use at your own risk.

03/18/2025 OTA version added
