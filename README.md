# vehicle-ecu

.ino file is an Arduino sketch. Currently using an Uno though I may upgrade to get more digital I/O pins.

.py file is written for python 3.6

Base functionality is implemented. Arduino <-> PC communication, fuel and ignition timing, decoding the CKP... its all there. Still much more work needed but with the right circuit to grab the crank signal and mosfets for the outputs, this should actually run an engine (v6 mustang).
