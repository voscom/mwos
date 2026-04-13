#ifndef MWOS3_MWOSLINKTOVALUE_H
#define MWOS3_MWOSLINKTOVALUE_H

#include "MWOSParam.h"

#ifndef MWOS_SENSOR_ANALOG_PARAM_ID
#define MWOS_SENSOR_ANALOG_PARAM_ID 0
#endif

/**
 * добавляет в класс модуля параметры и методы линковки на значения другого параметра в другом модуле
 * можно добавить линки на несколько значений (_p_links_count)
 * добавляет модулю 3 параметра (id задаются)
 * linkToModuleId
 * linkToModuleParamId
 * linkToValueIndex
 *
 */
#define MWOS_PARAMS_LINK_TO_MODULE_PARAM_VALUE(_p_moduleId, _p_paramId, _p_value_index, _p_links_count) \
MWOSModuleBase * _linkToModule[_p_links_count]; \
MWOSParam * _linkToModuleParam[_p_links_count]; \
MWOS_PARAM_INDEX_UINT _linkToValueIndex[_p_links_count]; \
int64_t _linkParamValue[_p_links_count]; \
\
MWOS_PARAM(_p_moduleId, linkToModuleId, MWOS_PARAM_INT_PTYPE, mwos_param_option, MWOS_STORAGE_EEPROM, _p_links_count); \
MWOS_PARAM(_p_paramId, linkToModuleParamId, MWOS_PARAM_INT_PTYPE, mwos_param_option, MWOS_STORAGE_EEPROM, _p_links_count); \
MWOS_PARAM(_p_value_index, linkToValueIndex, MWOS_PARAM_INDEX_UINT_PTYPE, mwos_param_option, MWOS_STORAGE_EEPROM, _p_links_count); \
\
void AddParamsLinkToValue() { \
    MWOSModuleBase::AddParam(&p_linkToModuleId); \
    MWOSModuleBase::AddParam(&p_linkToModuleParamId); \
    MWOSModuleBase::AddParam(&p_linkToValueIndex); \
} \
\
void LoadParamsAllLinkToValue() { \
    for (uint16_t i = 0; i < _p_links_count; ++i) LoadParamsLinkToValue(i); \
} \
\
void LoadParamsLinkToValue(uint16_t linksIndex) { \
    if (linksIndex>=_p_links_count) { \
        MW_LOG_MODULE(this); MW_LOG(F("error linksIndex: ")); MW_LOG_LN(linksIndex); \
        return; \
    } \
    _linkToValueIndex[linksIndex]=MWOSModule::loadValue(_linkToValueIndex[linksIndex], &p_linkToValueIndex, linksIndex); \
    int32_t _module_id=-1; \
    if (_linkToModule[linksIndex] != NULL) _module_id=_linkToModule[linksIndex]->id; \
    _module_id=MWOSModule::loadValue(_module_id, &p_linkToModuleId, linksIndex); \
    if (_module_id < 0) { \
        MW_LOG_MODULE(this); MW_LOG(F("error link module id: ")); MW_LOG_LN(_module_id); \
        _linkToModule[linksIndex]=NULL; \
        return; \
    } \
    _linkToModule[linksIndex]=mwos.getModule(_module_id); \
    if (_linkToModule[linksIndex] == NULL) { \
        MW_LOG_MODULE(this); MW_LOG(F("error link module not found: ")); MW_LOG_LN(_module_id); \
        return; \
    } \
    int32_t _module_param_id=MWOS_SENSOR_ANALOG_PARAM_ID; \
    if (_linkToModuleParam[linksIndex] != NULL) _module_param_id=_linkToModuleParam[linksIndex]->id; \
    _module_param_id=MWOSModule::loadValue(_module_param_id, &p_linkToModuleParamId, linksIndex); \
    if (_module_param_id < 0) { \
        MW_LOG_MODULE(this); MW_LOG(F("error link module param id: ")); MW_LOG_PROGMEM(_linkToModule[linksIndex]->name); MW_LOG(':'); MW_LOG_LN(_module_param_id); \
        _linkToModuleParam[linksIndex]=NULL; \
        return; \
    } \
    _linkToModuleParam[linksIndex]=_linkToModule[linksIndex]->getParam(_module_param_id); \
    if (_linkToModuleParam[linksIndex] == NULL) { \
        MW_LOG_MODULE(this); MW_LOG(F("error link module param: ")); MW_LOG_PROGMEM(_linkToModule[linksIndex]->name); MW_LOG(':'); MW_LOG_LN(_module_param_id); \
        return; \
    } \
    if (_linkToValueIndex[linksIndex] >= _linkToModuleParam[linksIndex]->arrayCount()) { \
        MW_LOG_MODULE(this); MW_LOG(F("error link param index: ")); MW_LOG_PROGMEM(_linkToModule[linksIndex]->name); MW_LOG(':'); MW_LOG_PROGMEM(_linkToModuleParam[linksIndex]->name); MW_LOG(':'); MW_LOG_LN(_linkToValueIndex[linksIndex]); \
        _linkToValueIndex[linksIndex]=0; \
        return; \
    } \
    MW_LOG_MODULE(this); MW_LOG(F("onInited link: ")); MW_LOG_PROGMEM(_linkToModule[linksIndex]->name); MW_LOG(':'); MW_LOG_PROGMEM(_linkToModuleParam[linksIndex]->name); MW_LOG(':'); MW_LOG_LN(_linkToValueIndex[linksIndex]); \
} \
\
void setDefaultParamLinkToValue(uint16_t linksIndex,MWOSModuleBase * module, MWOS_PARAM_UINT paramId, MWOS_PARAM_INDEX_UINT index) { \
    _linkToModule[linksIndex]=module; \
    _linkToModuleParam[linksIndex]=module->getParam(paramId); \
    _linkToValueIndex[linksIndex]=index; \
} \
\
bool IsParamsLinkToValue(uint16_t linksIndex) { \
    return (linksIndex<_p_links_count) && _linkToModule[linksIndex]!=NULL && _linkToModuleParam[linksIndex]!=NULL; \
} \
\
bool GetValueParamLinkToValue(uint16_t linksIndex) { \
    if (!IsParamsLinkToValue(linksIndex)) return false; \
    int64_t nowValue=_linkToModule[linksIndex]->getValue(_linkToModuleParam[linksIndex], _linkToValueIndex[linksIndex]); \
    if (_linkParamValue[linksIndex]!=nowValue) { \
        _linkParamValue[linksIndex]=nowValue; \
        return true; \
    } \
    return false; \
} \
\
bool IsIdParamForLink(uint16_t paramId) { \
    return paramId==_p_moduleId || paramId==_p_paramId || paramId==_p_value_index; \
} \
\
int32_t IdParamForLink(uint16_t paramId, int16_t linksIndex) { \
    switch (paramId) {  \
        case _p_moduleId: if (_linkToModule[linksIndex]!=NULL) return _linkToModule[linksIndex]->id; else return -1; \
        case _p_paramId: if (_linkToModuleParam[linksIndex]!=NULL) return _linkToModuleParam[linksIndex]->id; else return -1; \
        case _p_value_index: return _linkToValueIndex[linksIndex]; \
    } \
    return -1; \
}

#endif //MWOS3_MWOSLINKTOVALUE_H
