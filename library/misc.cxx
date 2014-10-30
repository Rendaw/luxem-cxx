#include "misc.h"

namespace luxem
{

finalize::finalize(std::function<void(void)> &&callback) : callback(callback) {}

finalize::finalize(std::function<void(void)> const &callback) : callback(callback) {}

finalize::~finalize(void) { callback(); }

}

