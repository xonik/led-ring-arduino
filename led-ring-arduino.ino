/*
 * Multiple MAX7221 can be chained. !CS must be kept low while clocking all bytes through, LEDs are set when !CS goes high.
 * 
 * I assume we can send data to a device down the line without affecting the earlier ones if we just send 0-bytes to push the
 * "real" one.
 * 
 * Clock does not run when !CS is high, hopefully this makes it possible to have multiple chains on the same SPI bus.
 * 
 * HW connections:
 * Cols on the LED dial equals digit on the MAX7221
 * Rows on the LED dial equals segments on the MAX7221. DP is col0, G is col7. 
 * NB: bit7 = DP and bit0 = G when sending data.
 */

#include <LedControl.h>
int DIN = 12;
int CS =  11;
int CLK = 10;

  
LedControl lc=LedControl(DIN,CLK,CS,1);

int wait = 25;
int repeats = 0;

void setup(){
  Serial.begin(9600);
  lc.shutdown(0,false);       //The MAX72XX is in power-saving mode on startup
  lc.setIntensity(0,15);      // Set the brightness to maximum value
  lc.clearDisplay(0);         // and clear the display*/
}

/*
 * Leds are wired slightly weird. They must be adressed in this order (led num from left to right on ring):
 * 0-3:   row0 -> col0-3
 * 4-7:   row1 -> col0-3 
 * 8-11:  row2 -> col0-3
 * 12-15: row3 -> col0-3
 * 16-19: row3 -> col4-7
 * 20-23: row2 -> col4-7
 * 24-27: row1 -> col4-7
 * 28-31: row0 -> col4-7
 * 
 * But cols are addressed in reverse order, col0 is bit7, col7 is bit0, so bits must be sent as: 
 * 0-3:   row0 -> bit7-4
 * 4-7:   row1 -> bit7-4 
 * 8-11:  row2 -> bit7-4
 * 12-15: row3 -> bit7-4
 * 16-19: row3 -> bit3-0
 * 20-23: row2 -> bit3-0
 * 24-27: row1 -> bit3-0
 * 28-31: row0 -> bit3-0
 * 
 * By reversing the input bit string we don't have to do any reversal internally:
 *   0-3 = bs31-28: row0 -> bit7-4
 *   4-7 = bs27-24: row1 -> bit7-4 
 *  8-11 = bs23-20: row2 -> bit7-4
 * 12-15 = bs19-16: row3 -> bit7-4
 * 16-19 = bs15-12: row3 -> bit3-0
 * 20-23 =  bs11-8: row2 -> bit3-0
 * 24-27 =   bs7-4: row1 -> bit3-0
 * 28-31 =   bs3-0: row0 -> bit3-0
 * 
 * By splitting bs into four bytes b3,b2,b1,b0 we get:
 *   0-3 = b3 hi: row0 -> bit7-4
 *   4-7 = b3 lo: row1 -> bit7-4 
 *  8-11 = b2 hi: row2 -> bit7-4
 * 12-15 = b2 lo: row3 -> bit7-4
 * 16-19 = b1 hi: row3 -> bit3-0
 * 20-23 = b1 lo: row2 -> bit3-0
 * 24-27 = b0 hi: row1 -> bit3-0
 * 28-31 = b0 lo: row0 -> bit3-0
 * 
 * Finally, ordered by row:
 *   0-3 = b3 hi: row0 -> bit7-4
 * 28-31 = b0 lo: row0 -> bit3-0
 *   4-7 = b3 lo: row1 -> bit7-4 
 * 24-27 = b0 hi: row1 -> bit3-0
 *  8-11 = b2 hi: row2 -> bit7-4
 * 20-23 = b1 lo: row2 -> bit3-0
 * 12-15 = b2 lo: row3 -> bit7-4
 * 16-19 = b1 hi: row3 -> bit3-0
 */

void setLeds(short address, long leds, bool status){
  // NB: leds are in reverse order, b31 = led1.
  byte bytes[4];
      bytes[0] = leds;
      bytes[1] = leds >> 8;
      bytes[2] = leds >> 16;
      bytes[3] = leds >> 24;
      
  byte row0 = (bytes[0] & 0b00001111 ) | (bytes[3] & 0b11110000);
  byte row1 = (bytes[0] >> 4) | ((bytes[3] << 4) & 0b11110000);
  byte row2 = (bytes[1] & 0b00001111 ) | (bytes[2] & 0b11110000);
  byte row3 = (bytes[1] >> 4) | ((bytes[2] << 4) & 0b11110000);

  lc.setRow(address, 0, row0);
  lc.setRow(address, 1, row1);
  lc.setRow(address, 2, row2);
  lc.setRow(address, 3, row3);
}

void setLedAtIndex(short address, short index, bool status){
  int row, col;
  if(index < 16){
    col = index % 4;
    row = index / 4;    
  } else {
    col = 4 + (index % 4);
    row = 3 - ((index - 16)/ 4);
  }

  lc.setLed(address, row, col, status);
}

void setWiper(short address, short index, bool status, bool allOn){
  int row, col;
  lc.clearDisplay(0);
  if(allOn){
    // Fill all led positions up to index with 1
    // NB: The MAX7221 is MSB = lowest col, so the bit string is reversed.

    // I'm using a little trick here: By letting long be a SIGNED variable,
    // and filling it with a 1 in the MSB, right shifting will extend the 1 to
    // all the places we want filled by a single shift command :)
    long leds=0x80000000; // first 1, rest 0   
    leds = leds >> index;
    
    setLeds(address, leds, status);    
  } else {
    setLedAtIndex(address, index, status);
  } 
}

void loop(){ 

  byte all[8];
  byte scroll = 0b00000001;

  for(int index=0; index < 32; index++){
    setWiper(0, index, true, true);
    delay(wait);
  }

}
