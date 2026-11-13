#pragma once

#ifndef CDC_ARRAY_H
#define CDC_ARRAY_H

#include <cstdint>
#include <cstdlib>

namespace cdc {

// Lightweight dynamic array matching PDB cdc::ArrayImpl / cdc::Array<T> (size: 0x0c)
template <typename T>
struct Array {
    uint32_t size;     // +0x00
    uint32_t capacity; // +0x04
    T* data;           // +0x08

    Array() : size(0), capacity(0), data(nullptr) {}

    ~Array()
    {
        if (capacity != 0) {
            std::free(data);
        }
    }

    // 0x00415e90 (ArrayImpl::reserve)
    void reserve(uint32_t newCap)
    {
        if (newCap <= capacity) {
            return;
        }
        T* newData = static_cast<T*>(std::malloc(newCap * sizeof(T)));
        for (uint32_t i = 0; i < size; ++i) {
            newData[i] = data[i];
        }
        if (capacity != 0) {
            std::free(data);
        }
        capacity = newCap;
        data = newData;
    }

    void push_back(const T& val)
    {
        if (size == capacity) {
            const uint32_t newCap = (capacity > 0) ? (capacity * 2) : 1;
            reserve(newCap);
        }
        if (data != nullptr) {
            data[size] = val;
        }
        ++size;
    }
};

static_assert(sizeof(Array<void*>) == 0x0c, "cdc::Array size mismatch");

} // namespace cdc

#endif // CDC_ARRAY_H
