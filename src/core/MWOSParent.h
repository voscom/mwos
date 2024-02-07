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
        int n=0;
        while (unit!=NULL && unit->unitType==newUnit->unitType) {  // ищем конец списка
            lastUnit=unit;
            n++;
            unit=unit->next;
        }
        if (lastUnit!=NULL) {
            newUnit->next=this;
            if (newUnit->id==0 && newUnit->unitType!=PARAM) newUnit->id=n; // автоматом расставим id, если они не были заданы при создании
            lastUnit->next=newUnit;
            return true;
        }
        return false;
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
     * Найти дочерний элемент по типу модуля
     * @param needModuleType
     * @return
     */
    MWOSUnit * FindChildByModuleType(ModuleType needModuleType) {
        MWOSUnit * unit=child;
        while (unit!=NULL && unit->unitType==child->unitType) {  // ищем конец списка
            if (unit->moduleType==needModuleType) return unit;
            unit=unit->next;
        }
        return NULL;
    }


};


#endif //MWOS3_MWOSPARENT_H
