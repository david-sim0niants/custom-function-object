#include <utility>


template<typename signature_T> class function;


template<typename Return_T, typename... Args_T>
class function<Return_T(Args_T...)>
{
private:
    void *functor;
    size_t functor_size = 0;
    Return_T (*invoker)(void *functor, Args_T&&...) = nullptr;
    void * (*constructor)(void *, const void *) = nullptr;
    void * (*destructor)(void *) = nullptr;


    template<typename Functor_T>
    static Return_T invoke_func_heap(void *functor, Args_T&&... args)
    {
        return (*reinterpret_cast<Functor_T *>(functor))(std::forward<Args_T>(args)...);
    }


    template<typename Functor_T>
    static Return_T invoke_func(void *functor, Args_T&&... args)
    {
        return (*reinterpret_cast<Functor_T *>(&functor))(std::forward<Args_T>(args)...);
    }

    template<typename Functor_T>
    static void construct(Functor_T *dst_functor, const Functor_T *src_functor)
    {
        new (dst_functor) Functor_T(*src_functor);
    }

    template<typename Functor_T>
    static void destruct(Functor_T *functor)
    {
        functor->~Functor_T();
    }

public:
    template<typename Functor_T>
    function(const Functor_T &f)
    {
        functor_size = sizeof(Functor_T);
        if constexpr (sizeof(Functor_T) > sizeof(functor))
        {
            functor = new Functor_T(f);
            invoker = reinterpret_cast<decltype(invoker)>(&invoke_func_heap<Functor_T>);
        }
        else
        {
            new (&functor) Functor_T(f);
            invoker = reinterpret_cast<decltype(invoker)>(&invoke_func<Functor_T>);
        }
        constructor = reinterpret_cast<decltype(constructor)>(&construct<Functor_T>);
        destructor = reinterpret_cast<decltype(destructor)>(&destruct<Functor_T>);
    }

    function(const function &other) : 
        functor_size(other.functor_size), 
        invoker(invoker), 
        constructor(constructor), 
        destructor(destructor)
    {
        if (functor_size > sizeof(functor))
        {
            functor = malloc(other.functor_size);
            constructor(functor, other.functor);
        }
        else
        {
            constructor(&functor, &other.functor);
        }
    }

    function(function &&other) noexcept : 
        functor(other.functor), 
        functor_size(other.functor_size), 
        invoker(other.invoker), 
        constructor(other.constructor), 
        destructor(other.destructor)
    {
        other.functor = nullptr;
        other.functor_size = 0;
        other.invoker = nullptr;
        other.constructor = nullptr;
        other.destructor = nullptr;
    }


    inline Return_T operator()(Args_T&&... args)
    {
        return invoker(functor, std::forward<Args_T>(args)...);
    }

    ~function()
    {
        if (functor_size > sizeof(functor))
            destructor(functor);
    }
};
