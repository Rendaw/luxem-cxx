#ifndef luxem_cxx_misc_h
#define luxem_cxx_misc_h

#include <functional>

namespace luxem
{

struct finalize
{
	std::function<void(void)> callback;

	finalize(std::function<void(void)> &&callback);
	finalize(std::function<void(void)> const &callback);
	~finalize(void);
};

}

#endif

