# Компиляция

### Platform IO
Для компиляции с помошью PlatformIO используйте:

```
pio run -e платформа
```
При этом будет произведена компиляция под нужную платформу, 
и загрузка скомпилированной прошивки на микроконтроллер 
(микроконтроллер должен быть подключен к вашему ПК). 

Платформы уже прописаны в [platformio.ini](platformio.ini):
1. [esp32-s3-devkitc-1](./platform/sp32-s3-devkitc-1/README.md)
2. [megaatmega2560](./platform/megaatmega2560/README.md)
3. [esp32dev](./platform/esp32dev/README.md)
4. [bluepill_f103c8](./platform/bluepill_f103c8/README.md)
5. [blackpill_f411ce](./platform/blackpill_f411ce/README.md)

Например, для компиляции под [megaatmega2560](./platform/megaatmega2560/README.md):
```
pio run -e megaatmega2560
```

Смотрите эти примеры в [platformio.ini](platformio.ini), что-бы добавить свою платформу.
Это не сложно!

### Arduino IDE
Установите необходимые библиотеки в Arduino IDE. 
Список библиотек и ссылки на них можно посмотреть в [platformio.ini](platformio.ini).
Откройте в Arduino IDE скетч [mwos_example.ino](./mwos_example/mwos_example.ino) 
(этот скетч будет пустым). В реальности будет компилироваться
[mwos_example.cpp](./mwos_example/mwos_example.cpp). 
Для удобства так же можно открыть файл [mwos_example.cpp](./mwos_example/mwos_example.cpp)

Можете компилировать и загружать пример!

### Конфигурирование MWOS
Пример конфиругирования MWOS можно посмотреть в файле:
[MWOSConfig.h](./mwos_example/MWOSConfig.h)






