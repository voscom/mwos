# 📘 Documentation: `MWTimeout.h`
**Language:** C++11 | **Platform:** Arduino (AVR, ESP32, STM32, RP2040, etc.)

---

## 💡 Why are non-blocking timers needed?
Standard Arduino code often uses `delay()`, which **completely stops program execution** for a specified time. During this period, the microcontroller cannot poll sensors, process UART/I2C/SPI commands, update displays, or respond to button presses.

`MWTimeout` implements a **non-blocking approach**: the object remembers the start time and only checks in the `loop()` whether the specified time has passed. This allows:
- ✅ Executing multiple parallel processes without delays (LED blinking + button polling + data transmission).
- ✅ Building deterministic finite state machines (FSM) and responsive interfaces.
- ✅ Saving processor cycles by avoiding idle time and complex interrupts.

---

## 🆚 Why is `MWTimeout` better than manual comparison with `millis()`?
A typical manual approach looks like this:
```cpp
unsigned long lastTime = 0;
const unsigned long interval = 5000;

void loop() {
    if (millis() - lastTime >= interval) {
        lastTime = millis();
        // action...
    }
}
```
| Problem with manual code | How `MWTimeout` solves it |
|--------------------------|---------------------------|
| ❌ **Verbosity**<br>Each timer needs separate `lastTime`, `interval` variables. Code grows, RAM is used inefficiently. | ✅ **Encapsulation**: One object stores all state. API: `start()`, `isTimeout()`. |
| ❌ **Risk of logic errors**<br>Easy to forget updating `lastTime`, mix up subtraction order, or use a signed type, breaking overflow handling. | ✅ **Overflow protection**: Uses unsigned arithmetic. `millis()` overflow (~49 days) is handled correctly at the hardware level. |
| ❌ **No unit abstraction**<br>You have to manually convert seconds → milliseconds, performing division at runtime each time. | ✅ **Built-in converters**: `startMS()`, `startSec()` automatically scale values to `accuracy`. |
| ❌ **Hidden costs**<br>`unsigned long` always takes 4 bytes, even for a 1-second timer. Division `millis()/coefficient` is performed in software on AVR. | ✅ **RAM and CPU control**: `classTime` template sets size (1/2/4 bytes), and `divOptimization` replaces division with a 1-cycle shift. |

💡 **Bottom line:** `MWTimeout` eliminates boilerplate, protects against typical embedded errors, and provides predictable memory/cycle consumption while maintaining the flexibility of manual code.

---

## ⚡ Key Features

### 💾 RAM Savings
| Aspect | Description |
|--------|-------------|
| **Instance size** | Depends only on `classTime`. Takes **1, 2, or 4 bytes** per object. No hidden fields. |
| **Static time cache** | `static classTime nowTime` is **shared among all objects** of the same template instance. Eliminates duplication and reduces system timer calls. |
| **No dynamic memory** | The class does not use `new`, `malloc`, or standard containers. Everything is allocated on the stack/in `.data` at compile time. |

### 🚀 Division Optimization
| Aspect | Description |
|--------|-------------|
| **Bit shift instead of `/`** | When `divOptimization = true`, the operation `1000 / accuracy` is replaced with `>> shift`. The shift amount is calculated at compile time (`constexpr`). |
| **Why is this needed?** | 8-bit AVRs have no hardware divider. Division is performed in software in 50–200 cycles. Bit shift takes **1 cycle**. |
| **Accuracy** | The divisor is rounded to the nearest power of two. Priority: speed. For `accuracy=1000`, optimization is automatically disabled (divisor = 1, compiler optimizes the operation to zero). |

---

## 🛠 Template Parameters
```cpp
template<class classTime = uint16_t, uint16_t accuracy = 1000, bool divOptimization = false>
class MWTimeout { ... };
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `classTime` | `uint8_t`, `uint16_t`, `uint32_t` | `uint16_t` | Variable type for storing the deadline. Determines maximum timeout range and RAM size per object. |
| `accuracy` | `uint16_t` | `1000` | Tick precision: `1` = 1 sec, `10` = 1/10 sec, `100` = 1/100 sec, `1000` = 1/1000 sec (ms). |
| `divOptimization` | `bool` | `false` | Replaces division with shift. Recommended `true` for `accuracy != 1000` on AVR/ESP8266. |

---

## 📝 Usage Examples

### 1. Basic variant (ms, standard)
```cpp
MWTimeout<> relayTimer; // uint16_t, accuracy=1000 (1/1000 sec), no optimization

void loop() {
    if (digitalRead(BUTTON) == HIGH) {
        relayTimer.startMS(5000); // 5 second timeout
    }
    if (relayTimer.isTimeout()) {
        digitalWrite(LED, !digitalRead(LED));
    }
}
```

### 2. Extreme RAM savings (1 byte per timer)
```cpp
// accuracy=100 -> ticks of 1/100 sec. Range: 0..255 (max ~2.55 sec)
MWTimeout<uint8_t, 100, true> shortTimer; 

void loop() {
    shortTimer.startMS(500); // ~500 ms
    while (!shortTimer.isTimeout()) { /* poll sensors */ }
}
```

### 3. Maximum speed (division replacement)
```cpp
// accuracy=10 -> ticks of 1/10 sec. Division replaced with shift.
MWTimeout<uint16_t, 10, true> fastTimer;

void loop() {
    fastTimer.start(10); // 10 ticks = 1 second
    if (fastTimer.isTimeout()) { /* action */ }
}
```

---

## 📚 API Reference

### Constructors
| Method | Description |
|--------|-------------|
| `MWTimeout()` | Default constructor. Timer not started (`ms_start = 0`). |
| `MWTimeout(int32_t time, bool refresh=true)` | Starts timer for `time` ticks. |
| `MWTimeout(float timeSec, bool refresh=true)` | Starts timer for `timeSec` seconds. |

### Timer Control
| Method | Description |
|--------|-------------|
| `void start(int32_t time=0, bool refresh=true)` | Start in ticks (`[sec/accuracy]`). |
| `void startMS(int32_t timeMSec=0, bool refresh=true)` | Start in milliseconds. Automatically scales to `accuracy`. |
| `void startSec(float timeSec=0, bool refresh=true)` | Start in seconds (with fractional part). |
| `void stop()` | Stops the timer. `isTimeout()` will return `true`. |
| `bool isStarted()` | `true` if timer was started (`ms_start > 0`). |

### State Checking
| Method | Returns |
|--------|---------|
| `bool isTimeout(int32_t extra=0, bool refresh=true)` | Has timeout expired? `extra` adds ticks to the threshold. Also returns `true` if timer is not started. |
| `bool isTimeoutMS(float extraMS=0, bool refresh=true)` | Same, but `extra` in ms. |
| `bool isTimeoutSec(float extraSec=0, bool refresh=true)` | Same, but `extra` in seconds. |

### Getting Time
| Method | Returns |
|--------|---------|
| `int32_t timeout(int32_t extra=0, bool refresh=true)` | Ticks elapsed since timeout. `<0` if timer is still running. |
| `int32_t timeoutMS(int32_t extraMS=0, bool refresh=true)` | Milliseconds elapsed since timeout. |
| `float timeoutSec(float extraSec=0, bool refresh=true)` | Seconds elapsed since timeout. |

### Internal/Static
| Method | Description |
|--------|-------------|
| `static classTime getNowTime(bool refresh=true)` | Returns current time in ticks. When `refresh=false`, returns cached value. |

---

## ⚠️ Important Notes

### Time Caching (`refresh`)
By default, methods call `getNowTime(true)` and update system time. In high-load loops, it is recommended to call the timer with `refresh=true` once or explicitly call `MWTimeoutBase::refresh()`, then perform checks with `refresh=false`. This reduces `millis()` calls and increases determinism.

### `millis()` Overflow
Standard Arduino technique is used: unsigned type subtraction (`ms_now - ms_start`). Overflow (~49 days for 32-bit counter) is handled correctly without additional flags.

### Accuracy with `divOptimization=true`
The divisor `1000 / accuracy` is rounded to the nearest power of two. For example, for `accuracy=100` (divisor 10), shift 3 (`/8`) or 4 (`/16`) is used. Error can reach `~12–25%`. For time-critical tasks, use `accuracy=1000` or disable optimization.

### C++11 Compatibility
Uses `static_assert`, `constexpr`, `std::make_signed`, `std::round`. Ensure `c++11` or higher standard is enabled in project settings (enabled by default in modern Arduino/PlatformIO).

### RAM Size
```cpp
sizeof(MWTimeout<uint8_t, 100, true>)   // = 1 byte
sizeof(MWTimeout<uint16_t, 1000, false>)// = 2 bytes
sizeof(MWTimeout<uint32_t, 1000, false>)// = 4 bytes
```
The static field `nowTime` takes an additional 4 bytes in the `.bss` section, but is **shared among all MWTimeout instances**.

---

📄 *The file is completely self-contained, requires no external libraries except `<Arduino.h>`, and usually included `<type_traits>` and `<cmath>`. Ready for use in ISR-safe context with proper call synchronization.*
