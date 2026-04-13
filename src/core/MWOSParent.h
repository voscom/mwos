#ifndef MWOS3_MWOSPARENT_H
#define MWOS3_MWOSPARENT_H

/***
 * Базовый родительский класс для однонаправленного списка MWOSUnit
 * Имеет методы работы с дочерними объектами
 */

class MWOSParent: public MWOSUnit {
public:

#pragma pack(push,1)
    MWOSUnit * child=NULL; // первый элемент дочернего однонаправленного списка
#pragma pack(pop)

    MWOSParent(char * unit_name, uint16_t unit_id=0) : MWOSUnit(unit_name,unit_id) {

    }

    /**
     * Возвращает размер дочерноего списка
     * Для подсчета пробегает весь список. Для больших списков лучше кешировать!
     * @return
     */
    uint16_t GetChildCount() {
        if (child==NULL) return 0;
        MWOSUnit * unit=child;
        uint16_t n=0;
        while (unit!=NULL && unit->unitType==child->unitType) {  // ищем конец списка
            n++;
            unit=unit->next;
        }
        return n;
    }

    /**
     * Добавить к дочернему списку новый объект
     * если новый объект содержит id - сортирует список по id
     * если id=0, то он добавляется как максимальный дочерний id+1
     */
    bool AddChild(MWOSUnit * newUnit) {
        if (newUnit==NULL || (newUnit->id>0 && indexOfChildById(newUnit->id)>=0)) { // обект не задан или уже есть с этим id
            return false;
        }
        if (child==NULL) {
            newUnit->next=this;
            //newUnit->id=0;
            child=newUnit;
            return true;
        }
        MWOSUnit * unit=child;
        MWOSUnit * lastUnit=NULL;
        MWOS_PARAM_UINT maxId=0;
        while (unit!=NULL && unit->unitType==newUnit->unitType) {  // ищем конец списка
            if (newUnit->id>0) {
                if (unit->id > newUnit->id) break; // нашли место в списке по id
            } else {
                if (unit->id > maxId) maxId=unit->id;
            }
            lastUnit=unit;
            unit=unit->next;
        }
        if (lastUnit==NULL) {
            newUnit->next=child;
            child=newUnit;
        } else {
            newUnit->next=lastUnit->next;
            lastUnit->next=newUnit;
        }
        if (newUnit->id==0 && newUnit->unitType!=PARAM) newUnit->id= maxId + 1; // автоматом расставим id для модулей, если они не были заданы при создании
        return true;
    }

    int16_t indexOfChildById(uint16_t unit_id) {
        if (unit_id==0) return -1;
        int16_t n=0;
        MWOSUnit * unitNow= child;
        while (unitNow!=NULL && unitNow->unitType==child->unitType) {
            if (unitNow->id==unit_id) return n;
            n++;
            unitNow=unitNow->next;
        }
        return -1;
    }

     /**
      * Возвращает какой это по счету элемент с таким именем
      * @param forUnit  Искомый элемент
      * @return     какой это по счету элемент с таким именем
      */
    uint16_t getNumForName(MWOSUnit * forUnit) {
        uint16_t res=0;
        MWOSUnit * unit=child;
        while (unit!=NULL && unit->unitType==child->unitType) {  // ищем конец списка
            if (unit->id==forUnit->id) return res;
            if (unit->IsName(forUnit->name)) res++;
            unit=unit->next;
        }
        return res;
    }

    /**
     * Ищет дочерний объект по ID
     * @return
     */
    MWOSUnit * FindChildById(uint16_t unit_id) {
        MWOSUnit * unit=child;
        while (unit!=NULL && unit->unitType==child->unitType) {  // ищем конец списка
            if (unit->id==unit_id) return unit;
            unit=unit->next;
        }
        return NULL;
    }

    /***
     * Найти дочерний элемент (только для модулей) по типу модуля
     * @param needModuleType    Тип модуля
     * @param notThisModule     Кроме указанного (указанный MWOSUnit исключается из результата поиска)
     * @return  Первый модуль такого типа (или NULL)
     */
    MWOSUnit * FindChildByModuleType(ModuleType needModuleType, MWOSUnit * notThisModule=NULL) {
        MWOSUnit * unit=child;
        while (unit!=NULL && unit->unitType==child->unitType) {  // ищем конец списка
            if ((unit->moduleType==needModuleType) && unit != notThisModule) return unit;
            unit=unit->next;
        }
        return NULL;
    }

    /**
     * Найти дочерний элемент по имени (элементы могут содержать имя внутри JSON)
     * @param childName Искомое имя (без JSON)
     * @return
     */
    MWOSUnit * FindChildByName(const char * childName) {
        MWOSUnit * unit=child;
        while (unit!=NULL && unit->unitType==child->unitType) {  // ищем конец списка
            if (unit->IsName(childName,false)) return unit;
            unit=unit->next;
        }
        return NULL;
    }


    /**
     * Расчитать CRC для имен и id всех дочерних элементов
     * @param crc   Куда и как расчитывать CRC
     * @param storageType  добавить контрольную сумму всех параметров этого хранилища
     * @return  Если есть модули или параметры с id>255
     */
    bool crcOfChilds(MW_CRC * crc, int8_t storageType) {
        bool res=false;
        MWOSUnit * unitNext=child;
        while (unitNext!=NULL && unitNext->unitType==child->unitType) {
            if (id>0xff) res= true;
            if (unitNext->unitType==UnitType::PARAM) {
                if (unitNext->storage==storageType) {
                    unitNext->crcOfName(crc); // crc имени
                    crc->add(id & 0xff); // crc id low
                    crc->add((id >> 8) & 0xff); // crc id hi
                }
            }
            if (unitNext->unitType==UnitType::MODULE) {
                if (((MWOSParent *) unitNext)->crcOfChilds(crc,storageType)) res= true; // рекурсивно для всех параметров этого хранилища каждого модуля
            }
            unitNext=unitNext->next;
        }
        return res;
    }


};


#endif //MWOS3_MWOSPARENT_H
