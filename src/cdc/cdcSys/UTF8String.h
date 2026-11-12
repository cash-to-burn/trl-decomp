#pragma once

#ifndef CDC_UTF8STRING_H
#define CDC_UTF8STRING_H

#include "common.h"

namespace cdc {

class UTF8String {
public:
    UTF8String();
    UTF8String(const char *str);
    UTF8String(const wchar_t *wstr);
    UTF8String(const UTF8String &other);
    ~UTF8String();

    UTF8String &operator=(const UTF8String &other);
    UTF8String &operator=(const char *str);

    void ConvertToWCHAR(wchar_t *dest, unsigned int maxChars) const;
    const char *c_str() const { return m_pBuffer ? m_pBuffer : ""; }
    int length() const { return m_length; }
    bool empty() const { return m_length == 0; }

private:
    int m_nChars;       // 0x00: number of characters
    int m_length;       // 0x04: length in bytes
    int m_allocated;    // 0x08: allocated capacity / heap flag
    char *m_pBuffer;    // 0x0C: pointer to UTF-8 string data
};

} // namespace cdc

#endif // CDC_UTF8STRING_H
