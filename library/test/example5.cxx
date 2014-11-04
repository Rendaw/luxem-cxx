#include "../luxem.h"

int main(void)
{
	auto data = luxem::read_struct("[-4, {key: value}]");
	return 0;
}

