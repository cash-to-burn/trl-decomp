#include "game/g2/poolmmg2.h"
#include <cstddef>

// 0x004dffd0 (PDB: 0x004e1b00)
void G2PoolMem_ResetPool(void* voidpool)
{
    G2PoolMemPool* pool = static_cast<G2PoolMemPool*>(voidpool);
    pool->stackTop = 0;
    for (int i = 0; i < static_cast<int>(pool->stackSize); ++i) {
        pool->stack[i] = static_cast<uint16_t>(i);
    }
}

// 0x004e0000 (PDB: 0x004e1b30)
void* G2PoolMem_Allocate(void* voidpool)
{
    G2PoolMemPool* pool = static_cast<G2PoolMemPool*>(voidpool);
    if (pool->stackTop >= pool->stackSize) {
        return nullptr;
    }
    const uint16_t blockIndex = pool->stack[pool->stackTop++];
    return static_cast<uint8_t*>(pool->blockPool) +
           static_cast<size_t>(blockIndex) * static_cast<size_t>(pool->blockSize);
}

// 0x004e0030 (PDB: 0x004e1b60)
void G2PoolMem_Free(void* voidpool, void* block)
{
    G2PoolMemPool* pool = static_cast<G2PoolMemPool*>(voidpool);
    const std::ptrdiff_t offset =
        static_cast<uint8_t*>(block) - static_cast<uint8_t*>(pool->blockPool);
    const uint16_t blockIndex =
        static_cast<uint16_t>(offset / static_cast<std::ptrdiff_t>(pool->blockSize));
    pool->stack[--pool->stackTop] = blockIndex;
}

// 0x004e0060 (PDB: 0x004e1b90)
void G2PoolMem_InitPool(void* voidpool, int blockCount, int blockSize)
{
    G2PoolMemPool* pool = static_cast<G2PoolMemPool*>(voidpool);
    // Retail initializes cdc::Alloc::s_heapAllocator (0x005d9a00) and calls vtable Alloc(size, 0).
    pool->blockPool = new uint8_t[static_cast<size_t>(blockCount) * static_cast<size_t>(blockSize)];
    pool->stack = new uint16_t[static_cast<size_t>(blockCount)];
    pool->blockSize = static_cast<uint16_t>(blockSize);
    pool->stackSize = static_cast<uint16_t>(blockCount);
    G2PoolMem_ResetPool(pool);
}
