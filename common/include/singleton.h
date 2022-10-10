#pragma once

#include <memory>
#include <cstring>

template <class T>
class singleton
{
public:
    virtual ~singleton () {}
    static T &instance ()
    {
        static T T_instance;
        return (T_instance);
    }
    
protected:
    singleton () {}
    
private:
    singleton ( const singleton& );
    const singleton& operator=( const singleton& );
};

#if 0
template <class T>
class singleton_ptr_auto
{
public:
    virtual ~singleton_ptr_auto () {}
    
    static T* instance ()
    {
        static std::unique_ptr<T> T_instance;
        if (!T_instance.get ()) {
            T_instance = std::unique_ptr<T>(new T);
        }
        return (T_instance.get());
    }
protected:
    singleton_ptr_auto () {}
};
#endif

template <class T>
class singleton_ptr
{
public:
	virtual ~singleton_ptr() {}
	static T* instance (bool clear=false)
	{
		static T* T_instance = NULL;
		if (NULL == T_instance) {
            if (true == clear) return NULL;
            T_instance = new T;
        }
        if (true == clear) {
            T* old_instance = T_instance;
            T_instance = NULL;
            return old_instance;
        }
		return T_instance;
	}
	// destroy must be called once and only once !!
	static void destroy ()
	{
		T* T_instance = instance (true);
		if (NULL != T_instance) { delete T_instance; }
	}
protected:
	singleton_ptr () {}
};

