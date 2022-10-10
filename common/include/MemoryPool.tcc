/*-
 * Copyright (c) 2013 Cosku Acay, http://www.coskuacay.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MEMORY_BLOCK_TCC
#define MEMORY_BLOCK_TCC

#if defined(CPP_11_SUPPORT)
template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align)
const noexcept
{
  uintptr_t result = reinterpret_cast<uintptr_t>(p);
  return ((align - result) % align);
}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()
noexcept
{
  currentBlock_ = nullptr;
  currentSlot_ = nullptr;
  lastSlot_ = nullptr;
  freeSlots_ = nullptr;
  maxBlock_count = 40;
  curBlock_count = 0;
}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool& memoryPool)
noexcept :
    MemoryPool()
{}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool&& memoryPool)
noexcept
{
  currentBlock_ = memoryPool.currentBlock_;
  memoryPool.currentBlock_ = nullptr;
  currentSlot_ = memoryPool.currentSlot_;
  lastSlot_ = memoryPool.lastSlot_;
  freeSlots_ = memoryPool.freeSlots;
  maxBlock_count = memoryPool.maxBlock_count;
  curBlock_count = memoryPool.curBlock_count;
}


template <typename T, size_t BlockSize>
template<class U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U>& memoryPool)
noexcept :
    MemoryPool()
{}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>&
MemoryPool<T, BlockSize>::operator=(MemoryPool&& memoryPool)
noexcept
{
  if (this != &memoryPool)
  {
    std::swap(currentBlock_, memoryPool.currentBlock_);
    currentSlot_ = memoryPool.currentSlot_;
    lastSlot_ = memoryPool.lastSlot_;
    freeSlots_ = memoryPool.freeSlots;
    maxBlock_count = memoryPool.maxBlock_count;
    curBlock_count = memoryPool.curBlock_count;
  }
  return *this;
}



template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool()
noexcept
{
  purge_memory();
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference x)
const noexcept
{
  return &x;
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference x)
const noexcept
{
  return &x;
}



template <typename T, size_t BlockSize>
bool
MemoryPool<T, BlockSize>::allocateBlock()
{
  // Allocate space for the new block and store a pointer to the previous one
  if(curBlock_count >= maxBlock_count) return false;
  void* ptr = operator new(BlockSize, std::nothrow);
  if(nullptr == ptr) return false;
  ++curBlock_count;
  data_pointer_ newBlock = reinterpret_cast<data_pointer_>(ptr);
  reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
  currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
  // Pad block body to staisfy the alignment requirements for elements
  data_pointer_ body = newBlock + sizeof(slot_pointer_);
  size_type bodyPadding = padPointer(body, alignof(slot_type_));
  currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
  lastSlot_ = reinterpret_cast<slot_pointer_>
  (newBlock + BlockSize - sizeof(slot_type_) + 1);
  return true;
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type n, const_pointer hint)
{
  if (freeSlots_ != nullptr) {
    pointer result = reinterpret_cast<pointer>(freeSlots_);
    freeSlots_ = freeSlots_->next;
    return result;
  }
  else {
    if (currentSlot_ >= lastSlot_){
      if(!allocateBlock())
        return nullptr;
    }
    return reinterpret_cast<pointer>(currentSlot_++);
  }
}



template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n)
{
  if (p != nullptr && nullptr != currentBlock_) {
    reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
    freeSlots_ = reinterpret_cast<slot_pointer_>(p);
  }
}



template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size()
const noexcept
{
  size_type maxBlocks = -1 / BlockSize;
  return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) * maxBlocks;
}



template <typename T, size_t BlockSize>
template <class U, class... Args>
inline void
MemoryPool<T, BlockSize>::construct(U* p, Args&&... args)
{
  new (p) U (std::forward<Args>(args)...);
}



template <typename T, size_t BlockSize>
template <class U>
inline void
MemoryPool<T, BlockSize>::destroy(U* p)
{
  p->~U();
}



template <typename T, size_t BlockSize>
template <class... Args>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args&&... args)
{
  pointer result = allocate();
  construct<value_type>(result, std::forward<Args>(args)...);
  return result;
}



template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deleteElement(pointer p)
{
  if (p != nullptr) {
    p->~value_type();
    deallocate(p);
  }
}

template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::purge_memory()
{
  slot_pointer_ curr = currentBlock_;
  while (curr != nullptr) {
    slot_pointer_ prev = curr->next;
    operator delete(reinterpret_cast<void*>(curr));
    curr = prev;
  }

  currentBlock_ = nullptr;
  currentSlot_ = nullptr;
  lastSlot_ = nullptr;
  freeSlots_ = nullptr;
  curBlock_count = 0;
}
#else
template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer(data_pointer_ p, size_type align)
const throw()
{
  size_t result = reinterpret_cast<size_t>(p);
  return ((align - result) % align);
}


template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()
throw()
{
  currentBlock_ = 0;
  currentSlot_ = 0;
  lastSlot_ = 0;
  freeSlots_ = 0;
  maxBlock_count = 40;
  curBlock_count = 0;
}


template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool &memoryPool)
throw()
{
  MemoryPool();
}


template<typename T, size_t BlockSize>
template<class U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool <U> &memoryPool)
throw()
{
  MemoryPool();
}


template<typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool()
throw()
{
  purge_memory();
}


template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::address(reference x)
const throw()
{
  return &x;
}


template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference x)
const throw()
{
  return &x;
}


template<typename T, size_t BlockSize>
bool
MemoryPool<T, BlockSize>::allocateBlock()
{
  // Allocate space for the new block and store a pointer to the previous one
  if (curBlock_count >= maxBlock_count) return false;
  void *ptr = operator new(BlockSize, std::nothrow);
  if (NULL == ptr) return false;
  ++curBlock_count;
  data_pointer_ newBlock = reinterpret_cast<data_pointer_>(ptr);
  reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
  currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
  // Pad block body to staisfy the alignment requirements for elements
  data_pointer_ body = newBlock + sizeof(slot_pointer_);
  size_type bodyPadding = padPointer(body, sizeof(slot_type_));
  currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
  lastSlot_ = reinterpret_cast<slot_pointer_>
  (newBlock + BlockSize - sizeof(slot_type_) + 1);
  return true;
}


template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type, const_pointer)
{
  if (freeSlots_ != 0) {
    pointer result = reinterpret_cast<pointer>(freeSlots_);
    freeSlots_ = freeSlots_->next;
    return result;
  }
  else {
    if (currentSlot_ >= lastSlot_) {
      if (!allocateBlock())
        return NULL;
    }
    return reinterpret_cast<pointer>(currentSlot_++);
  }
}


template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type)
{
  if (p != 0 && 0 != currentBlock_) {
    reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
    freeSlots_ = reinterpret_cast<slot_pointer_>(p);
  }
}


template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size()
const throw()
{
  size_type maxBlocks = -1 / BlockSize;
  return (BlockSize - sizeof(data_pointer_)) / sizeof(slot_type_) * maxBlocks;
}


template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::construct(pointer p, const_reference val)
{
  new(p) value_type(val);
}


template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::destroy(pointer p)
{
  p->~value_type();
}


template<typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(const_reference val)
{
  pointer result = allocate();
  construct(result, val);
  return result;
}


template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deleteElement(pointer p)
{
  if (p != 0) {
    p->~value_type();
    deallocate(p);
  }
}

template<typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::purge_memory()
{
  slot_pointer_ curr = currentBlock_;
  while (curr != 0) {
    slot_pointer_ prev = curr->next;
    operator delete(reinterpret_cast<void *>(curr));
    curr = prev;
  }

  currentBlock_ = 0;
  currentSlot_ = 0;
  lastSlot_ = 0;
  freeSlots_ = 0;
  curBlock_count = 0;
}
#endif

#endif // MEMORY_BLOCK_TCC
