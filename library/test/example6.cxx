#include "../luxem.h"

int main(void)
{
	std::vector<int> numbers;
	std::string text;
	luxem::reader reader;
	reader.element([&](std::shared_ptr<luxem::value> &&data)
	{
		if (data->is<luxem::primitive>()) 
			numbers.push_back(data->as<luxem::primitive>().get_int());
		else if (data->is<luxem::reader::object_context>())
			data->as<luxem::reader::object_context>().element(
				"key",
				[&](std::shared_ptr<luxem::value> &&data)
				{
					text = std::move(data->as<luxem::primitive>().get_string());
				}
			);
	});

	return 0;
}

