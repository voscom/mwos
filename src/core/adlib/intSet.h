#ifndef intSet_h
#define intSet_h

class intSet {
public:

	intSet(String name) {
		_name=name;
	}

	/**
	 * Задать новое значение
	 */
	void set(int16_t newValue) {
		_nowValue=newValue;
		MW_LOG(_name); MW_LOG('='); MW_LOG_LN(_nowValue);
	}

	/**
	 * Прочитать текущее значение
	 */
	int16_t get() {
		return _nowValue;
	}


private:
	int16_t _nowValue;
	String _name;

};

#endif
