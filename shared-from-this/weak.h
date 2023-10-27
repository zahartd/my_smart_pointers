#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
private:
    template <typename Y>
    friend class SharedPtr;

    template <typename Y>
    friend class WeakPtr;

    T* ptr_ = nullptr;                 // pointer to type
    IControlBlock* cblock_ = nullptr;  // pointer to the control block that owns the object
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    constexpr WeakPtr() noexcept = default;

    WeakPtr(const WeakPtr& other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        if (cblock_ != nullptr) {
            cblock_->AddWeakRef();  // increment weak refs counter
        }
    };

    template <class Y>
    WeakPtr(const WeakPtr<Y>& other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        if (cblock_ != nullptr) {
            cblock_->AddWeakRef();  // increment weak refs counter
        }
    }

    WeakPtr(WeakPtr&& other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        other.Zeroing();
    };

    template <class Y>
    WeakPtr(WeakPtr<Y>&& other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        other.Zeroing();
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        if (cblock_ != nullptr) {
            cblock_->AddWeakRef();  // increment weak refs counter
        }
    };

    template <class Y>
    WeakPtr(const SharedPtr<T>& other) noexcept : ptr_(other.ptr_), cblock_(other.cblock_) {
        if (cblock_ != nullptr) {
            cblock_->AddWeakRef();  // increment weak refs counter
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) noexcept {
        if (this != &other) {
            this->Release();    // release old object
            ptr_ = other.ptr_;  // seizure of shared possession
            cblock_ = other.cblock_;
            if (cblock_ != nullptr) {
                cblock_->AddWeakRef();  // increment weak refs counter
            }
        }
        return *this;
    };

    template <class Y>
    WeakPtr& operator=(const WeakPtr<Y>& other) noexcept {
        if (this != &other) {
            this->Release();    // release old object
            ptr_ = other.ptr_;  // seizure of shared possession
            cblock_ = other.cblock_;
            if (cblock_ != nullptr) {
                cblock_->AddWeakRef();  // increment weak refs counter
            }
        }
        return *this;
    }

    template <class Y>
    WeakPtr& operator=(SharedPtr<Y>& other) noexcept {
        this->Release();    // release old object
        ptr_ = other.ptr_;  // seizure of shared possession
        cblock_ = other.cblock_;
        if (cblock_ != nullptr) {
            cblock_->AddWeakRef();  // increment weak refs counter
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            this->Release();    // release old object
            ptr_ = other.ptr_;  // seizure of shared possession (at this moment it only this)
            cblock_ = other.cblock_;
            other.Zeroing();  // reset tmp rvalue object
        }
        return *this;
    };

    template <class Y>
    WeakPtr& operator=(WeakPtr<Y>&& other) noexcept {
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

    ~WeakPtr() {
        this->Release();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Zeroing() noexcept {
        ptr_ = nullptr;
        cblock_ = nullptr;
    }

    void Release() noexcept {
        if (cblock_ != nullptr) {
            cblock_->RemoveWeakRef();  // decrement weak refs counter
            if (cblock_->IsZeroStrongOwning() && cblock_->IsZeroWeakOwning()) {
                cblock_->Destroy();
            }
        }
        this->Zeroing();
    }

    void Reset() noexcept {
        this->Release();  // "nullptred" this - After the call, *this manages no object
    }

    void Swap(WeakPtr<T>& other) noexcept {
        std::swap(cblock_, other.cblock_);
        std::swap(ptr_, other.ptr_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const noexcept {
        if (cblock_ != nullptr) {
            return cblock_->GetStrongRefsCount();
        }
        return 0;
    };

    bool Expired() const noexcept {
        return UseCount() == 0;
    };

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);

    SharedPtr<T> Lock() const noexcept {
        return Expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
    };
};
