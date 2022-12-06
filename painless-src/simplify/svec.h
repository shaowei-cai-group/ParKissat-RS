#ifndef _vec_hpp_INCLUDED
#define _vec_hpp_INCLUDED

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstddef>

template<class T>
class svec {
     static inline int  imax   (int x, int y) { int mask = (y-x) >> (sizeof(int)*8-1); return (x&mask) + (y&(~mask)); }
    static inline void nextCap(int &cap) { cap += ((cap >> 1) + 2) & ~1; }

    public:
        T* data;
        int sz, cap;
        svec()                   :  data(NULL), sz(0), cap(0) {}
        svec(int size, const T& pad) : data(NULL) , sz(0)   , cap(0)    { growTo(size, pad); }
        explicit svec(int size)  :  data(NULL), sz(0), cap(0) { growTo(size); }
        ~svec()                                               { clear(true); }

        operator T*      (void)         { return data; }
        
        int     size     (void) const   { return sz;   }
        int     capacity (void) const   { return cap;  }
        void    capacity (int min_cap);

        void    setsize  (int v)        { sz = v;} 
        void    push  (void)            { if (sz == cap) capacity(sz+1); new (&data[sz]) T(); sz++; }
        void    push   (const T& elem)  { if (sz == cap) capInc(sz + 1); data[sz++] = elem; }
        void    push_  (const T& elem)  { assert(sz < cap); data[sz++] = elem; }
        void    pop    (void)           { assert(sz > 0), sz--, data[sz].~T(); } 
        void    copyTo (svec<T>& copy)   { copy.clear(); copy.growTo(sz); for (int i = 0; i < sz; i++) copy[i] = data[i]; }
        
        void    growTo   (int size);
        void    growTo   (int size, const T& pad);
        void    clear    (bool dealloc = false);
        void    capInc   (int to_cap);


        T&       operator [] (int index)       { return data[index]; }
        const T& operator [] (int index) const { return data[index]; }

        T&       last        (void)            { return data[sz - 1]; }
        const T& last        (void)      const { return data[sz - 1]; }
                    
};


class OutOfMemoryException{};

template<class T>
void svec<T>::clear(bool dealloc) {
    if (data != NULL) {
        sz = 0;
        if (dealloc) free(data), data = NULL, cap = 0;
    }
}

template<class T>
void svec<T>::capInc(int to_cap) {
    if (cap >= to_cap) return;
    int add = imax((to_cap - cap + 1) & ~1, ((cap >> 1) + 2) & ~1); 
    if (add > __INT_MAX__ - cap || ((data = (T*)::realloc(data, (cap += add) * sizeof(T))) == NULL) && errno == ENOMEM)
        throw OutOfMemoryException();
}

template<class T>
void svec<T>::capacity(int min_cap) {
    if (cap >= min_cap) return;
    int add = imax((min_cap - cap + 1) & ~1, ((cap >> 1) + 2) & ~1);   // NOTE: grow by approximately 3/2
    if (add > __INT_MAX__ - cap || ((data = (T*)::realloc(data, (cap += add) * sizeof(T))) == NULL) && errno == ENOMEM)
        throw OutOfMemoryException();
 }

template<class T>
void svec<T>::growTo(int size) {
    if (sz >= size) return;
    capInc(size);
    for (int i = 0; i < sz; i++) new (&data[i]) T();
    sz = size;
}

template<class T>
void svec<T>::growTo(int size, const T& pad) {
    if (sz >= size) return;
    capacity(size);
    for (int i = sz; i < size; i++) data[i] = pad;
    sz = size; }


#endif