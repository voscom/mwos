#ifndef MWOS3_MWOSKeyboardBase_H
#define MWOS3_MWOSKeyboardBase_H
#include "core/MWOSModule.h"
#include "core/adlib/MWTimeout.h"

#ifndef MWOS_KEYBOARD_BUFFER
#define MWOS_KEYBOARD_BUFFER 20 // максимальное количество одновременно нажатых клавиш
#endif
/***
 * Базовый класс клавиатуры
 *
 * Обрабатывает комбинации нажатых клавиш одновременно
 *
 * В момент нажатия клавиши, она добавляется к списку нажатых.
 * На следующий фрейм клавиша удаляется из списка.
 *
 * Модуль может удалить нажатую клавишу, что-бы ее не обработали другие модули.
 * Фиксирует время нажатия
 *
 */
class MWOSKeyboardBase : public MWOSModule {
public:

    /**
    * Список кодов символов нажатых клавиш в данный момент (удаляется при отпускании клавиш)
    */
    String _keysCodePressed="";

    /**
     * Время нажатия (от момента нажатия первой кнопки комбинации)
     */
    MWTimeoutFrom timeoutPressed;

    /**
     * первая необработанная нажатая клавиша в буфере клавиатуры.
     * обработкой считается вызов функции GetPressedKey() или отправки 0 в этот параметр
     */
    MWOS_PARAM(0, keys, mwos_param_string, mwos_param_control, MWOS_STORAGE_NO, MWOS_KEYBOARD_BUFFER);

    MWOSKeyboardBase() : MWOSModule((char *) F("keyboard")) {
        moduleType=MODULE_KEYBOARD;
        AddParam(&p_keys);
    }

    /***
     * Вызывается на многие системные события и каждый тик операционной системы
     * @param modeEvent    Тип вызываемого системного события
     */
    virtual void onEvent(MWOSModeEvent modeEvent) {
        MWOSModule::onEvent(modeEvent);
        if (modeEvent==EVENT_UPDATE) { // Вызывается каждый тик операционной системы
            _keysNew=""; // удалим новые кнопки (это уже начало следующего фрейма и все должны были их увидеть)
        }
    }

    virtual int64_t getValue(MWOSParam * param, int16_t arrayIndex) {
        if (param->id==0) {
            if (_keysPressed.length()>arrayIndex) return _keysPressed[arrayIndex];
            return 0;
        }
        return MWOSModule::getValue(param, arrayIndex); // отправим значение из EEPROM
    }

    /***
     * Вызывается при получении нового значения от сервера
     * @param receiverDat     Полученные данные со структурой
     */
    virtual void onReceiveValue(MWOSNetReceiverFields * receiverDat) {
        if (receiverDat->param_id==0) pressKey(receiverDat->GetValueUInt8());
        else MWOSModule::onReceiveValue(receiverDat);  // сохраним в хранилище
    }

    /**
     * Была ли нажата любая новая клавиша в этом фрейме?
     * @return  Была нажата в этом фрейме
     */
    bool IsKeyPress() {
        return _keysNew.length()>0;
    }

    /**
     * Была ли нажата новая клавиша в этом фрейме?
     * @param k Код символа клавиши
     * @return  Была нажата в этом фрейме
     */
    bool IsKeyPress(char k) {
        return _keysNew.indexOf(k)>=0;
    }

    /**
     * Сейчас нажата клавиша?
     * @param k Код символа клавиши
     * @return Сейчас нажата
     */
    bool IsKeyPressedNow(char k) {
        return _keysCodePressed.indexOf(k)>=0;
    }

    /**
     * Удалить код клавиши в этом фрейме, что-бы другие модули его не поймали
     * @param k код клавиши
     */
    void RemoveKey(char k) {
        int pos=_keysNew.indexOf(k);
        if (pos>=0) _keysNew.remove(pos,1);
    }

    /**
     * Удалить все клавиши в этом фрейме, что-бы другие модули их не поймали
     */
    void RemoveKeys() {
        _keysNew="";
    }

    /**
     * Получить код символа первой нажатой клавиши в этом фрейме
     * @return  Код символа или 0, если нет нажатых
     */
    char GetPressedKey() {
        if (_keysNew.length()>0) return _keysNew[0];
        return 0;
    }

protected:

    uint8_t * _codes=NULL; // таблица кодов клавиш
    /**
    * Список номеров нажатых клавиш в данный момент (удаляется при отпускании клавиш)
    */
    String _keysPressed="";
    /**
    * Список новых клавиш нажатых в этом фрейме (удаляется на следующий фрейм)
    */
    String _keysNew="";

    void pressKey(uint8_t keyNum) {
        char ch=keyNum;
        if (_keysPressed.indexOf(keyNum)<0) {
            _keysPressed+=ch;
            if (_codes!=NULL) ch=pgm_read_byte_near(_codes + keyNum - 1);
            _keysCodePressed+=ch;
            _keysNew+=ch;
            if (!timeoutPressed.isStarted()) timeoutPressed.start();
        }
    }

    void unpressKey(uint8_t keyNum) {
        if (_keysPressed.isEmpty()) return;
        char ch=keyNum;
        int pos=_keysPressed.indexOf(ch);
        if (pos>=0) {
            _keysPressed.remove(pos,1);
            if (_keysPressed.isEmpty()) {
                timeoutPressed.stop();
                _keysCodePressed="";
            } else {
                if (_codes!=NULL) ch=pgm_read_byte_near(_codes + keyNum - 1);
                pos=_keysCodePressed.indexOf(ch);
                if (pos>=0) _keysCodePressed.remove(pos,1);
            }
        }
    }

};


#endif //MWOS3_MWOSKeyboardMatrix_H
