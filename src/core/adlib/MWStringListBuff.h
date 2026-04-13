#ifndef MWSTRINGLISTBUFF_H
#define MWSTRINGLISTBUFF_H

/**
 * Найти подстроку по номеру. Портит исходный буфер (заменяет ';' на 0).
 * @param buffer
 * @param index
 * @return  Подстрока (или пустая строка)
 */
uint8_t* getSubString(uint8_t* buffer, unsigned int index, char delimiter=';') {
    if (buffer == nullptr) return nullptr;
    uint8_t* current = buffer;
    unsigned int count = 0;
    // Идем по строке в один проход
    while (*current != '\0') {
        if (count == index) {
            uint8_t* start = current;
            // Ищем конец текущей подстроки (до ';' или до конца буфера)
            while (*current != '\0' && *current != delimiter) {
                current++;
            }
            // Заменяем разделитель на ноль (терминируем подстроку)
            *current = '\0';
            return start;
        }

        if (*current == delimiter) {
            count++;
        }
        current++;
    }
    // Если индекс не найден (count < index),
    // current сейчас указывает на финальный '\0' всей строки
    return current;
}


#endif
