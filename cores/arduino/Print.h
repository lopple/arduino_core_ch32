/*
  Copyright (c) 2016 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef Print_h
#define Print_h

#include <inttypes.h>
#include <stdio.h> // for size_t
#include <stdarg.h> // for printf

#include "WString.h"
#include "Printable.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

#if defined(CH32V00x)
#define CORE_LIGHTWEIGHT_PRINT
#define CORE_LIGHTWEIGHT_PRINT_NO_FLOAT
#define CORE_LIGHTWEIGHT_PRINT_NO_ULL
#endif

class Print {
  private:
    int write_error;

    size_t printNumber(unsigned long, uint8_t);
#if !defined(CORE_LIGHTWEIGHT_PRINT_NO_ULL)
    size_t printULLNumber(unsigned long long, uint8_t);
#endif
#if !defined(CORE_LIGHTWEIGHT_PRINT_NO_FLOAT)
    size_t printFloat(double, uint8_t);
#endif

  protected:
#if defined(CORE_LIGHTWEIGHT_PRINT)
    size_t (*_write_char)(void*, uint8_t);
    void* _write_ctx;
#endif

    void setWriteError(int err = 1)
    {
      write_error = err;
    }
  public:
#if defined(CORE_LIGHTWEIGHT_PRINT)
    Print(size_t (*write_char)(void*, uint8_t), void* ctx) 
      : write_error(0), _write_char(write_char), _write_ctx(ctx) {}
#else
    Print() : write_error(0) {}
#endif

    int getWriteError()
    {
      return write_error;
    }
    void clearWriteError()
    {
      setWriteError(0);
    }

#if defined(CORE_LIGHTWEIGHT_PRINT)
    size_t write(uint8_t c)
    {
      if (_write_char) {
        return _write_char(_write_ctx, c);
      }
      return 0;
    }
#else
    virtual size_t write(uint8_t) = 0;
#endif

    size_t write(const char *str)
    {
      if (str == NULL) {
        return 0;
      }
      return write((const uint8_t *)str, strlen(str));
    }

#if defined(CORE_LIGHTWEIGHT_PRINT)
    size_t write(const uint8_t *buffer, size_t size);
#else
    virtual size_t write(const uint8_t *buffer, size_t size);
#endif

    size_t write(const char *buffer, size_t size)
    {
      return write((const uint8_t *)buffer, size);
    }

    // default to zero, meaning "a single write may block"
    // should be overridden by subclasses with buffering
#if defined(CORE_LIGHTWEIGHT_PRINT)
    int availableForWrite()
    {
      return 0;
    }
#else
    virtual int availableForWrite()
    {
      return 0;
    }
#endif

    size_t print(const __FlashStringHelper *);
    size_t print(const String &);
    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char, int = DEC);
    size_t print(int, int = DEC);
    size_t print(unsigned int, int = DEC);
    size_t print(long, int = DEC);
    size_t print(unsigned long, int = DEC);
#if !defined(CORE_LIGHTWEIGHT_PRINT_NO_ULL)
    size_t print(long long, int = DEC);
    size_t print(unsigned long long, int = DEC);
#endif
#if !defined(CORE_LIGHTWEIGHT_PRINT_NO_FLOAT)
    size_t print(double, int = 2);
#endif
    size_t print(const Printable &);

    size_t println(const __FlashStringHelper *);
    size_t println(const String &s);
    size_t println(const char[]);
    size_t println(char);
    size_t println(unsigned char, int = DEC);
    size_t println(int, int = DEC);
    size_t println(unsigned int, int = DEC);
    size_t println(long, int = DEC);
    size_t println(unsigned long, int = DEC);
#if !defined(CORE_LIGHTWEIGHT_PRINT_NO_ULL)
    size_t println(long long, int = DEC);
    size_t println(unsigned long long, int = DEC);
#endif
#if !defined(CORE_LIGHTWEIGHT_PRINT_NO_FLOAT)
    size_t println(double, int = 2);
#endif
    size_t println(const Printable &);
    size_t println(void);

    int printf(const char *format, ...);
    int printf(const __FlashStringHelper *format, ...);
    int vprintf(const __FlashStringHelper *format, va_list ap);
    int vprintf(const char *format, va_list ap);

#if defined(CORE_LIGHTWEIGHT_PRINT)
    void flush() { /* Empty */ }
#else
    virtual void flush() { /* Empty implementation for backward compatibility */ }
#endif
};

#endif
