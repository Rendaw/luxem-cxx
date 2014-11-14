#include "../luxem.h"
#include <iostream>

int main(void)
{
	auto data = std::make_shared<luxem::array>(luxem::ad{
		std::make_shared<luxem::primitive>(-4),
		std::make_shared<luxem::object>(luxem::od{
			{"key", std::make_shared<luxem::primitive>("value")}
		})
	});

	std::cout << luxem::writer().set_pretty().value(data).dump() << std::endl;

	return 0;
}

