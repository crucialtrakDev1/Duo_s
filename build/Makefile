RASCC=/opt/gcc-linaro-4.9-2015.02-3-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc

LDFLAGS = -L./lib
LDLIBS    = -lwiringPi

INCLUDES = \
		   -I. \
		   -I./Api/core/inc \
		   -I./platform/inc

INC_MARIADB = -I../lib/mariadb-c/include
#LIB_MARIADB = -L../lib/mariadb-c/lib -lmariadb -lmariadbclient
#LIB_MARIADB = -L../lib/mariadb-c/lib -lmariadbclient
LIB_MARIADB = -L../lib/mariadb-c/lib -lmysqlclient
LIB_SSL = -L../lib/openssl/lib -lssl -lcrypto
LIB_TOF = -L../lib/ -lVL53L0X_Rasp

INC_LIBXML2 = -I../lib/libxml2
LIB_LIBXML2 = -L../lib/libxml2 'xml2-config --cflags --libs'


LIB_WIRINGPI = -L../lib/usrLocalLib -lwiringPi
INC_LIBEVENT = -I../lib/libevent/include
LIB_LIBEVENT = -L../lib/libevent/lib -levent

LIB_LIBEVENTx86 = -L../lib/x86/libevent-2.0.22-stable/prefix/lib -levent

INC_GPIO2 = -I/usr/src/linux-headers-4.4.0-97/include/linux
INC_GPIO2_DEFENDON = -I/usr/src/linux-headers-4.4.0-97/arch/x86/include/

INC_TOF= -I./lib/tof
INC_LCD= -I./lib/tft_lcd
INC_GPIO= -I./lib/gpio

INC_DUO_REDIS= -I./lib/hiredis
LIB_DUO_REDIS= -L./lib/hiredis -lhiredis

INC_DUO_LIBEVENT = -I./lib/event2
LIB_DUO_LIBEVENT = -L./lib/event2 -levent

LIB_DUO_LIBEVENT_STATIC = -L./ -levent




INC_REDIS= -I./include/hiredis
INC_REDISx86= -I../lib/x86/hiredis
LIB_REDIS= -L../lib -lhiredis
LIB_REDISx86= -L../lib/x86/hiredis -lhiredis
LIB_EXPAT = -L../lib -lexpat
#LIB_EXPAT = -L/usr/local/arm/expat/lib -lexpat
LIB_SQLITE3 = -L../lib -lsqlite3
LIB_ETRIREG = -L../lib -lEtriReg
LIB_ETRIDEREG = -L../lib -lEtriDereg

configRW = ./lib/configRW.c
writeLog = ./lib/writeLog.c
sqliteManager = ./lib/sqliteManager.c
TCPSocket = ./lib/TCPSocket.c
SQLite3Interface = ./lib/SQLite3Interface.c
stringTrim = ./lib/stringTrim.c
hexString = ./lib/hexString.c
networkInfo = ./lib/networkInfo.c
uart = ./lib/uart.c
rfid = ./lib/rfid.c
debugPrintf = ./lib/debugPrintf.c
RedisControl = ./lib/RedisControl.c
UpgradeFirmware = ./lib/UpgradeFirmware.c
DirectInterface = ./lib/DirectInterface.c

#parser library
libpointparser = ./lib/libpointparser.c
libDeivceInfoParser = ./lib/libDeviceInfoParser.c
serialInfoParser = ./lib/parser/serialInfoParser.c
configInfoParser = ./lib/parser/configInfoParser.c
timeSyncInfoParser = ./lib/parser/timeSyncInfoParser.c
driverInfoParser = ./lib/parser/driverInfoParser.c
directInterfaceInfoParser = ./lib/parser/directInterfaceInfoParser.c

iniParser= ./lib/parser/ini.c
libGPIO= ./lib/gpio/gpio_control.c
libLCD= ./lib/tft_lcd/tft_lcd_control.c
libTof= ./lib/tof/tofM.c
libTofSub1= ./lib/tof/vl53l0x_api.c
libTofSub2= ./lib/tof/vl53l0x_api_calibration.c
libTofSub3= ./lib/tof/vl53l0x_api_core.c
libTofSub4= ./lib/tof/vl53l0x_api_strings.c
libTofSub5= ./lib/tof/vl53l0x_platform.c

#BACS library
BACSVerify = ./lib/BACSVerify.c

TARGET0 = ./bin
REMOTE = /work/sysroot/work/smart
REMOTE1 = /work/sysroot1/work/smart
REMOTE2 = /work/sysroot2/work/smart
LIB_TARGET = ./lib


all:


DuoIo:
	gcc -o ../release/BACSDuoIo BACSDuoIo.c -lwiringPi -lwiringPiDev -lm -lpthread -lrt -lcrypt  $(iniParser) $(INC_GPIO) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC) $(libGPIO) -pthread

DuoTof:
	gcc -o ../release/BACSDuoTof BACSDuoTof.c -lwiringPi -lwiringPiDev -lm -lpthread -lrt -lcrypt $(iniParser) $(INC_TOF) $(INC_LCD) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC) $(libTof)  $(libTofSub1) $(libTofSub2) $(libTofSub3) $(libTofSub4) $(libTofSub5) -pthread

DuoLcd:
	gcc -o ../release/BACSDuoLcd BACSDuoLcd.c  $(iniParser) $(INC_TOF) $(INC_LCD) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC) $(libTof) $(libLCD) $(libTofSub1) $(libTofSub2) $(libTofSub3) $(libTofSub4) $(libTofSub5) -pthread

DuoUart:
	gcc -o ../release/BACSDuoUart BACSDuoUart.c $(iniParser) $(INC_TOF) $(INC_LCD) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC) $(libTof) $(libLCD) $(libTofSub1) $(libTofSub2) $(libTofSub3) $(libTofSub4) $(libTofSub5) -pthread

DuoOSDP:
	gcc -o ../release/BACSOSDP BACSOSDP.c $(iniParser) $(INC_TOF) $(INC_LCD) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC) $(libTof) $(libLCD) $(libTofSub1) $(libTofSub2) $(libTofSub3) $(libTofSub4) $(libTofSub5) -pthread


DuoRfid:
	gcc -o ../release/BACSDuoRfid rfid.c  $(iniParser) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC) $(libTof) $(libLCD) $(libTofSub1) $(libTofSub2) $(libTofSub3) $(libTofSub4) $(libTofSub5) -pthread


DuoWiegand:
	gcc -o ../release/BACSDuoWiegand WiegandM.c -lwiringPi -lwiringPiDev -lm -lpthread -lrt -lcrypt $(iniParser) $(INC_DUO_REDIS) $(INC_GPIO) $(libGPIO) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC)  -pthread
	

pubtest:
	gcc -o ../release/pubtest pubtest.c $(LIB_DUO_LIBEVENT_STATIC) $(LIB_DUO_REDIS) -lpthread $(INC_DUO_LIBEVENT) $(INC_DUO_REDIS) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC)
	
oled:
	gcc -o ../release/BACSOled oled.c -lwiringPi -lwiringPiDev -lm -lpthread -lrt -lcrypt $(iniParser) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC) -pthread
		
DuoDoor:
	gcc -o ../release/BACSDoor BACSDoor.c $(iniParser) $(INC_DUO_REDIS) $(LIB_DUO_REDIS) $(INC_DUO_LIBEVENT) $(LIB_DUO_LIBEVENT_STATIC)  -pthread

clean:
	rm -rf BACSOled *.o *.*~ -f
