#pragma once

#include "compressed_pair.h"

#include <cstddef>      // std::nullptr_t
#include <type_traits>  // std::nullptr_t

template <class T>
struct [[maybe_unused]] Slug {
    constexpr Slug() noexcept = default;

    template <class Up>
        requires std::is_base_of_v<T, Up>
    [[maybe_unused]] Slug(Slug<Up>&&) noexcept {
    }

    void operator()(T* ptr) const noexcept {
        static_assert(!std::is_void<T>::value, "can't delete pointer to incomplete type");
        static_assert(sizeof(T) > 0, "can't delete pointer to incomplete type");
        delete ptr;
    }
};

template <class T>
struct [[maybe_unused]] Slug<T[]> {
    constexpr Slug() noexcept = default;

    template <class Up>
        requires std::is_base_of_v<T, Up>
    [[maybe_unused]] Slug(Slug<Up>&&) noexcept {
    }

    void operator()(T* ptr) const noexcept {
        static_assert(sizeof(T) > 0, "can't delete pointer to incomplete type");
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = Slug<T>>
class UniquePtr {
private:
    CompressedPair<T*, Deleter> data_;  // First = T* ptr_, Second = Deleter deleter

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) noexcept : data_(ptr, Deleter()){};

    UniquePtr(T* ptr, Deleter deleter) noexcept
        : data_(ptr, std::forward<decltype(deleter)>(deleter)){};

    template <class U, class D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept
        : data_(other.Release(), std::forward<D>(other.GetDeleter())) {
    }

    // Visibly delete copy constructor
    UniquePtr(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    // Visibly delete copy constructor
    UniquePtr& operator=(UniquePtr& other) = delete;

    template <class U, class D>
    UniquePtr& operator=(UniquePtr<U, D>&& other) noexcept {
        Reset(other.Release());
        GetDeleter() = std::forward<D>(other.GetDeleter());
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() noexcept {
        auto ptr = Get();
        if (ptr != nullptr) {
            GetDeleter()(ptr);  // no throw
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() noexcept {
        auto current_ptr = Get();
        data_.GetFirst() = nullptr;
        return current_ptr;
    }

    void Reset(T* ptr = nullptr) noexcept {
        const auto old_ptr = Get();
        data_.GetFirst() = ptr;
        if (old_ptr != nullptr) {
            GetDeleter()(old_ptr);
        }
    }

    void Swap(UniquePtr& other) noexcept {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.data_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const noexcept {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() noexcept {
        return data_.GetSecond();
    }

    const Deleter& GetDeleter() const noexcept {
        return data_.GetSecond();
    }

    explicit operator bool() const noexcept {
        return Get() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    typename std::add_lvalue_reference<T>::type operator*() const noexcept {
        return *Get();
    }

    T* operator->() const noexcept {
        return Get();
    }
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
private:
    CompressedPair<T*, Deleter> data_;  // First = T* ptr_, Second = Deleter deleter

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) noexcept : data_(ptr, Deleter()){};

    UniquePtr(T* ptr, Deleter deleter) noexcept
        : data_(ptr, std::forward<decltype(deleter)>(deleter)){};

    template <class U, class D>
    UniquePtr(UniquePtr<U, D>&& other) noexcept
        : data_(other.Release(), std::forward<D>(other.GetDeleter())) {
    }

    // Visibly delete copy constructor
    UniquePtr(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    // Visibly delete copy constructor
    UniquePtr& operator=(UniquePtr& other) = delete;

    template <class U, class D>
    UniquePtr& operator=(UniquePtr<U, D>&& other) noexcept {
        Reset(other.Release());
        GetDeleter() = std::forward<D>(other.GetDeleter());
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() noexcept {
        auto ptr = Get();
        if (ptr != nullptr) {
            GetDeleter()(ptr);  // no throw
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() noexcept {
        auto current_ptr = Get();
        data_.GetFirst() = nullptr;
        return current_ptr;
    }

    void Reset(T* ptr = nullptr) noexcept {
        const auto old_ptr = Get();
        data_.GetFirst() = ptr;
        if (old_ptr != nullptr) {
            GetDeleter()(old_ptr);
        }
    }

    void Swap(UniquePtr& other) noexcept {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.data_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const noexcept {
        return data_.GetFirst();
    }

    Deleter& GetDeleter() noexcept {
        return data_.GetSecond();
    }

    const Deleter& GetDeleter() const noexcept {
        return data_.GetSecond();
    }

    explicit operator bool() const noexcept {
        return Get() != nullptr;
    }

    typename std::add_lvalue_reference<T>::type operator[](size_t i) const noexcept {
        return Get()[i];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    typename std::add_lvalue_reference<T>::type operator*() const noexcept {
        return *Get();
    }

    T* operator->() const noexcept {
        return Get();
    }
};
