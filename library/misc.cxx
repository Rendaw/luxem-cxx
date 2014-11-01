#include "misc.h"

namespace luxem
{

finally::finally(std::function<void(void)> &&callback) : callback(callback) {}

finally::finally(std::function<void(void)> const &callback) : callback(callback) {}

finally::~finally(void) { callback(); }

}

