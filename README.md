# Vehicle-ECU
testing.ino - Arduino sketch. Written for the Uno though this may change.

main.py file - Python 3.6 file. 

Upload the sketch to any atmega328. Read the top of the sketch for what pins do what.

# Features
Arduino <-> PC serial communication

Can decode most toothed wheel crankshaft signals (at least those that I have seen). Signal must already be conditioned (0-5v square wave).

Fuel and ignition advance maps as well as on-the-fly adjustment.

# Note
This was written with the intention of running it on a 99 Mustang V6. It is capable of running any v6 engine using a wasted spark ignition system, and can be adapted for other types or cylinder counts. Inline or V-Engine should not matter.
