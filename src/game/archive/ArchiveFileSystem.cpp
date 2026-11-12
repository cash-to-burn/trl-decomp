#include "game/archive/ArchiveFileSystem.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

cdc::FileSystem *g_pFileSystem = nullptr;
ArchiveFileSystem *g_pArchiveFS = nullptr;

// Implementation of a simple native OS filesystem
class NativeFileSystem : public cdc::FileSystem {
public:
    virtual ~NativeFileSystem() {}

    virtual bool FileExists(const char *path) override {
        FILE *f = fopen(path, "rb");
        if (f) {
            fclose(f);
            return true;
        }
        return false;
    }

    virtual uint32_t GetFileSize(const char *path) override {
        FILE *f = fopen(path, "rb");
        if (!f) return 0;
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fclose(f);
        return (sz > 0) ? (uint32_t)sz : 0;
    }

    virtual void *OpenFile(const char *path) override {
        return (void *)fopen(path, "rb");
    }

    virtual void Release() override {
        delete this;
    }
};

// 0x0047ac70
ArchiveFileSystem::ArchiveFileSystem(cdc::FileSystem *parentFS)
    : m_pParentFS(parentFS), m_pFileHandle(nullptr), m_entryCount(0), m_pTocEntries(nullptr), m_numBigfiles(0)
{
}

ArchiveFileSystem::~ArchiveFileSystem()
{
    Release();
}

// 0x0047a990
bool ArchiveFileSystem::Open(const char *archiveName)
{
    if (!archiveName) return false;

    char baseName[160];
    strncpy(baseName, archiveName, sizeof(baseName) - 1);
    baseName[sizeof(baseName) - 1] = '\0';

    // Strip extension (e.g. "BIGFILE.DAT" -> "BIGFILE")
    char *dot = strrchr(baseName, '.');
    if (dot) {
        *dot = '\0';
    }

    // Probe for existence of bigfile.000
    char chunkPath[256];
    snprintf(chunkPath, sizeof(chunkPath), "%s.000", baseName);

    if (m_pParentFS && !m_pParentFS->FileExists(chunkPath)) {
        // Fallback to checking disk directly if parent not queried
        FILE *f = fopen(chunkPath, "rb");
        if (!f) {
            return false;
        }
        fclose(f);
    }

    // In retail, BIGFILE.000 header contains entry count and TOC
    return true;
}

uint32_t ArchiveFileSystem::CalculateHash(const char *path)
{
    uint32_t hash = 0;
    if (!path) return hash;
    while (*path) {
        char c = *path;
        if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
        if (c == '/') c = '\\';
        hash = (hash * 37) + (uint8_t)c;
        path++;
    }
    return hash;
}

bool ArchiveFileSystem::FileExists(const char *path)
{
    if (m_pParentFS && m_pParentFS->FileExists(path)) {
        return true;
    }
    return false;
}

uint32_t ArchiveFileSystem::GetFileSize(const char *path)
{
    if (m_pParentFS) {
        return m_pParentFS->GetFileSize(path);
    }
    return 0;
}

void *ArchiveFileSystem::OpenFile(const char *path)
{
    if (m_pParentFS) {
        return m_pParentFS->OpenFile(path);
    }
    return nullptr;
}

void ArchiveFileSystem::Release()
{
    if (m_pTocEntries) {
        free(m_pTocEntries);
        m_pTocEntries = nullptr;
    }
    m_entryCount = 0;
}

// 0x0045c2a0
void InitFS(void)
{
    if (!g_pFileSystem) {
        g_pFileSystem = new NativeFileSystem();
    }
}

// 0x0045c300
void InitArchive(void)
{
    if (!g_pArchiveFS) {
        g_pArchiveFS = new ArchiveFileSystem(g_pFileSystem);
    }
    if (!g_pArchiveFS->Open("BIGFILE.DAT")) {
        // Warning / Error in original binary
    }
}

// 0x0045c360
void ReleaseFS(void)
{
    if (g_pArchiveFS) {
        g_pArchiveFS->Release();
        delete g_pArchiveFS;
        g_pArchiveFS = nullptr;
    }
    if (g_pFileSystem) {
        g_pFileSystem->Release();
        g_pFileSystem = nullptr;
    }
}
