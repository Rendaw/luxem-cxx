#ifndef luxem_cxx_misc_h
#define luxem_cxx_misc_h

#include <functional>

namespace luxem
{

struct finally
{
	std::function<void(void)> callback;

	finally(std::function<void(void)> &&callback);
	finally(std::function<void(void)> const &callback);
	~finally(void);
};

}

#endif

