#include "../luxem.h"
#include <iostream>

int main(void)
{
	std::cout << luxem::writer()
		.array_begin()
			.value(-4)
			.object_begin()
				.key("key").value("value")
			.object_end()
		.array_end()
		.dump() << std::endl;
	return 0;
}

