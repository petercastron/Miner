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

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#if defined(CPP_11_SUPPORT)
template <typename T, size_t BlockSize = 4096>
class MemoryPool  //build use c++11
{
public:
  /* Member types */
  typedef T               value_type;
  typedef T*              pointer;
  typedef T&              reference;
  typedef const T*        const_pointer;
  typedef const T&        const_reference;
  typedef size_t          size_type;
  typedef ptrdiff_t       difference_type;
  typedef std::false_type propagate_on_container_copy_assignment;
  typedef std::true_type  propagate_on_container_move_assignment;
  typedef std::true_type  propagate_on_container_swap;

  template <typename U> struct rebind {
    typedef MemoryPool<U> other;
  };

  /* Member functions */
  MemoryPool() noexcept;
  MemoryPool(const MemoryPool& memoryPool) noexcept;
  MemoryPool(MemoryPool&& memoryPool) noexcept;
  template <class U> MemoryPool(const MemoryPool<U>& memoryPool) noexcept;

  ~MemoryPool() noexcept;

  MemoryPool& operator=(const MemoryPool& memoryPool) = delete;
  MemoryPool& operator=(MemoryPool&& memoryPool) noexcept;

  pointer address(reference x) const noexcept;
  const_pointer address(const_reference x) const noexcept;

  // Can only allocate one object at a time. n and hint are ignored
  pointer allocate(size_type n = 1, const_pointer hint = 0);
  void deallocate(pointer p, size_type n = 1);

  size_type max_size() const noexcept;

  template <class U, class... Args> void construct(U* p, Args&&... args);
  template <class U> void destroy(U* p);

  template <class... Args> pointer newElement(Args&&... args);
  void deleteElement(pointer p);

  void purge_memory();

  size_t size() { return (curBlock_count * BlockSize); }
  void set_maxBlock_count(size_t count) { maxBlock_count = count; }
  size_t get_maxBlock_count() { return maxBlock_count; }
  size_t get_curBlock_count() { return curBlock_count; }
private:
  union Slot_ {
    value_type element;
    Slot_* next;
  };

  typedef char* data_pointer_;
  typedef Slot_ slot_type_;
  typedef Slot_* slot_pointer_;

  slot_pointer_ currentBlock_;
  slot_pointer_ currentSlot_;
  slot_pointer_ lastSlot_;
  slot_pointer_ freeSlots_;
  size_t maxBlock_count;
  size_t curBlock_count;

  size_type padPointer(data_pointer_ p, size_type align) const noexcept;
  bool allocateBlock();
  
  /*
  static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize too small.");
  */
};
#else

template<typename T, size_t BlockSize = 4096>
class MemoryPool  //build use c++98
{
public:
  /* Member types */
  typedef T value_type;
  typedef T *pointer;
  typedef T &reference;
  typedef const T *const_pointer;
  typedef const T &const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  template<typename U>
  struct rebind
  {
    typedef MemoryPool<U> other;
  };

  /* Member functions */
  MemoryPool() throw();
  MemoryPool(const MemoryPool &memoryPool) throw();
  template<class U>
  MemoryPool(const MemoryPool<U> &memoryPool) throw();

  ~MemoryPool() throw();

  pointer address(reference x) const throw();
  const_pointer address(const_reference x) const throw();

  // Can only allocate one object at a time. n and hint are ignored
  pointer allocate(size_type n = 1, const_pointer hint = 0);
  void deallocate(pointer p, size_type n = 1);

  size_type max_size() const throw();

  void construct(pointer p, const_reference val);
  void destroy(pointer p);

  pointer newElement(const_reference val);
  void deleteElement(pointer p);

  void purge_memory();

  size_t size() { return curBlock_count * BlockSize; }
  void set_maxBlock_count(size_t count) { maxBlock_count = count; }
  size_t get_maxBlock_count() { return maxBlock_count; }
  size_t get_curBlock_count() { return curBlock_count; }
private:
  union Slot_
  {
    value_type element;
    Slot_ *next;
  };

  typedef char *data_pointer_;
  typedef Slot_ slot_type_;
  typedef Slot_ *slot_pointer_;

  slot_pointer_ currentBlock_;
  slot_pointer_ currentSlot_;
  slot_pointer_ lastSlot_;
  slot_pointer_ freeSlots_;
  size_t maxBlock_count;
  size_t curBlock_count;

  size_type padPointer(data_pointer_ p, size_type align) const throw();
  bool allocateBlock();
  /*
   static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize too small.");
   */
};

#endif

#include "MemoryPool.tcc"

#endif // MEMORY_POOL_H
