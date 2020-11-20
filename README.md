# Vehicle-ECU
Designed to run a v6 engine (99 Ford Mustang). Can be adapted to any engine size or arrangement.

# Files
eec-v_full.ino - Arduino sketch. Currently using the Atmega328.
eec-iv_icm.ino - Arduino Sketch. An attempt to replace the ignition control module in a 94 Ford Probe which uses eec-iv.

main.py - Monitor and Tuning software written in Python v3.6.

Upload the sketch to any atmega328. Read the top of the sketch for what pins do what.

# Features
Arduino <-> PC communication is via USB

Can decode toothed wheel crankshaft signals. Signal must already be conditioned (0-5v square wave).

Fuel and spark advance maps, as well as on-the-fly adjustment.

# Note
This was written with the intention of replacing the stock ecu in a  99 Mustang V6. Its kind of a dream project. It should be capable of running any v6 engine using a wasted spark ignition system, and can be adapted for other types or cylinder counts. Inline or V-Engine should not matter.
