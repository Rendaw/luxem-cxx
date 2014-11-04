#include "../luxem.h"

int main(void)
{
	luxem::array data(luxem::ad{
		std::make_shared<luxem::primitive>(-4),
		std::make_shared<luxem::object>(luxem::od{
			{"key", std::make_shared<luxem::primitive>("value")}
		})
	});

	return 0;
}

