#include "../luxem.h"

int main(void)
{
	std::vector<std::shared_ptr<luxem::value>> data;
	luxem::reader reader;
	reader.build_struct([&](std::shared_ptr<luxem::value> &&in_data)
	{
		data.emplace_back(std::move(in_data));
	});
	
	return 0;
}

