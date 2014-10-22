#include "../read.h"
#include "../write.h"

#include <iostream>
#include <memory>

template <typename type> void assert1(type const &value)
{
	std::cout << "Assert true: " << value << std::endl;
	assert(value);
}

template <typename type> void assert2(type const &got, type const &expected)
{
	std::cout << "Expected: " << expected << std::endl;
	std::cout << "Got     : " << got << std::endl;
	assert(got == expected);
}

void compare_value(luxem::value const *got, luxem::value const *expected)
{
	assert2(got->has_type(), expected->has_type());
	if (got->has_type()) assert2(got->get_type(), expected->get_type());
	if (auto expected_object = dynamic_cast<luxem::object_value const *>(expected))
	{
		auto got_object = dynamic_cast<luxem::object_value const *>(got);
		assert1(got_object);
		assert2(got_object->get_object().size(), expected_object->get_object().size());
		for (auto &expected_pair : expected_object->get_object())
		{
			auto got_pair = got_object->get_object().find(expected_pair.first);
			assert(got_pair != got_object->get_object().end());
			compare_value(got_pair->second.get(), expected_pair.second.get());
		}
	}
	else if (auto expected_array = dynamic_cast<luxem::array_value const *>(expected))
	{
		auto got_array = dynamic_cast<luxem::array_value const *>(got);
		assert1(got_array);
		assert2(got_array->get_array().size(), expected_array->get_array().size());
		for (size_t index = 0; index < expected_array->get_array().size(); ++index)
			compare_value(got_array->get_array()[index].get(), expected_array->get_array()[index].get());
	}
	else if (auto expected_primitive = dynamic_cast<luxem::primitive_value const *>(expected))
	{
		auto got_primitive = dynamic_cast<luxem::primitive_value const *>(got);
		assert1(got_primitive);
		assert2(got_primitive->get_primitive(), expected_primitive->get_primitive());
	}
	else assert(false);
}

int main(void)
{
	luxem::array_value input{luxem::ad{
		std::make_shared<luxem::primitive_value>(-4),
		std::make_shared<luxem::primitive_value>(23u),
		std::make_shared<luxem::primitive_value>(12.7f),
		std::make_shared<luxem::primitive_value>(29.3),
		std::make_shared<luxem::primitive_value>(true),
		std::make_shared<luxem::primitive_value>("hog"),
		std::make_shared<luxem::primitive_value>("int", 4),
		std::make_shared<luxem::array_value>(),
		std::make_shared<luxem::array_value>(luxem::ad{
			std::make_shared<luxem::primitive_value>("programming language")
		}),
		std::make_shared<luxem::array_value>("peanut", luxem::ad{
			std::make_shared<luxem::primitive_value>("equality"),
			std::make_shared<luxem::primitive_value>(999)
		}),
		std::make_shared<luxem::object_value>(),
		std::make_shared<luxem::object_value>(luxem::od{
			{"key", std::make_shared<luxem::primitive_value>("value")}
		}),
		std::make_shared<luxem::object_value>("horse", luxem::od{
			{"equestrianism", std::make_shared<luxem::primitive_value>(true)}
		}),
		std::make_shared<luxem::primitive_value>(luxem::subencodings::ascii16{}, std::vector<uint8_t>{1, 12, 255})
	}};
	compare_value(&input, &input);
	std::shared_ptr<luxem::value> output;
	luxem::reader reader;
	reader.build_struct([&output](std::shared_ptr<luxem::value> &&value) mutable { output = std::move(value); });
	reader.feed(luxem::writer().value(input).dump());
	assert(output);
	compare_value(output.get(), &input);
	return 0;
}

