#Makefiletemplate from https://de.wikibooks.org/wiki/C-Programmierung_mit_AVR-GCC/_Makefile
#Using avrdude in ponyser mode
TARGET=main
MCU=atmega32
SOURCES=globals.c thermometer.c conrad_dcf.c main.c

PROGRAMMER=ponyser
#auskommentieren für automatische Wahl
PORT=-Pcom1

#Ab hier nichts verändern
OBJECTS=$(SOURCES:.c=.o)
CFLAGS +=-c -std=c99 -Os -DF_CPU=14745600 
LDFLAGS +=

all: hex 

program: hex
	avrdude -p$(MCU) $(PORT) -c$(PROGRAMMER) -Uflash:w:$(TARGET).hex:a

hex: $(TARGET).hex

#I think we don't use the eeprom
#eeprom: $(TARGET)_eeprom.hex

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -O ihex -j .data -j .text $(TARGET).elf $(TARGET).hex

$(TARGET)_eeprom.hex: $(TARGET).elf
	avr-objcopy -O ihex -j .eeprom --change-section-lma .eeprom=1 $(TARGET).elf $(TARGET)_eeprom.hex

$(TARGET).elf: $(OBJECTS)
	avr-gcc $(LDFLAGS) -mmcu=$(MCU) $(OBJECTS) -o $(TARGET).elf

.c.o:
	avr-gcc $(CFLAGS) -mmcu=$(MCU) $< -o $@

size:
	avr-size --mcu=$(MCU) -C $(TARGET).elf



clean_tmp:
	rm -rf *.o
	rm -rf *.elf

clean:
	rm -rf *.o
	rm -rf *.elf
	rm -rf *.hex