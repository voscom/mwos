/*
 * Author: vladimir9@bk.ru
 *
 * Кольцевой последовательный буффер (стим) в ОЗУ
 * Имеет отдельные указатели для чтения/записи
 * Может содержать несколько указателей чтения (все кроме первого доступны только для функци readTo)
 * При переполнении записи - удаляет старые данные, но можно переопределить функцию clear
 *
 */

#ifndef MWStreamRAM_H_
#define MWStreamRAM_H_

#include <Arduino.h>
#include <Stream.h>
#include "LibCRC.h"

class MWStreamRAM : public Stream {
public:

    uint16_t offsetWrite;
    uint16_t total; // общий размер буффера
    uint8_t * bufferRAM;
    uint16_t * offsetRead;
    uint8_t offsetsCount;

    /**
     * Создать кольцевой последовательный буффер
     * в беффере всегда свободен хотя-бы 1 байт
     * Желательно перехватать метод clear, что-бы очищать место сразу на размер записи
     * @param bufferSize	Размер буффера
     * @param offsetsReadCount	Количество указателей чтения
     */
	MWStreamRAM(uint16_t bufferSize, uint8_t offsetsReadCount=1) : Stream() {
		total=bufferSize;
		offsetsCount=offsetsReadCount;
		bufferRAM=(uint8_t *)malloc(total); // new uint8_t(total);
		offsetRead=new uint16_t(offsetsCount);
		offsetWrite=0;
		for (uint8_t i=0; i<offsetsCount; i++) offsetRead[i]=0;
	};
	virtual ~MWStreamRAM() {
		free(bufferRAM);
		delete[] offsetRead;
	};

    void show(Print * toPrint,uint8_t offsetNum) {
    	uint16_t len=available(offsetNum);
    	toPrint->print("Buff (");
    	toPrint->print(len);
    	toPrint->print(") = [");
    	for (uint16_t i=0; i<total; i++) {
    		int ch=bufferRAM[i];
    		toPrint->print(',');
    		toPrint->print(ch,HEX);
    	}
    	toPrint->print("] = {");
    	for (uint16_t i=0; i<len; i++) {
    		int ch=getByte(i,offsetNum);
    		toPrint->print(',');
    		toPrint->print(ch,HEX);
    	}
    	toPrint->println('}');
    };

    /**
     * Вернуть байт из буффера без изменения указателей
     * @param offset	Смещениее от указателя чтения
     * @param offsetNum	Номер указателя чтения
     */
    int getByte(uint16_t offset,uint8_t offsetNum) {
    	if (offset>available(offsetNum)) return -1;
    	uint16_t offsetNew=offsetRead[offsetNum]+offset;
    	if (offsetNew>=total) offsetNew-=total;
    	return bufferRAM[offsetNew];
    };

    int read(uint8_t offsetNum) {
    	if (available(offsetNum)<1) return -1;
    	int res=bufferRAM[offsetRead[offsetNum]++];
    	if (offsetRead[offsetNum]>=total) offsetRead[offsetNum]=0;
    	return res;
    };
    virtual int read() {
    	return read(0);
    };
    size_t readBytes(uint8_t offsetNum, char *buffer, size_t length) {
    	if (length>available(offsetNum)) length=available(offsetNum);
    	if (length<1) return 0;
    	if ((offsetRead[offsetNum]<offsetWrite) || (offsetRead[offsetNum]+length<total)) {
    		memcpy(buffer,&bufferRAM[offsetRead[offsetNum]],length);
    	} else {
    		uint16_t firstSize=total-offsetRead[offsetNum];
    		memcpy(buffer,&bufferRAM[offsetRead[offsetNum]],firstSize);
    		int16_t lastSize=length-firstSize;
    		if (lastSize>0) memcpy(&buffer[firstSize],bufferRAM,lastSize);
    	}
    	clear(offsetNum,length);
    	return length;
    };
    virtual size_t readBytes(char *buffer, size_t length) {
    	return readBytes(0,buffer,length);
    }


    virtual size_t write(uint8_t ch) {
    	bufferRAM[offsetWrite++]=ch;
    	if (offsetWrite>=total) offsetWrite=0;
    	for (uint8_t i=0; i<offsetsCount; i++) { // очистим место для всех указателей чтения
    		if (offsetWrite==offsetRead[i]) clear(i,1);
    	}
    	return 1;
    };
    virtual size_t write(const uint8_t *buffer, size_t size) {
    	if (size<1) return 0;
    	for (uint8_t i=0; i<offsetsCount; i++) { // очистим место для всех указателей чтения
    		uint16_t freeSize=total-available(i);
        	if (size>=freeSize) { // не хватает места
        		clear(i,size-freeSize);
        	}
    	}
    	if (offsetWrite+size<=total) {
    		memcpy(&bufferRAM[offsetWrite],buffer,size);
    	} else {
    		uint16_t firstSize=total-offsetWrite;
    		memcpy(&bufferRAM[offsetWrite],buffer,firstSize);
    		int16_t lastSize=size-firstSize;
    		if (lastSize>0) memcpy(bufferRAM,&buffer[firstSize],lastSize);
    	}
    	offsetWrite+=size;
		if (offsetWrite>=total) offsetWrite-=total;
		return size;
    };

    virtual void flush() {

    };
    virtual int peek() { return 0; };

    uint16_t available(uint8_t offsetNum) {
    	if (offsetWrite==offsetRead[offsetNum]) return 0;
		if (offsetWrite>offsetRead[offsetNum]) return offsetWrite-offsetRead[offsetNum];
		return total+offsetWrite-offsetRead[offsetNum];
    }
    virtual int available() {
    	return available(0);
    };
    int availableWrite(uint8_t offsetNum) {
    	return total-available(offsetNum)-1;
    };

    virtual int availableForWrite() { return availableWrite(0); }

    /**
     * Очистить необходимое место в кольцевом буфере
     * можно переопределить, чтобы очищать по записям (обычно оба указателя надо очищать одинаково)
     * в потомках можно предусмотреть сохранение очищаемых данных
     * @param	offsetNum	Номер указателя чтения, который стоит очистить
     * @param	size	Сколько очистить минимум
     * @return	Сколько реально очищено
     */
    virtual uint16_t clear(uint8_t offsetNum, int16_t size) {
    	offsetRead[offsetNum]+=size;
    	if (offsetRead[offsetNum]>=total) offsetRead[offsetNum]-=total;
    	return size;
    }

    /**
     * Прочитать и записать в другой стрим (смещает указатель чтения)
     * Добавляет контрольную сумму считанного блока
     * @param	stream		Куда записывать прочитанное
     * @param	offsetNum	Номер указателя чтения
     * @param	crc			Куда добавлять контрольную сумму (перед этим надо сделать crc.begin())
     * @param	length		Длина блока для чтения (автоматически уменьшится до доступного)
     */
    uint16_t readTo(Stream * stream, uint8_t offsetNum=0, MW_CRC16 * crc=NULL, uint16_t length=0xffff, uint16_t streamAvaliableSize=8192) {
    	if (length>available()) length=available();
    	if (length<1) return 0;
    	if ((offsetRead[offsetNum]<offsetWrite) || (offsetRead[offsetNum]+length<total)) {
    		stream->write(&bufferRAM[offsetRead[offsetNum]],length);
    		if (crc!=NULL) crc->addBuffer(&bufferRAM[offsetRead[offsetNum]],length);
    	} else {
    		uint16_t firstSize=total-offsetRead[offsetNum];
    		stream->write(&bufferRAM[offsetRead[offsetNum]],firstSize);
    		if (crc!=NULL) crc->addBuffer(&bufferRAM[offsetRead[offsetNum]],firstSize);
    		int16_t lastSize=length-firstSize;
    		if (lastSize>0) {
    			stream->write(bufferRAM,lastSize);
    			if (crc!=NULL) crc->addBuffer(bufferRAM,lastSize);
    		}
    	}
    	offsetRead[offsetNum]+=length;
    	if (offsetRead[offsetNum]>=total) offsetRead[offsetNum]-=total;
    	return length;
    }

    size_t writeInt16(uint16_t dat) {
    	size_t size=0;
    	uint8_t * datBuff=(uint8_t *) &dat;
    	size+=write((uint8_t) datBuff[0]);
    	size+=write((uint8_t) datBuff[1]);
    	return size;
    }

    size_t writeInt32(uint32_t dat) {
    	size_t size=0;
    	uint8_t * datBuff=(uint8_t *) &dat;
    	size+=write((uint8_t) datBuff[0]);
    	size+=write((uint8_t) datBuff[1]);
    	size+=write((uint8_t) datBuff[2]);
    	size+=write((uint8_t) datBuff[3]);
    	return size;
    }

};

#endif /* MWOSLog_H_ */
