#pragma once

struct LogParcer {
	LogParcer(int c) {
		_buf_size = c;
		_buf = new char[_buf_size + 9];
		reset();
	}
	~LogParcer() {
		delete[] _buf;
	}
	char* buf() { 
		return _buf; 
	}
	int size() { 
		return _size; 
	}
	void reset() { 
		_size = 0; _buf[0] = 0; riched = false;
	}
	bool compleated() { 
		return riched; 
	}
	int getLine(const char* sz, size_t cb, bool endWithNewLine) //returns number of bytes paresed in sz
	{
		int i = 0;
		const char* end = sz + cb;
		bool endOfLine = false;
		//skip leading new lines
		if (_size == 0)
		{
			for (; i < cb && (sz[i] == '\n' || sz[i] == '\r'); i++)
			{
			}
		}
		for (; i < cb && (sz[i] != '\n' && sz[i] != '\r'); i++)
		{
			if (_size < _buf_size)
			{
				_buf[_size] = sz[i];
				_size++;
			}
			else
			{
				ATLASSERT(false);
				break;
			}

		}
		riched = (_size > 0 && i < cb && (sz[i] == '\n' || sz[i] == '\r')) || (_size == _buf_size);
		//skip trailing new lines
		for (; i < cb && (sz[i] == '\n' || sz[i] == '\r'); i++)
		{
		}
		if (riched)
		{
			if (endWithNewLine)
			{
				_buf[_size] = '\n';
				_size++;
			}
		}
		_buf[_size] = 0;
		return i;
	}
private:
	bool riched;
	char* _buf;
	int _buf_size;
	int _size;
};