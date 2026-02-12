#include "mbed.h"
#include <chrono>

using namespace std::chrono;

// --- Hardware Configuration ---
DigitalOut trigger(D2); 
DigitalIn  echo(D3); 

// KL25Z Built-in LEDs (Active Low: 0=ON, 1=OFF)
DigitalOut status_ledR(LED_RED); 
DigitalOut status_ledB(LED_BLUE); 
DigitalOut status_ledG(LED_GREEN); 

// Function to measure distance
float get_distance() {
    Timer t;
    t.reset();
    
    // Trigger pulse
    trigger = 1;
    wait_us(10);
    trigger = 0;
    
    // Wait for echo high (Start of pulse)
    uint32_t timeout = 0;
    while (echo == 0) {
        timeout++;
        if(timeout > 1000000) return -1.0f; 
    }
    
    t.start();
    
    // Wait for echo low (End of pulse)
    while (echo == 1);
    
    t.stop();
    
    long long time_us = duration_cast<microseconds>(t.elapsed_time()).count();
    
    // --- CHANGE 1: CALCULATION LIMIT (2 Meters) ---
    // 200cm corresponds to approx 11765 microseconds
    // (200 * 2) / 0.0343 = ~11661, we use 11765 for a small buffer
    if (time_us > 11765 || time_us <= 0) return -1.0f;
    
    return (float)time_us * 0.0343f / 2.0f;
}

int main() {
    printf("KL25Z Obstacle Detector (Max Print: 1.5m)\r\n");
    
    // Ensure Green is always OFF
    status_ledG = 1; 

    while (true) {
        float dist = get_distance();
        
        // Manual float splitting
        int whole_part = (int)dist;
        int decimal_part = (int)((dist - whole_part) * 100);

        // --- CHANGE 2: PRINT LIMIT (1.5 Meters) ---
        // We only proceed if dist is valid AND less than or equal to 150cm
        bool valid_range = (dist != -1.0f && dist <= 150.0f);

        if (valid_range) {
            
            // --- LED LOGIC (Threshold: 80cm) ---
            if (dist <= 80.0f) {
                // OBSTACLE DETECTED: Blue ON, Red OFF
                status_ledB = 0; 
                status_ledR = 1;
                printf("Distance: %d.%02d cm | Status: Obstacle Detected!\r\n", whole_part, decimal_part);
            } else {
                // NO OBSTACLE (80cm < dist <= 150cm): Blue OFF, Red ON
                status_ledB = 1; 
                status_ledR = 0;
                printf("Distance: %d.%02d cm | Status: No Obstacle\r\n", whole_part, decimal_part);
            }
            
        } else {
            // OUT OF RANGE (> 1.5m or Sensor Error)
            // If it's > 1.5m, we treat it as "No Obstacle" for the LEDs (Red ON)
            status_ledB = 1; 
            status_ledR = 0;
            
            // We print "Out of Range" instead of the distance number
            printf("Distance: Out of Range (>150cm)\r\n");
        }

        ThisThread::sleep_for(100ms);
    }
}