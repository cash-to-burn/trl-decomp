#pragma once

#ifndef GAME_ARCHIVE_ARCHIVEFILESYSTEM_H
#define GAME_ARCHIVE_ARCHIVEFILESYSTEM_H

#include "common.h"
#include "cdc/cdcFile/FileSystem.h"

class ArchiveFileSystem : public cdc::FileSystem {
public:
    // 0x0047ac70
    ArchiveFileSystem(cdc::FileSystem *parentFS = nullptr);
    virtual ~ArchiveFileSystem();

    virtual bool FileExists(const char *path) override;
    virtual uint32_t GetFileSize(const char *path) override;
    virtual void *OpenFile(const char *path) override;
    virtual void Release() override;
    virtual int Poll() override { return 0; }
    virtual void ProcessRequests() override {}

    // 0x0047a990
    bool Open(const char *archiveName);

    // 0x0047aea0
    static uint32_t CalculateHash(const char *path);

private:
    cdc::FileSystem *m_pParentFS;
    void *m_pFileHandle;
    uint32_t m_entryCount;
    void *m_pTocEntries;
    int m_numBigfiles;
};

extern cdc::FileSystem *g_pFileSystem;
extern ArchiveFileSystem *g_pArchiveFS;

#ifdef __cplusplus
extern "C" {
#endif

// 0x0045c2a0 - Initializes native OS filesystem abstraction
void InitFS(void);

// 0x0045c300 - Initializes Bigfile virtual archive filesystem
void InitArchive(void);

// 0x0045c360 - Releases Bigfile archive and native filesystem
void ReleaseFS(void);

#ifdef __cplusplus
}
#endif

#endif // GAME_ARCHIVE_ARCHIVEFILESYSTEM_H
