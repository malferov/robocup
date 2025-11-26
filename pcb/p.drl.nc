( pcb2gcode 2.5.0 )
( Software-independent Gcode )

( This file uses 1 drill bit sizes. )
( Bit sizes: [0.8mm] )

G94       (Millimeters per minute feed rate.)
G21       (Units == Millimeters.)
G91.1     (Incremental arc distance mode.)
G90       (Absolute coordinates.)
G00 S3000     (RPM spindle speed.)

G00 Z2.00000 (Retract)
T1
M5      (Spindle stop.)
G04 P1.00000
(MSG, Change tool bit to drill size 0.8mm)
M0      (Temporary machine stop.)
M3      (Spindle on clockwise.)
G0 Z2.00000
G04 P1.00000

G1 F20.00000
G0 X3.29700 Y5.94700
G1 Z-1.40000
G1 Z2.00000
G0 X3.29700 Y8.48700
G1 Z-1.40000
G1 Z2.00000
G0 X3.29700 Y11.02700
G1 Z-1.40000
G1 Z2.00000
G0 X11.47300 Y12.97300
G1 Z-1.40000
G1 Z2.00000
G0 X11.47300 Y4.47300
G1 Z-1.40000
G1 Z2.00000
G0 X24.17200 Y12.97300
G1 Z-1.40000
G1 Z2.00000
G0 X31.27800 Y6.94100
G1 Z-1.40000
G1 Z2.00000
G0 X36.36700 Y10.00400
G1 Z-1.40000
G1 Z2.00000
G0 X36.35800 Y6.94100
G1 Z-1.40000
G1 Z2.00000
G0 X31.28700 Y10.00400
G1 Z-1.40000
G1 Z2.00000
G0 X24.17200 Y4.47300
G1 Z-1.40000
G1 Z2.00000

G00 Z2.000 ( All done -- retract )

M5      (Spindle off.)
G04 P1.000000
M9      (Coolant off.)
M2      (Program end.)

