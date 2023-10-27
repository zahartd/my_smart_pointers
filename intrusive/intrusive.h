#pragma once

#include <cstddef>  // for std::nullptr_t
#include <utility>  // for std::exchange / std::swap

class SimpleCounter {
public:
    constexpr SimpleCounter() noexcept {};

    SimpleCounter([[maybe_unused]] const SimpleCounter& other) noexcept : SimpleCounter(){};

    SimpleCounter& operator=([[maybe_unused]] const SimpleCounter& other) noexcept {
        SimpleCounter();
        return *this;
    };

    size_t IncRef() {
        return ++count_;
    };

    size_t DecRef() {
        return --count_;
    };

    size_t RefCount() const {
        return count_;
    };

private:
    size_t count_ = 0;
};

struct DefaultDelete {
    template <typename T>
    static void Destroy(T* object) {
        delete object;
    }
};

template <typename Derived, typename Counter, typename Deleter = DefaultDelete>
class RefCounted {
public:
    // Increase reference counter.
    void IncRef() {
        counter_.IncRef();
    };

    // Decrease reference counter.
    // Destroy object using Deleter when the last instance dies.
    void DecRef() {
        counter_.DecRef();
        if (counter_.RefCount() == 0) {
            Deleter::Destroy(static_cast<Derived*>(this));
        }
    };

    // Get current counter value (the number of strong references).
    size_t RefCount() const {
        return counter_.RefCount();
    };

private:
    Counter counter_;
};

template <typename Derived, typename D = DefaultDelete>
using SimpleRefCounted [[maybe_unused]] = RefCounted<Derived, SimpleCounter, D>;

template <typename T>
class IntrusivePtr {
private:
    template <typename Y>
    friend class IntrusivePtr;

    T* ptr_ = nullptr;

public:
    // Constructors
    constexpr IntrusivePtr() noexcept {};

    constexpr IntrusivePtr(std::nullptr_t) noexcept {};

    IntrusivePtr(T* ptr) noexcept : ptr_(ptr) {
        static_assert(!std::is_void<T>::value, "T must be a complete type");
        static_assert(sizeof(T) > 0, "T must be a complete type");
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
    };

    template <typename Y>
    IntrusivePtr(const IntrusivePtr<Y>& other) noexcept : ptr_(other.ptr_) {
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
    }

    template <typename Y>
    IntrusivePtr(IntrusivePtr<Y>&& other) noexcept : ptr_(other.ptr_) {
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
        other.Release();
    }

    IntrusivePtr(const IntrusivePtr& other) noexcept : ptr_(other.ptr_) {
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
    };

    IntrusivePtr(IntrusivePtr&& other) noexcept : ptr_(other.ptr_) {
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
        other.Release();
    };

    // `operator=`-s
    IntrusivePtr& operator=(const IntrusivePtr& other) noexcept {
        if (this->ptr_ == other.ptr_) {
            return *this;
        }
        this->Release();
        ptr_ = other.ptr_;
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
        return *this;
    };

    IntrusivePtr& operator=(IntrusivePtr&& other) noexcept {
        if (this->ptr_ == other.ptr_) {
            return *this;
        }
        this->Release();
        ptr_ = other.ptr_;
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
        other.Release();
        return *this;
    };

    // Destructor
    ~IntrusivePtr() noexcept {
        this->Release();
    };

    // Modifiers
    void Reset() noexcept {
        this->Release();
    };

    void Reset(T* ptr) noexcept {
        this->Release();
        ptr_ = ptr;
        if (ptr_ != nullptr) {
            ptr_->IncRef();
        }
    };

    void Swap(IntrusivePtr& other) noexcept {
        std::swap(ptr_, other.ptr_);
    };

    // Observers
    T* Get() const {
        return ptr_;
    };

    T& operator*() const {
        return *Get();
    };

    T* operator->() const {
        return Get();
    };

    size_t UseCount() const noexcept {
        if (ptr_ != nullptr) {
            return ptr_->RefCount();
        }
        return 0;
    };

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    };

private:
    void Release() noexcept {
        if (ptr_ != nullptr) {
            ptr_->DecRef();
        }
        this->Zeroing();
    }

    void Zeroing() noexcept {
        ptr_ = nullptr;
    };
};

template <typename T, typename... Args>
IntrusivePtr<T> MakeIntrusive(Args&&... args) {
    return IntrusivePtr<T>(new T(std::forward<Args>(args)...));
}
