 Dual Axis Solar Tracker (Arduino Project)

Neev

This is my small Arduino project where I built a **dual-axis solar tracker** that automatically follows the sun using **LDR light sensors** and **servo motors**.  
The idea is simple — 4 LDRs detect which side is brighter, and two servos tilt and pan a small solar panel or frame toward the light.

---

How It Works

- 4x **LDRs** (Light Dependent Resistors) are placed in a cross pattern (Top-Left, Top-Right, Bottom-Left, Bottom-Right).  
- Each pair compares brightness between left/right and top/bottom.
- The Arduino reads all 4 sensors (via analog inputs) and figures out which direction has more sunlight.
- Two **servo motors** (pan and tilt) move slowly toward the brightest direction using a basic proportional control (no full PID).
- Small **deadband** and smoothing are added to stop jitter when light is stable.

Basically, it keeps the panel facing the brightest spot in the sky all day.
