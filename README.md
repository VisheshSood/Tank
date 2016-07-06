# Lab 5: Using Bluetooth to Allow Tank Control

This is the folder directory for our Lab 5 Files. 


*bluetooth_tank.c* is our only file that we run on the board. It initializes the bluetooth module, all the used GPIO pins, and also has the AI code for the motors that use sensors to control the tank. It also has the interupt handlers for the bluetooth signals and if the user controls the tank, the code takes care of all directions and settings.

*Android Application Code* is the directory for our Android app (designed and developed by Vishesh). Unlike other groups, we made a custom app with controls as extra credit to control our tank.

*police.ino* is our speed bump arduino code that uses photocell sensor readings to play a police siren. This was our other extra credit feature, using the sensor to determine the speed and pretend a police is after the car.

*Report.pdf* contains our wiring structure, in which we show which GPIO pins are used for each motor output and how we set up the bluetooth.


Vishesh Sood