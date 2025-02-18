I use an open source sprinkler controller to control the time and duration that my lawn is watered.  
The microcontroller code in the controller attempts to adjust the watering duration based on weather.
It is seldom accurate.  Frequently, I look ouside to see drizzling rain and the sprinklers running.

This project is an attempt to save water by more accuratly inhibiting watering.  A relay will 
open the ground line to the sprinklers when the project deems that watering is not appropriate.
It does this by obtaining the current wather and the forecast weather fromm OpenWeatherMap.  If it 
is raining or expected to start raining shortly, watering will be inhibited.  Simple so far.

If it is not raining and not forecast to start raining in the very near future, water may still be inappropriate.
The OpenWeatherMap data is sent to OpenAI which is asked for a "Yes" or "No" on inhibiting watering.
If "Yes" watering will be inhibited.

My hope is that this setup will be more accurate that the approach taken in the existing sprinkler controller.

The project uses a colorful TFT display and a 3v relay board.  Be sure to configure your TFT_eSPI library for your display.
There are lots of instructions on how to do this on YouTube. The microcontroller is a TTGO T8 v1.7.1.
By minor modifications to pin assignments you should be able to use your choice of hardware.  Be sure to use a 3v relay board.
ESP32 Dev Boards are not 5v tolerant.  I have included a Fritzing file but it is unlikly to be usable for anyone
elses implementation of the project. You will need OpenAI and OpenWeatherMap API keys.  They are easily optained. OpenAI costs 
a small amount of money to run.

![IMG_1593](https://github.com/user-attachments/assets/47233484-1f50-40bc-a024-e01cabff9d25)

There are a couple of interesting innovation in this project. All graphics are AI genereated PNG files.
They are not stored in the sketch.  They are downloaded from the GitHub page when needed.  You can have 
as many graphics as you like without using up limited microcontroller memory.  You can change them whenever
you like without recompiling.  The OpenAI prompt is also stored in the GitHub site instead of in the sketch.
This allows the prompt to be refined withour recompiling.
