#pragma once

#ifndef CDC_FILESYSTEM_H
#define CDC_FILESYSTEM_H

#include "common.h"

namespace cdc {

class FileSystem {
public:
    virtual ~FileSystem() {}

    virtual bool FileExists(const char *path) = 0;
    virtual uint32_t GetFileSize(const char *path) = 0;
    virtual void *OpenFile(const char *path) = 0;
    virtual void Release() = 0;
};

} // namespace cdc

#endif // CDC_FILESYSTEM_H
