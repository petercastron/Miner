#pragma once

#include "defs.h"
#include "commonHead.h"
#include "MemoryPool.h"
#include <semaphore.h>
#include <string>
#include <queue>

class event_base
{
public:
  event_base()
  { }
  virtual ~event_base()
  { };
public:
  virtual bool event_init(const std::string &name) = 0;
  virtual bool event_notify() = 0;
  virtual void event_close() = 0;
  virtual bool event_wait() = 0;
  virtual int event_timewait(u_int timeout) = 0;
};

//
class cond_event : public event_base
{
public:
  cond_event();
  virtual ~cond_event();
public:
LOCK_V(cond_event)
public:
  virtual bool event_init(const std::string &name);
  virtual void event_close();

  virtual bool event_wait()
  {
    bool rtCode = false;
    if (!_is_inited) return false;
    lock_cond_event();
    if (0 == pthread_cond_wait(&_cond, &_locker_cond_event)) {
      rtCode = true;
    } else rtCode = false;
    unlock_cond_event();
    return rtCode;
  }

  virtual int event_timewait(u_int timeout)
  {
    int rtCode = 0;
    struct timeval tv;
    struct timespec ts;
    if (!_is_inited) return false;
    gettimeofday(&tv, NULL);
    ts.tv_sec = tv.tv_sec + timeout;
    ts.tv_nsec = 0;
    lock_cond_event();
    rtCode = pthread_cond_timedwait(&_cond, &_locker_cond_event, &ts);
    unlock_cond_event();
    return rtCode;
  }

  virtual bool event_notify()
  {
    bool rtCode = false;
    if (!_is_inited) return false;

    lock_cond_event();
    if (0 == pthread_cond_signal(&_cond)) {
      rtCode = true;
    } else rtCode = false;
    unlock_cond_event();
    return rtCode;
  }
private:
  bool _is_inited;
  pthread_cond_t _cond;
};

#if !defined(NOTUSING_SEMAPHORE)  //for system who not support semaphore, e.g Android

class sem_event : public event_base
{
public:
  sem_event();
  virtual ~sem_event();
public:
LOCK_V(sem_event)
public:
  virtual bool event_init(const std::string &name);
  virtual void event_close();
  virtual bool event_wait()
  {
    if (_is_inited && (SEM_FAILED != _psemaphore)) {
      if (0 == sem_wait(_psemaphore)) {
        return true;
      } else return false;
    } else return false;
  }
  virtual int event_timewait(u_int timeout)
  {
    return event_wait() ? 0 : -1;
  }

  virtual bool event_notify()
  {
    bool rtCode = false;

    if (!_is_inited || (SEM_FAILED == _psemaphore)) return false;

    if (0 == sem_post(_psemaphore)) {
      rtCode = true;
    } else rtCode = false;

    return rtCode;
  }
private:
  bool _is_inited;
  sem_t *_psemaphore;
  std::string _sem_name;
};

typedef sem_event obj_event;
#else
typedef cond_event obj_event;
#endif
//
typedef void *(*funThread)(void *);

class thread_worker
{
public:
  thread_worker();
  ~thread_worker();
public:
  bool thread_worker_start(funThread fun, void *parmame, bool need_join = true,
                           int policy = SCHED_OTHER, int priority = 0);
  void wait_thread_stop();
  bool cpu_bind(u_int num);
private:
  pthread_t _thread_id;
  bool _need_join;
};

//
template<typename T, size_t BlockSize = 4096>
class memery_pool
{
public:
  memery_pool()
  {
    _init_locker_memery_pool();
  }
  ~memery_pool()
  {
    clean_msg_pool();
    _destroy_locker_memery_pool();
  }
public:
LOCK_V(memery_pool)
public:
  inline T *msg_alloc()
  {
    T *pmsg = NULL;
    lock_memery_pool();
    pmsg = _msg_pool.allocate();
    unlock_memery_pool();
    return pmsg;
  }

  inline void msg_free(void *pmsg)
  {
    lock_memery_pool();
    _msg_pool.deallocate((T *) pmsg);
    unlock_memery_pool();
  }

  void clean_msg_pool()
  {
    lock_memery_pool();
    _msg_pool.purge_memory();
    unlock_memery_pool();
  }
  size_t size() { return _msg_pool.size(); }
  void setMaxBlockCount(size_t maxCount) { _msg_pool.set_maxBlock_count(maxCount); }
  size_t getMaxBlockCount() { return _msg_pool.get_maxBlock_count(); }
  size_t getCurBolckCount() { return _msg_pool.get_curBlock_count(); }
private:
  MemoryPool<T, BlockSize> _msg_pool;
};

//
typedef enum _msg_priority
{
  MSG_PRI_NULL = 0,
  MSG_PRI_LOWER,
  MSG_PRI_NORMAL,
  MSG_PRI_HIGH,
} MSG_PRI;

template<typename T>
class msg_queue
{
public:
  msg_queue()
  {
    _queue_max_size = 0;
    _init_locker_msg_queue();
  }

  ~msg_queue()
  {
    clean_queue();
    _destroy_locker_msg_queue();
  }
public:
LOCK_V(msg_queue)
public:
  inline void set_queue_max_size(u_int max_size)
  { _queue_max_size = max_size; }

  inline T get_from_queue()
  {
    T pmsg = T(); //c++11 value-initialization
    lock_msg_queue();
    do {
      if (!_msg_queue_high.empty()) {
        pmsg = _msg_queue_high.front();
        _msg_queue_high.pop();
        break;
      }

      if (!_msg_queue_normal.empty()) {
        pmsg = _msg_queue_normal.front();
        _msg_queue_normal.pop();
        break;
      }

      if (!_msg_queue_lower.empty()) {
        pmsg = _msg_queue_lower.front();
        _msg_queue_lower.pop();
        break;
      }
    } while (false);

    unlock_msg_queue();

    return pmsg;
  }

  inline void add_to_queue(T msg, bool &bempty, MSG_PRI mpri = MSG_PRI_HIGH)
  {
    lock_msg_queue();
    switch (mpri) {
      case MSG_PRI_LOWER:
        bempty = _msg_queue_lower.empty();
        if (0 != _queue_max_size && _msg_queue_lower.size() >= _queue_max_size)
          _msg_queue_lower.pop();
        _msg_queue_lower.push(msg);
        break;
      case MSG_PRI_NORMAL:
        bempty = _msg_queue_normal.empty();
        if (0 != _queue_max_size && _msg_queue_normal.size() >= _queue_max_size)
          _msg_queue_normal.pop();
        _msg_queue_normal.push(msg);
        break;
      case MSG_PRI_HIGH:
        bempty = _msg_queue_high.empty();
        if (0 != _queue_max_size && _msg_queue_high.size() >= _queue_max_size)
          _msg_queue_high.pop();
        _msg_queue_high.push(msg);
        break;
      default:
        break;
    }
    unlock_msg_queue();
  }

  void clean_queue()
  {
    lock_msg_queue();
    while (!_msg_queue_lower.empty())
      _msg_queue_lower.pop();
    while (!_msg_queue_normal.empty())
      _msg_queue_normal.pop();
    while (!_msg_queue_high.empty())
      _msg_queue_high.pop();
    unlock_msg_queue();
  }
  
  size_t size()
  {
    return (_msg_queue_high.size() + _msg_queue_normal.size() + _msg_queue_lower.size());
  }
private:
  std::queue<T> _msg_queue_lower;
  std::queue<T> _msg_queue_normal;
  std::queue<T> _msg_queue_high;
  u_int _queue_max_size;
};




