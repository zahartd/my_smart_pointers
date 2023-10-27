#pragma once

#include <type_traits>
#include <cstddef>
#include <utility>

// Traits for types
// can type T be compressed?
template <typename T>
concept Compressible = std::is_empty_v<T> && !std::is_final_v<T>;

// or not?
template <typename T>
concept NotCompressible = !Compressible<T>;

// Pair element helper class
template <typename T, size_t Index, bool IsCompressible>
class CompressedPairElement;

// Implementation of a pair element to be compressed
// In this case, the element must inherit from type T and expose its interface
template <typename T, size_t Index>
class CompressedPairElement<T, Index, true> : public T {
public:
    constexpr CompressedPairElement() = default;
    template <typename U>
    [[maybe_unused]] constexpr CompressedPairElement(U&& data) : T(std::forward<U>(data)) {
    }

    const T& Get() const {
        return *this;
    }

    T& Get() {
        return *this;
    }
};

// Implementation of a pair element that is not subject to compression
template <typename T, size_t Index>
class CompressedPairElement<T, Index, false> {
public:
    constexpr CompressedPairElement() = default;
    template <typename U>
    [[maybe_unused]] constexpr CompressedPairElement(U&& data) : data_(std::forward<U>(data)) {
    }

    const T& Get() const {
        return data_;
    }

    T& Get() {
        return data_;
    }

protected:
    T data_{};
};

template <typename F, typename S>
class CompressedPair : public CompressedPairElement<F, 0, Compressible<F>>,
                       public CompressedPairElement<S, 1, Compressible<S>> {
protected:
    using First = CompressedPairElement<F, 0, Compressible<F>>;
    using Second = CompressedPairElement<S, 1, Compressible<S>>;

public:
    constexpr CompressedPair() = default;
    template <typename U1, typename U2>
    constexpr CompressedPair(U1&& first, U2&& second)
        : First(std::forward<U1>(first)), Second(std::forward<U2>(second)) {
    }

    F& GetFirst() {
        return First::Get();
    }

    const F& GetFirst() const {
        return First::Get();
    }

    S& GetSecond() {
        return Second::Get();
    }

    const S& GetSecond() const {
        return Second::Get();
    }
};
