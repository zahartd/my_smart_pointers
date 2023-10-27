#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <cstddef>   // std::nullptr_t
#include <type_traits>
#include <memory>

class ESFTBase {
};

template<typename T>
class EnableSharedFromThis;

class IControlBlock {
private:
    size_t strong_ref_counter_ = 0;
    size_t weak_ref_counter_ = 0;

public:
    size_t GetStrongRefsCount() const noexcept {
        return strong_ref_counter_;
    }

    void AddStrongRef() noexcept {
        ++strong_ref_counter_;
    }

    void RemoveStrongRef() noexcept {
        --strong_ref_counter_;
    }

    bool IsZeroStrongOwning() const noexcept {
        return strong_ref_counter_ == 0;
    }

    [[maybe_unused]] size_t GetWeakRefsCount() const noexcept {
        return weak_ref_counter_;
    }

    void AddWeakRef() noexcept {
        ++weak_ref_counter_;
    }

    void RemoveWeakRef() noexcept {
        --weak_ref_counter_;
    }

    bool IsZeroWeakOwning() const noexcept {
        return weak_ref_counter_ == 0;
    }

    void Destroy() noexcept {  // for total destroy control block
        delete this;
    }

    virtual void Dispose() = 0;  // for free memory

    virtual ~IControlBlock() = default;
};

template<class T>
class ControlBlockPtr final : public IControlBlock {
private:
    T *ptr_ = nullptr;

public:
    constexpr ControlBlockPtr() = default;  // same as ControlBlockPtr(nullptr)

    explicit ControlBlockPtr(T *ptr) noexcept: ptr_(ptr) {
        if (ptr_ != nullptr) {
            IControlBlock::AddStrongRef();
        }
    }

    void Dispose() noexcept override {
        delete ptr_;
    }

    ~ControlBlockPtr() noexcept override {
    }
};

template<class T>
class ControlBlockHolder final : public IControlBlock {
private:
    std::aligned_storage_t<sizeof(T), alignof(T)> storage_;

public:
    template<class... Args>
    ControlBlockHolder(Args &&... args) {
        new(&storage_) T(std::forward<Args>(args)...);  // NO_LINT
        IControlBlock::AddStrongRef();
    }

    T *GetPtr() {
        return reinterpret_cast<T *>(&storage_);
    }

    void Dispose() noexcept override {
        std::destroy_at(std::launder(GetPtr()));
    }

    ~ControlBlockHolder() noexcept override {
    }
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template<typename T>
class SharedPtr {
private:
    template<typename Y>
    friend
    class SharedPtr;

    template<typename Y>
    friend
    class WeakPtr;

    T *ptr_ = nullptr;                 // pointer to type
    IControlBlock *cblock_ = nullptr;  // pointer to the control block that owns the object

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    constexpr SharedPtr() noexcept {
    }

    constexpr SharedPtr(std::nullptr_t) noexcept {
    }

    template<class Y>
    explicit SharedPtr(Y *ptr) : ptr_(ptr), cblock_(new ControlBlockPtr<Y>(ptr)) {
        static_assert(!std::is_void<Y>::value,
                      "Y must be a complete type");  // from ccpreference.com
        static_assert(sizeof(Y) > 0, "Y must be a complete type");
        if constexpr (std::is_convertible_v<Y *, ESFTBase *>) {
            ptr->weak_this_ = *this;
        }
    }

    SharedPtr(const SharedPtr<T> &other) noexcept: ptr_(other.ptr_), cblock_(other.cblock_) {
        if (cblock_ != nullptr) {
            cblock_->AddStrongRef();  // increment strong refs counter
        }
        if constexpr (std::is_convertible_v<T *, ESFTBase *>) {
            other.ptr_->weak_this_ = *this;
        }
    }

    template<class Y>
    SharedPtr(const SharedPtr<Y> &other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        if (cblock_ != nullptr) {
            cblock_->AddStrongRef();  // increment strong refs counter
        }
        if constexpr (std::is_convertible_v<Y *, ESFTBase *>) {
            other.ptr_->weak_this_ = *this;
        }
    }

    template<class Y>
    SharedPtr(SharedPtr<Y> &&other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        if constexpr (std::is_convertible_v<Y *, ESFTBase *>) {
            other.ptr_->weak_this_ = *this;
        }
        other.Zeroing();
    }

    // MakeShared ctor
    template<class Y>
    SharedPtr(ControlBlockHolder<Y> *cblock_holder) noexcept
            : ptr_(cblock_holder->GetPtr()), cblock_(cblock_holder) {
        if constexpr (std::is_convertible_v<Y *, ESFTBase *>) {
            ptr_->weak_this_ = *this;
        }
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template<typename Y>
    SharedPtr(const SharedPtr<Y> &other, T *ptr) noexcept : ptr_(ptr), cblock_(other.cblock_) {
        if (cblock_ != nullptr) {
            cblock_->AddStrongRef();  // // increment strong refs counter
        }
    }

    template<typename Y>
    SharedPtr(const SharedPtr<Y> &&other, T *ptr) noexcept : ptr_(ptr), cblock_(other.cblock_) {
        other.Zeroing();
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T> &other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        cblock_ = other.cblock_;
        ptr_ = other.ptr_;
        cblock_->AddStrongRef();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr &operator=(const SharedPtr<T> &other) noexcept {
        if (this != &other) {
            this->Release();    // release old object
            ptr_ = other.ptr_;  // seizure of shared possession
            cblock_ = other.cblock_;
            if (cblock_ != nullptr) {
                cblock_->AddStrongRef();  // increment counter
            }
        }
        return *this;
    }

    SharedPtr &operator=(SharedPtr<T> &&other) noexcept {
        if (this != &other) {
            this->Release();    // release old object
            ptr_ = other.ptr_;  // seizure of shared possession (at this moment it only this)
            cblock_ = other.cblock_;
            other.Zeroing();  // reset tmp rvalue object
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        this->Release();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Zeroing() noexcept {
        ptr_ = nullptr;
        cblock_ = nullptr;
    }

    void Reset() noexcept {
        this->Release();  // "nullptred" this - After the call, *this manages no object
    }

    template<class Y>
    void Reset(Y *ptr) {
        this->Release();
        ptr_ = ptr;
        cblock_ = new ControlBlockPtr<Y>(ptr);
    }

    void Swap(SharedPtr<T> &other) noexcept {
        std::swap(cblock_, other.cblock_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T *Get() const noexcept {
        return ptr_;
    }

    T &operator*() const noexcept {
        return *Get();
    }

    T *operator->() const noexcept {
        return Get();
    }

    size_t UseCount() const noexcept {
        if (cblock_ != nullptr) {
            return cblock_->GetStrongRefsCount();
        }
        return 0;
    }

    explicit operator bool() const noexcept {
        return Get() != nullptr;
    }

private:
    void Release() noexcept {
        if (cblock_ != nullptr) {
            if constexpr (std::is_convertible_v<T *, ESFTBase *>) {
                if (!ptr_->weak_this_.Expired()) {
                    ptr_->weak_this_.Reset();
                }
            }
            cblock_->RemoveStrongRef();  // decrement strong refs counter
            if (cblock_->IsZeroStrongOwning()) {
                cblock_->Dispose();
            }
            if (cblock_->IsZeroStrongOwning() && cblock_->IsZeroWeakOwning()) {
                cblock_->Destroy();
            }
        }
        this->Zeroing();
    }
};

template<class T, class U>
inline bool operator==(const SharedPtr<T> &lhs, const SharedPtr<U> &rhs) noexcept {
    return lhs.Get() == rhs.Get();
}

// Allocate memory only once
template<class T, class... Args>
SharedPtr<T> MakeShared(Args &&... args) {
    return SharedPtr<T>(new ControlBlockHolder<T>(std::forward<Args>(args)...));
}

// Look for usage examples in tests
template<class T>
class EnableSharedFromThis : public ESFTBase {
private:
    template<typename Y>
    friend
    class SharedPtr;

    WeakPtr<T> weak_this_;  // special weak pointer to itself
public:
    SharedPtr<T> SharedFromThis() {
        return weak_this_.Lock();
    };

    [[maybe_unused]] SharedPtr<const T> SharedFromThis() const {
        return weak_this_.Lock();
    };

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this_;
    };

    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this_;
    };
};
