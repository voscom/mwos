/*
 *      Author: vladimir9@bk.ru
 *
 *      Для отображения отадки необходимо добавить в первой строчке скетча:
 *      #define DEBUG_ON
 *      или просто выбрать режим отладки в опциях компиляции в ArduinoIDE
 *
 *      если необходимо задать другой порт отлидки, то его легко переопределить, добавив после #define DEBUG_ON
 *		#define DEBUG_PORT Serial1
 *		где вместо Serial1 можно указать любой последовательный порт
 *
 *		ВНИМАНИЕ!!! порт отладки необходимо инициализировать в самом начале программы (до начала отладки)
 *
 *
 */
#ifndef MWOS3_DEBUG_H_
#define MWOS3_DEBUG_H_

//#if defined(ESP32) && defined(ARDUHAL_LOG_LEVEL) && (ARDUHAL_LOG_LEVEL>0)
//#endif

#ifdef MWOS_DEBUG

#include <Arduino.h>
#include <Print.h>
#include <IPAddress.h>
#if MWOS_FILE_SYSTEM>0
#include <FS.h>
#endif
#include "MWOSModuleBase.h"
#include "adlib/MWTimeout.h"

class MWOSDebug : public Print {
  public:
	bool inited=false;
#if MWOS_FILE_SYSTEM>0
	File * file=NULL;
#endif

    virtual size_t write(uint8_t byte) {
#if MWOS_FILE_SYSTEM>0
    	if (file!=NULL) file->write(byte);
#endif
    	return MWOS_DEBUG .write(byte);
    }
    virtual int peek() {
    	return 0;
    }
    virtual int read() {
    	return MWOS_DEBUG .read();
    }
    virtual int available() {
    	return MWOS_DEBUG .available();
    }
    virtual void flush() {
#if MWOS_FILE_SYSTEM>0
    	if (file!=NULL) file->flush();
#endif
        MWOS_DEBUG .flush();
    }

    virtual int begin(unsigned long baud) {
    	if (!inited) {
            MWOS_DEBUG .begin(baud);
    		inited=true;
    	}
    	initFPS();
    	initMem();
    	return 0;
    }

    uint16_t printStrPROGMEM(char * StrPROGMEM) {
        if (StrPROGMEM==NULL) return 0;
        uint16_t size=strlen_P(StrPROGMEM);
        for (size_t i = 0; i < size; ++i) {
            char ch = pgm_read_byte_near(StrPROGMEM + i);
            print(ch);
        }
        return size;
    }

    uint16_t printModule(MWOSUnit * module,uint16_t index=UINT16_MAX) {
    	uint16_t size=0;
    	uint32_t tm=millis();
    	size+=print(tm/60000);
    	size+=print(':');
    	size+=print((tm % 60000)/1000);
    	size+=print('.');
    	size+=print(tm % 1000);
    	size+=print('>');
        size+=module->printName(this);
        size+=print('.');
        size+=print(module->id);
        if (index!=UINT16_MAX) {
            size+=print(':');
            size+=print(index);
        }
        size+=print('>');
    	//size+=printMem();
        size+=print(' ');
    	return size;
	}

	/**
	 * Необходимо вызывать в местах, где необходим контроль расхода памяти
	 */
	uint16_t updateMem() {
		uint16_t mem=getFreeMemory();
		if (_memMin>mem) _memMin=mem;
		if (_memMax<mem) _memMax=mem;
		return mem;
	}
	void initMem() {
		_memMin=0x7fffffff;
		_memMax=0;
	}
	uint16_t printMem(bool withMinMax=true) {
    	uint16_t size=0;
    	size+=print(F(" Free mem: "));
    	size+=print(updateMem());
    	if (withMinMax) {
        	size+=print('(');
        	size+=print(_memMin);
        	size+=print('-');
        	size+=print(_memMax);
        	size+=print(')');
    	}
    	size+=println();
    	initMem();
    	return size;
	}

	uint16_t printByte(unsigned char bt) {
		uint16_t size=0;
		if (bt<16) { size+=print('0'); }
		size+=print(bt,HEX);
		return size;
	}

	uint16_t printBytes(unsigned char * bts, uint16_t len) {
		uint16_t size=0;
		size+=print('#');
		for (uint16_t i=0; i<len; i++) {
			size+=print(' ');
			size+=printByte(bts[i]);
			if (i>15) {
				size+=print('~');
				break;
			}
		}
		return size;
	}

	/**
	 * Необходимо вызывать 1 раз за глобальный цикл
	 */
	uint16_t updateFPS() {
		_debugFPS++;
		uint16_t _fpsTime=millis()-_lastTimeFPS;
		if (_fpsTimeMin>_fpsTime) _fpsTimeMin=_fpsTime;
		if (_fpsTimeMax<_fpsTime) _fpsTimeMax=_fpsTime;
		_lastTimeFPS=millis();
		return _fpsTime;
	}

	void initFPS() {
    	_debugFPS=0;
    	_fpsTimeMin=0xffff;
    	_fpsTimeMax=0;
    	_startTimeFPS=millis();
    	_lastTimeFPS=_startTimeFPS;
	}
	uint16_t printFPS(bool withMinMax=true) {
    	uint16_t size=0;
    	size+=print(F(" FPS: "));
    	uint32_t _deltaTimeFPS=millis()-_startTimeFPS;
    	if (_debugFPS>0) _debugFPS=_deltaTimeFPS/_debugFPS;
    	size+=print(_debugFPS);
    	if (withMinMax) {
        	size+=print('(');
        	size+=print(_fpsTimeMin);
        	size+=print('-');
        	size+=print(_fpsTimeMax);
        	size+=print(')');
    	}
    	size+=println();
    	initFPS();
    	return size;
	}

	uint16_t print_address(IPAddress ip, uint16_t port) { // вывести адрес в лог
        uint16_t size=0;
		size+=print(ip[0]);
		size+=print('.');
		size+=print(ip[1]);
		size+=print('.');
		size+=print(ip[2]);
		size+=print('.');
		size+=print(ip[3]);
        size+=print(':');
        size+=println(port);
        return size;
	}

	uint16_t print_address(const char *str, uint16_t port) { // вывести адрес в лог
		uint16_t size=Print::write((const char *)str);
		size+=print(':');
		size+=println(port);
		return size;
	}


private:

	uint32_t _startTimeFPS=0;
	uint32_t _lastTimeFPS=0;

	uint16_t _fpsTimeMin=0xffff;
	uint16_t _fpsTimeMax=0;

	uint16_t _debugFPS=0;

	uint32_t _memMin=0xffff;
	uint32_t _memMax=0;

};
#ifndef MWOS_NO_GLOBAL_INSTANCES
MWOSDebug debug_mw;
#endif

#define MW_DEBUG_PORT debug_mw
#endif

#ifdef MW_DEBUG_PORT
#define MW_LOG(...) MW_DEBUG_PORT.print( __VA_ARGS__ )
#define MW_LOG_LN(...) MW_DEBUG_PORT.println( __VA_ARGS__ )
#define MW_LOG_MODULE(...) MW_DEBUG_PORT.printModule( __VA_ARGS__ )
#define MW_LOG_BYTE(...) MW_DEBUG_PORT.printByte( __VA_ARGS__ )
#define MW_LOG_BYTES(...) MW_DEBUG_PORT.printBytes( __VA_ARGS__ )
#define MW_LOG_ADDR(...) MW_DEBUG_PORT.print_address( __VA_ARGS__ )
#define MW_LOG_PROGMEM(...) MW_DEBUG_PORT.printStrPROGMEM( __VA_ARGS__ )

#define MW_DEBUG_LOG_FPS(...) MW_DEBUG_PORT.printFPS(__VA_ARGS__)
#define MW_DEBUG_LOG_MEM(...) MW_DEBUG_PORT.printMem(__VA_ARGS__)
#define MW_DEBUG_UPDATE_FPS(...) MW_DEBUG_PORT.updateFPS()
#define MW_DEBUG_UPDATE_MEM(...) MW_DEBUG_PORT.updateMem()
#define MW_DEBUG_BEGIN(...) MW_DEBUG_PORT.begin( __VA_ARGS__ )

#else

#define MW_LOG(...)
#define MW_LOG_MODULE(...)
#define MW_LOG_LN(...)
#define MW_LOG_ADDR(...)
#define MW_LOG_PROGMEM(...)

#define MW_DEBUG_LOG_MEM(...)
#define MW_DEBUG_LOG_FPS(...)
#define MW_DEBUG_UPDATE_FPS(...)
#define MW_DEBUG_UPDATE_MEM(...)
#define MW_DEBUG_BEGIN(...)
#define MW_LOG_BYTE(...)
#define MW_LOG_BYTES(...)

#endif


#endif /* MWOSDebug_H_ */
