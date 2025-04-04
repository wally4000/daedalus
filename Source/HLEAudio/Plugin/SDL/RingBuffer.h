#pragma once 

#include <vector> 
#include <mutex>
#include "Base/Types.h"

class AudioRingBuffer
{
public:
    AudioRingBuffer(size_t capacity)
        : mBuffer(capacity), mHead(0), mTail(0), mSize(0) {}

    bool Push(const void* data, size_t size)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        if (size > AvailableSpace())
            return false;

        const u8* dataPtr = static_cast<const u8*>(data);

        for (size_t i = 0; i < size; ++i)
        {
            mBuffer[mHead] = dataPtr[i];
            mHead = (mHead + 1) % mBuffer.size();
            ++mSize;
        }

        return true;
    }

    size_t Pop(void* data, size_t size)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        size_t bytesRead = std::min(size, mSize);
        u8* dataPtr = static_cast<u8*>(data);

        for (size_t i = 0; i < bytesRead; ++i)
        {
            dataPtr[i] = mBuffer[mTail];
            mTail = (mTail + 1) % mBuffer.size();
            --mSize;
        }

        return bytesRead;
    }

    size_t AvailableSpace() const { return mBuffer.size() - mSize; }
    size_t Size() const { return mSize; }

private:
    std::vector<u8> mBuffer;
    size_t mHead, mTail, mSize;
    mutable std::mutex mMutex;
};
