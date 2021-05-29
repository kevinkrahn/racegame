#pragma once

template <typename T> struct RemoveReference      { typedef T type; };
template <typename T> struct RemoveReference<T&>  { typedef T type; };
template <typename T> struct RemoveReference<T&&> { typedef T type; };

template <typename T>
constexpr typename RemoveReference<T>::type&& move(T&& t)
{
    return static_cast<typename RemoveReference<T>::type&&>(t);
}

template <typename T>
void swap(T& a, T& b)
{
    auto tmp = (T&&)a;
    a = (T&&)b;
    b = (T&&)tmp;
}

template <class T> struct RemoveCV                   { typedef T type; };
template <class T> struct RemoveCV<const T>          { typedef T type; };
template <class T> struct RemoveCV<volatile T>       { typedef T type; };
template <class T> struct RemoveCV<const volatile T> { typedef T type; };
template <class T> struct RemoveConst                { typedef T type; };
template <class T> struct RemoveConst<const T>       { typedef T type; };
template <class T> struct RemoveVolatile             { typedef T type; };
template <class T> struct RemoveVolatile<volatile T> { typedef T type; };

template <typename T, T v>
struct IntegralConstant
{
    static constexpr T value = v;
    using ValueType = T;
    using Type = IntegralConstant;
    constexpr operator ValueType() const noexcept { return value; }
    constexpr ValueType operator()() const noexcept { return value; }
};

using FalseType = IntegralConstant<bool, false>;
using TrueType = IntegralConstant<bool, true>;

template <typename> struct IsIntegralBase :  FalseType {};
template <> struct IsIntegralBase<bool> : TrueType {};
template <> struct IsIntegralBase<unsigned char> : TrueType {};
template <> struct IsIntegralBase<unsigned short> : TrueType {};
template <> struct IsIntegralBase<unsigned int> : TrueType {};
template <> struct IsIntegralBase<unsigned long> : TrueType {};
template <> struct IsIntegralBase<unsigned long long> : TrueType {};
template <> struct IsIntegralBase<char> : TrueType {};
template <> struct IsIntegralBase<short> : TrueType {};
template <> struct IsIntegralBase<int> : TrueType {};
template <> struct IsIntegralBase<long> : TrueType {};
template <> struct IsIntegralBase<long long> : TrueType {};

template <typename T> struct IsIntegral : IsIntegralBase<typename RemoveCV<T>::type> {};

template <typename> struct IsFloatingPointBase :  FalseType {};
template <> struct IsFloatingPointBase<float> : TrueType {};
template <> struct IsFloatingPointBase<double> : TrueType {};

template <typename T> struct IsFloatingPoint : IsFloatingPointBase<typename RemoveCV<T>::type> {};

template <typename T> struct IsArithmetic : IntegralConstant<bool, IsFloatingPoint<T>::value || IsIntegral<T>::value> {};

template <typename T> struct IsArray : FalseType {};
template <typename T> struct IsArray<T[]> : TrueType {};
template <typename T, unsigned long long N> struct IsArray<T[N]> : TrueType {};

template <typename T> struct IsEnum : IntegralConstant<bool, __is_enum(T)> {};

template <typename T, T MIN, T MAX>
struct NumericLimitsBase
{
    static constexpr T min = MIN;
    static constexpr T max = MAX;
};
template <typename T> struct NumericLimits {};
template <> struct NumericLimits<char> : NumericLimitsBase<char, -128, 127> {};
template <> struct NumericLimits<unsigned char> : NumericLimitsBase<unsigned char, 0, 255> {};
template <> struct NumericLimits<short> : NumericLimitsBase<short, -32767-1, 32767> {};
template <> struct NumericLimits<unsigned short> : NumericLimitsBase<unsigned short, 0, 65535> {};
template <> struct NumericLimits<int> : NumericLimitsBase<int, -2147483647-1, 2147483647> {};
template <> struct NumericLimits<unsigned int> : NumericLimitsBase<unsigned int, 0, 4294967295U> {};
#if 0
template <> struct NumericLimits<long> : NumericLimitsBase<long, -9223372036854775807L-1, 9223372036854775807L> {};
template <> struct NumericLimits<unsigned long> : NumericLimitsBase<unsigned long, 0, 18446744073709551615UL> {};
#endif
template <> struct NumericLimits<long long> : NumericLimitsBase<long long, -9223372036854775807LL-1, 9223372036854775807LL> {};
template <> struct NumericLimits<unsigned long long> : NumericLimitsBase<unsigned long long, 0, 18446744073709551615ULL> {};

template <bool B, typename T> struct EnableIf {};
template <typename T> struct EnableIf<true, T> { typedef T type; };

// TODO: decide if this is worth having
template <typename E> struct EnableBitMaskOperators { static const bool enable = false; };
template <typename E> typename EnableIf<EnableBitMaskOperators<E>::enable, E>::type
operator | (E lhs, E rhs) { return (E)((unsigned int)lhs | (unsigned int)rhs); }
template <typename E> typename EnableIf<EnableBitMaskOperators<E>::enable, E>::type&
operator |= (E& lhs, E rhs) { return (E&)((unsigned int&)lhs |= (unsigned int)rhs); }
template <typename E> typename EnableIf<EnableBitMaskOperators<E>::enable, E>::type
operator & (E lhs, E rhs) { return (E)((unsigned int)lhs & (unsigned int)rhs); }
template <typename E> typename EnableIf<EnableBitMaskOperators<E>::enable, E>::type&
operator &= (E& lhs, E rhs) { return (E)((unsigned int&)lhs &= (unsigned int)rhs); }
template <typename E> typename EnableIf<EnableBitMaskOperators<E>::enable, E>::type
operator ~ (E rhs) { return (E)(~(unsigned int)rhs); }

#define BITMASK_OPERATORS(e) template <> struct EnableBitMaskOperators<e> { static const bool enable = true; };\
    bool operator ! (e v) { return v == (e)(0); }

template <typename T> struct IsLValueReference : FalseType {};
template <typename T> struct IsLValueReference <T&> : TrueType {};
template <typename T> constexpr T&& forward(typename RemoveReference<T>::type& t)
{
    return static_cast<T&&>(t);
}
template <typename T> constexpr T&& forward(typename RemoveReference<T>::type&& t)
{
    static_assert(!IsLValueReference<T>::value);
    return static_cast<T&&>(t);
}

template <typename Result, typename... Args>
class FunctionInvokerBase
{
public:
    virtual ~FunctionInvokerBase() {}
    virtual Result invoke(Args... args) const = 0;
};

template <typename F, typename Result, typename... Args>
class FunctionInvoker : public FunctionInvokerBase<Result, Args...>
{
    F f;

public:
    FunctionInvoker(F&& f) : f(move(f)) {}
    Result invoke(Args... args) const override { return f(args...); }
};

template <typename>
class Function;

// TODO: make a version that holds a reference to the invokable object instead of storing it.
// this would be useful for passing callbacks to functions

template <typename Result, typename... Args>
class Function<Result(Args...)>
{
    using InvokerBaseType = FunctionInvokerBase<Result, Args...>;

    char storage[32] = { 0 };

public:
    template <typename F>
    Function(F&& f)
    {
        static_assert(sizeof(F) <= sizeof(storage));
        new (storage) FunctionInvoker<typename RemoveReference<F>::type, Result, Args...>(move(f));
    }

    Function() {}
    Function(Function const& other) = default;
    Function(Function&& other) = default;
    Function& operator = (Function const& other) = default;
    Function& operator = (Function&& other) = default;

    //~Function() { ((InvokerBaseType*)storage)->~InvokerBaseType(); }

    Result operator()(Args... args) const
    {
        return ((InvokerBaseType*)storage)->invoke(forward<Args>(args)...);
    }
};
