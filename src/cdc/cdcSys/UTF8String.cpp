#include "cdc/cdcSys/UTF8String.h"

namespace cdc {

// 0x00455000 / 0x005df210
UTF8String::UTF8String()
    : m_nChars(0), m_length(0), m_allocated(0), m_pBuffer(nullptr)
{
}

UTF8String::UTF8String(const char *str)
    : m_nChars(0), m_length(0), m_allocated(0), m_pBuffer(nullptr)
{
    if (str) {
        m_length = (int)strlen(str);
        m_allocated = m_length + 1;
        m_pBuffer = (char *)malloc(m_allocated);
        if (m_pBuffer) {
            memcpy(m_pBuffer, str, m_allocated);
        }
        m_nChars = m_length; // simplified
    }
}

UTF8String::UTF8String(const wchar_t *wstr)
    : m_nChars(0), m_length(0), m_allocated(0), m_pBuffer(nullptr)
{
    if (wstr) {
#ifdef _WIN32
        int bytes = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (bytes > 0) {
            m_pBuffer = (char *)malloc(bytes);
            if (m_pBuffer) {
                WideCharToMultiByte(CP_UTF8, 0, wstr, -1, m_pBuffer, bytes, nullptr, nullptr);
                m_length = bytes - 1;
                m_allocated = bytes;
                m_nChars = m_length;
            }
        }
#endif
    }
}

UTF8String::UTF8String(const UTF8String &other)
    : m_nChars(other.m_nChars), m_length(other.m_length), m_allocated(0), m_pBuffer(nullptr)
{
    if (other.m_pBuffer && other.m_length > 0) {
        m_allocated = other.m_length + 1;
        m_pBuffer = (char *)malloc(m_allocated);
        if (m_pBuffer) {
            memcpy(m_pBuffer, other.m_pBuffer, m_allocated);
        }
    }
}

// 0x00401c30 / 0x004f7640
UTF8String::~UTF8String()
{
    if (m_pBuffer && m_allocated) {
        free(m_pBuffer);
        m_pBuffer = nullptr;
    }
}

UTF8String &UTF8String::operator=(const UTF8String &other)
{
    if (this != &other) {
        if (m_pBuffer && m_allocated) {
            free(m_pBuffer);
            m_pBuffer = nullptr;
        }
        m_nChars = other.m_nChars;
        m_length = other.m_length;
        m_allocated = 0;
        if (other.m_pBuffer && other.m_length > 0) {
            m_allocated = other.m_length + 1;
            m_pBuffer = (char *)malloc(m_allocated);
            if (m_pBuffer) {
                memcpy(m_pBuffer, other.m_pBuffer, m_allocated);
            }
        }
    }
    return *this;
}

UTF8String &UTF8String::operator=(const char *str)
{
    if (m_pBuffer && m_allocated) {
        free(m_pBuffer);
        m_pBuffer = nullptr;
    }
    m_nChars = 0;
    m_length = 0;
    m_allocated = 0;
    if (str) {
        m_length = (int)strlen(str);
        m_allocated = m_length + 1;
        m_pBuffer = (char *)malloc(m_allocated);
        if (m_pBuffer) {
            memcpy(m_pBuffer, str, m_allocated);
        }
        m_nChars = m_length;
    }
    return *this;
}

// 0x005df010
void UTF8String::ConvertToWCHAR(wchar_t *dest, unsigned int maxChars) const
{
    if (!dest || maxChars == 0) return;
    if (!m_pBuffer) {
        dest[0] = L'\0';
        return;
    }
#ifdef _WIN32
    MultiByteToWideChar(CP_UTF8, 0, m_pBuffer, -1, dest, (int)maxChars);
#else
    // Posix fallback if needed
    dest[0] = L'\0';
#endif
}

} // namespace cdc
