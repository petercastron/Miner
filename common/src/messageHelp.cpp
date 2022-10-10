#include "messageHelp.h"

#if !defined(NOTUSING_SEMAPHORE)
//class sem_event
sem_event::sem_event()
{
  _psemaphore = SEM_FAILED;
  _sem_name = "";
  _is_inited = false;
  _init_locker_sem_event();
}

sem_event::~sem_event()
{
  event_close();
  _destroy_locker_sem_event();
}

bool sem_event::event_init(const std::string &name)
{
  bool rtCode = false;

  lock_sem_event();
  if (false == _is_inited) {
    char szbuf[128] = {0};
    sprintf(szbuf, "%s%p", name.c_str(), this);
    
    _sem_name = szbuf;
    if (_sem_name.empty()) rtCode = false;

    sem_unlink(_sem_name.c_str());
    _psemaphore = sem_open(_sem_name.c_str(), O_CREAT | O_EXCL, 0644, 1);
    if (_psemaphore == SEM_FAILED) rtCode = false;
    else {
      rtCode = true;
      _is_inited = true;
    }
  } else rtCode = true;
  unlock_sem_event();

  return rtCode;
}

void sem_event::event_close()
{
  lock_sem_event();
  if (true == _is_inited) {

    if (SEM_FAILED != _psemaphore) {
      event_notify();
      sem_close(_psemaphore);
    }

    if (!_sem_name.empty())
      sem_unlink(_sem_name.c_str());

    _psemaphore = SEM_FAILED;
    _sem_name = "";
    _is_inited = false;
  }
  unlock_sem_event();
}
//end of class sem_event
#endif

//class cond_event
cond_event::cond_event()
{
  _is_inited = false;
  _init_locker_cond_event();
}

cond_event::~cond_event()
{
  event_close();
  _destroy_locker_cond_event();
}

bool cond_event::event_init(const std::string &name)
{
  bool rtCode = false;

  lock_cond_event();
  if (false == _is_inited) {
    if (0 != pthread_cond_init(&_cond, NULL)) rtCode = false;
    else {
      _is_inited = true;
      rtCode = true;
    }
  } else rtCode = true;
  unlock_cond_event();

  return rtCode;
}

void cond_event::event_close()
{
  if (true == _is_inited) {
    event_notify();

    lock_cond_event();
    _is_inited = false;
    pthread_cond_destroy(&_cond);
    unlock_cond_event();
  }
}
//end of class cond_event


//class thread_worker
thread_worker::thread_worker()
{
  _thread_id = 0;
  _need_join = true;
}

thread_worker::~thread_worker()
{
  wait_thread_stop();
}

bool thread_worker::thread_worker_start(funThread fun, void *parmame, bool need_join, int policy, int priority)
{
  int result = -1;
  pthread_attr_t attr;
  pthread_attr_t *pattr = NULL;

  _need_join = need_join;

  pthread_attr_init(&attr);

  if (!_need_join) {
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (NULL == pattr) pattr = &attr;
  }

  if ((SCHED_RR == policy || SCHED_FIFO == policy) && 0 != priority) {
    struct sched_param param;
    param.sched_priority = priority;
    pthread_attr_setschedpolicy(&attr, policy);
    pthread_attr_setschedparam(&attr, &param);
    //pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);//要使优先级其作用必须要有这句话
    if (NULL == pattr) pattr = &attr;
  }

  result = pthread_create(&_thread_id, pattr, fun, parmame);

  pthread_attr_destroy(&attr);
  if (0 == result) return true;
  else return false;
}

void thread_worker::wait_thread_stop()
{
  if ((0 != _thread_id) && _need_join) {
    pthread_join(_thread_id, NULL);
    _thread_id = 0;
  }
}

bool thread_worker::cpu_bind(u_int num)
{
#ifdef OPEN_CPU_BIND
  cpu_set_t cpu_info;
  CPU_ZERO(&cpu_info);
  CPU_SET(num, &cpu_info);
  if (0 != pthread_setaffinity_np(_thread_id, sizeof(cpu_set_t), &cpu_info)) {
    return false;
  }
#endif
  return true;
}

//end of class thread_worker


