#include "../luxem.h"

#include <iostream>

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
	reader.feed("4, 12, {\"key\": \"I AM A VALUE TOO\"}, 3");
	std::cout << "numbers:";
	for (auto number : numbers) std::cout << " " << number;
	std::cout << std::endl;
	std::cout << "text: " << text << std::endl;

	return 0;
}

