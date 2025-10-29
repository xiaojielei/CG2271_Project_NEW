for these files, the data of the DHT is connected to gpio4 (pin4), vcc to the 3.3v of the esp32 and a common ground

upload .ino to esp32 and .c to the mcxc

uart communication 
pte23 to gpio1
pte22 to gpio2

open up terminal on mcu app (free rtos config same as uart lab) and results will be printed in the console
MCXC sends command to esp32 every 2 seconds to read data and prints output in the console in MCU app. 