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
G0 X3.11700 Y6.03400
G1 Z-1.40000
G1 Z2.00000
G0 X3.11700 Y8.57400
G1 Z-1.40000
G1 Z2.00000
G0 X6.29200 Y13.55900
G1 Z-1.40000
G1 Z2.00000
G0 X3.11700 Y11.11400
G1 Z-1.40000
G1 Z2.00000
G0 X6.29200 Y3.55900
G1 Z-1.40000
G1 Z2.00000
G0 X18.92000 Y7.00000
G1 Z-1.40000
G1 Z2.00000
G0 X18.92900 Y10.06200
G1 Z-1.40000
G1 Z2.00000
G0 X18.99200 Y13.55900
G1 Z-1.40000
G1 Z2.00000
G0 X24.00900 Y10.06200
G1 Z-1.40000
G1 Z2.00000
G0 X24.00000 Y7.00000
G1 Z-1.40000
G1 Z2.00000
G0 X18.99200 Y3.55900
G1 Z-1.40000
G1 Z2.00000

G00 Z2.000 ( All done -- retract )

M5      (Spindle off.)
G04 P1.000000
M9      (Coolant off.)
M2      (Program end.)

