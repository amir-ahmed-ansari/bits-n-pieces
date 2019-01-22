// Based on https://hackernoon.com/experimenting-with-small-buffer-optimization-for-c-lambdas-d5b703fb47e4
// but without small buffer optimization.

// NOTE: This code is solely for the purpose of exposition. It doesn't currently work as intended.

#include <exception>
#include <functional>
#include <type_traits>
#include <utility>

namespace std {

template <typename ...>
class erased_function;

template <typename R, typename ...Args>
class erased_function<R(Args...)>
{
public:

    virtual ~erased_function() = default;

    virtual R operator()(Args...) const = 0;

    virtual R operator()(Args...) = 0;
};

namespace detail {

class call_destructor
{
public:

	template <typename T>
	call_destructor(T& t)
	{
		if (mInitialized)
		{
			t.~T();
		}
		mInitialized = true;
	}
	
	~call_destructor()
	{
		mInitialized = false;
	}
	
private:

	bool mInitialized{false};
};

}

template <typename T, typename R, typename ...Args>
class typed_function final
	: public erased_function<R(Args...)>
	, private detail::call_destructor
{
public:

    ~typed_function() override = default;
	
	typed_function(const T& t) = delete;

    explicit typed_function(T&& t)
	: call_destructor{mCallable}
	, mCallable{std::move(t)}
    {
        static_assert(std::is_invocable_r_v<R, T, Args...>);
    }
	
	auto& base()
	{
		return static_cast<erased_function<R(Args...)>&>(*this);
	}

    R operator()(Args... args) const override
    {
        return mCallable(std::forward<Args>(args)...);
    }

    R operator()(Args... args) override
    {
        return mCallable(std::forward<Args>(args)...);
    }

private:

	T mCallable;
};

template <typename R, typename ...Args, typename T>
auto make_typed_function(T&& t)
{
	return std::typed_function<T, R, Args...>{std::forward<T>(t)};
}

template <typename ...T>
class erased_function_container;

// NOTE: Change ownership semantics to suit your purpose. This implementation assumes
// the lifetime of wrapped object is either longer, or the object is destroyed and
// in-place initialized on the same thread any number of times.
template <typename R, typename ...Args>
class erased_function_container<R(Args...)>
{
    using wrappedT = erased_function<R(Args...)>;

public:

    erased_function_container() = default;
	
    constexpr explicit erased_function_container(wrappedT& p)
    : mP{&p}
    {
    }
	
    constexpr explicit erased_function_container(wrappedT* p)
    : mP{p}
    {
    }

    constexpr R operator()(Args ...args)
    {
        if (mP == nullptr)
        {
            throw std::runtime_error{"Cannot invoke nullptr"};
        }
    
        return (*mP)(std::forward<Args>(args)...);
    }

private:

    wrappedT* mP{nullptr};
};

}

class Server
{
public:

	using AsyncHandler = std::erased_function_container<void(int)>;
	void AsyncMethod(float f, AsyncHandler h) {}
};

// Proposed keyword
#define class_local
class Client
{
public:

	void UseServer(Server& s)
	{
		int i = 1;
		double d = 2.0;
		// l is now an unnamed member of Client. Its size is included when computing the size of Client.
		class_local auto l = std::make_typed_function<void, int>([&i, d](int) {});
		s.AsyncMethod(.2f, std::erased_function_container<void(int)>{l.base()});
	}
};

int main()
{
	Server s;
	Client c;
	c.UseServer(s);

    return 0;
}
