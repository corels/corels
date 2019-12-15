#pragma once
#include "utils.h"
#include <memory>
#include <typeinfo>

#if defined(TRACK_ALLOC)
template <class T, DataStruct S>
struct track_alloc {
    DataStruct data_struct = S;
    typedef T value_type;
    track_alloc() noexcept {}
    template <class U> track_alloc (const track_alloc<U, S>&) noexcept {}
    template <class U> struct rebind { typedef track_alloc<U, S> other; };
    T* allocate (size_t n) {
        logger->addToMemory(sizeof(T) * n, data_struct);
        return static_cast<T*>(malloc(n*sizeof(T)));
    }
    void deallocate (T* p, size_t n) {
        logger->removeFromMemory(n * sizeof(*p), data_struct);
        free(p);
    }
};

template <class T, class U, DataStruct S, DataStruct V>
bool operator==(const track_alloc<T, S>& t1, const track_alloc<U, V>& t2) {
    return (S == V) && (typeid(T) == typeid(U));
}
template <class T, class U, DataStruct S, DataStruct V>
bool operator!=(const track_alloc<T, S>& t1, const track_alloc<U, V>& t2) {
    return !(S == V) || !(typeid(T) == typeid(U));
}
template<class T, DataStruct S>
using tracking_vector = std::vector<T, track_alloc<T, S> >;

#else
template<class T, DataStruct S>
using tracking_vector = std::vector<T, std::allocator<T> >;
#endif
