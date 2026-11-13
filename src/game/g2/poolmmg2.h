#pragma once

#ifndef GAME_G2_POOLMMG2_H
#define GAME_G2_POOLMMG2_H

#include <cstdint>

// Size: 0x10 (16 bytes, TI: 0x9cf0)
struct G2PoolMemPool {
    /* 0x00 */ uint16_t blockSize;
    /* 0x02 */ uint16_t stackTop;
    /* 0x04 */ uint16_t stackSize;
    /* 0x06 */ uint16_t pad06;
    /* 0x08 */ uint16_t* stack;
    /* 0x0c */ void* blockPool;
};
static_assert(sizeof(G2PoolMemPool) == 0x10, "G2PoolMemPool size mismatch");

void G2PoolMem_ResetPool(void* voidpool);                           // 0x004dffd0 (PDB: 0x004e1b00)
void* G2PoolMem_Allocate(void* voidpool);                           // 0x004e0000 (PDB: 0x004e1b30)
void G2PoolMem_Free(void* voidpool, void* block);                   // 0x004e0030 (PDB: 0x004e1b60)
void G2PoolMem_InitPool(void* voidpool, int blockCount, int blockSize); // 0x004e0060 (PDB: 0x004e1b90)

#endif // GAME_G2_POOLMMG2_H
