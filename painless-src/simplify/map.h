#ifndef _map_h_INCLUDED
#define _map_h_INCLUDED

#include "svec.h"
#include <cstdio>
#include <cstring>
#include <fstream>

typedef long long ll;

static const int nprimes          = 25;
static const int primes [nprimes] = { 31, 73, 151, 313, 643, 1291, 2593, 5233, 10501, 21013, 42073, 84181, 168451, 337219, 674701, 1349473, 2699299, 5398891, 10798093, 21596719, 43193641, 86387383, 172775299, 345550609, 691101253 };

template<class K, class D>
class Map {
 public:
    struct Pair { K key; D data; };

 private:

    svec<Pair>* table;
    int        cap;
    int        size;

    bool    checkCap(int new_size) const { return new_size > cap; }
    int index  (const K& k) const { return 1ll * k % cap; }
    void   _insert (const K& k, const D& d) { 
        svec<Pair>& ps = table[index(k)];
        ps.push(); ps.last().key = k; ps.last().data = d; }

    void    rehash () {
        const svec<Pair>* old = table;

        int old_cap = cap;
        int newsize = primes[0];
        for (int i = 1; newsize <= cap && i < nprimes; i++)
           newsize = primes[i];

        table = new svec<Pair>[newsize];
        cap   = newsize;

        for (int i = 0; i < old_cap; i++){
            for (int j = 0; j < old[i].size(); j++){
                _insert(old[i][j].key, old[i][j].data); }}

        delete [] old;
    }

    
 public:

    Map () : table(NULL), cap(0), size(0) {}
    ~Map () { delete [] table; }

    void insert (const K& k, const D& d) { 
        if (size != 0) {
            svec<Pair>& ps = table[index(k)];
            for (int i = 0; i < ps.size(); i++)
                if (ps[i].key == k) {
                    ps[i].data = d;
                    return;
                }
        }
        if (checkCap(size+1)) rehash(); _insert(k, d); size++; 
    }
    
    D get (const K& k, D d) const {
        if (size == 0) return false;
        const svec<Pair>& ps = table[index(k)];
        for (int i = 0; i < ps.size(); i++)
            if (ps[i].key == k)
                return ps[i].data;
        return d;
    }

    void remove(const K& k) {
        assert(table != NULL);
        svec<Pair>& ps = table[index(k)];
        int j = 0;
        for (; j < ps.size() && ps[j].key != k; j++);
        assert(j < ps.size());
        ps[j] = ps.last();
        ps.pop();
        size--;
    }

    D get_and_remove(const K& k) {
        assert(table != NULL);
        svec<Pair>& ps = table[index(k)];
        int j = 0;
        for (; j < ps.size() && ps[j].key != k; j++);
        D s = ps[j].data;
        assert(j < ps.size());
        ps[j] = ps.last();
        ps.pop();
        size--;
        return s;
    }

    void clear  () {
        cap = size = 0;
        delete [] table;
        table = NULL;
    }

    int  elems() const { return size; }
    int  bucket_count() const { return cap; }

};


#endif
