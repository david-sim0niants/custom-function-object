#include <utility>


template<typename signature_T> class function;


template<typename Return_T, typename... Args_T>
class function<Return_T(Args_T...)>
{
private:
    /**
     * @brief If functor's data size is larger than a void pointer's size (e.g. a lambda with many capturing variables),
     * then the functor member is a pointer to that data. Otherwise it's treated exactly as the functor's data to avoid
     * heap allocations for small functors. In a special case, when the functor is a simple free function, the member
     * functor holds its pointer.
     */
    void *functor = nullptr;
    /**
     * @brief Functor data size. Indicates that if larger than a void pointer's size, then the functor member is a
     * pointer to a heap allocated functor data.
     */
    size_t functor_size = 0;
    /**
     * @brief Pointer to the functor invoker to be called from () operator.
     */
    Return_T (*invoker)(void *functor, Args_T&&...) = nullptr;
    /**
     * @brief Pointer to the functor constructor function.
     */
    void * (*constructor)(void *, const void *) = nullptr;
    /**
     * @brief Pointer to the functor destructor function.
     */
    void (*destructor)(void *) = nullptr;


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
    static Functor_T * construct(Functor_T *dst_functor, const Functor_T *src_functor)
    {
        if (dst_functor)
            return new (dst_functor) Functor_T(*src_functor);
        else return new Functor_T(*src_functor);
    }

    template<typename Functor_T>
    static void destruct(Functor_T *functor)
    {
        delete functor;
    }

public:
    // Default constructor.
    function() = default;

    // Constructor accepting a functor of any type.
    template<typename Functor_T>
    function(const Functor_T &f)
    {
        functor_size = sizeof(Functor_T);
        constructor = reinterpret_cast<decltype(constructor)>(&construct<Functor_T>);
        destructor = reinterpret_cast<decltype(destructor)>(&destruct<Functor_T>);
        if constexpr (sizeof(Functor_T) > sizeof(functor))
        {
            functor = constructor(nullptr, &f);
            invoker = reinterpret_cast<decltype(invoker)>(&invoke_func_heap<Functor_T>);
        }
        else
        {
            constructor(&functor, &f);
            invoker = reinterpret_cast<decltype(invoker)>(&invoke_func<Functor_T>);
        }
    }

    // Copy constructor.
    function(const function &other) :
        functor_size(other.functor_size),
        invoker(other.invoker),
        constructor(other.constructor),
        destructor(other.destructor)
    {
        if (functor_size > sizeof(functor))
        {
            functor = constructor(nullptr, other.functor);
        }
        else
        {
            constructor(&functor, &other.functor);
        }
    }

    // Move constructor.
    function(function &&other) noexcept :
        functor(other.functor),
        functor_size(other.functor_size),
        invoker(other.invoker),
        constructor(other.constructor),
        destructor(other.destructor)
    {
        other.functor = nullptr;
        other.functor_size = 0;
        other.constructor = nullptr;
        other.destructor = nullptr;
    }

    // The meaning behind all of these.
    inline Return_T operator()(Args_T&&... args)
    {
        return invoker(functor, std::forward<Args_T>(args)...);
    }

    // Destructor that will call a function pointed by the desctructor if a functor is allocated in heap.
    ~function()
    {
        if (functor_size > sizeof(functor) && functor)
        {
            destructor(functor);
        }
    }
};
