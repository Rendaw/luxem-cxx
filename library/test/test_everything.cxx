#undef NDEBUG

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

void compare_value(luxem::value const &got, luxem::value const &expected)
{
	assert2(got.has_type(), expected.has_type());
	if (got.has_type()) assert2(got.get_type(), expected.get_type());
	if (expected.is<luxem::object_value>())
	{
		auto expected_object = expected.as<luxem::object_value>();
		auto got_object = got.as<luxem::object_value>();
		assert2(got_object.get_data().size(), expected_object.get_data().size());
		for (auto &expected_pair : expected_object.get_data())
		{
			auto got_pair = got_object.get_data().find(expected_pair.first);
			assert(got_pair != got_object.get_data().end());
			compare_value(*got_pair->second, *expected_pair.second);
		}
	}
	else if (expected.is<luxem::array_value>())
	{
		auto expected_array = expected.as<luxem::array_value>();
		auto got_array = got.as<luxem::array_value>();
		assert2(got_array.get_data().size(), expected_array.get_data().size());
		for (size_t index = 0; index < expected_array.get_data().size(); ++index)
			compare_value(*got_array.get_data()[index], *expected_array.get_data()[index]);
	}
	else if (expected.is<luxem::primitive_value>())
	{
		auto expected_primitive = expected.as<luxem::primitive_value>();
		auto got_primitive = got.as<luxem::primitive_value>();
		assert2(got_primitive.get_primitive(), expected_primitive.get_primitive());
	}
	else assert(false);
}

int main(void)
{
	assert2(luxem::writer().value(-4).dump(), std::string("-4,"));
	assert2(luxem::writer().value(4u).dump(), std::string("4,"));
	assert2(luxem::writer().value(4.7).dump(), std::string("4.7,"));
	assert2(luxem::writer().value(4.7f).dump(), std::string("4.7,"));
	assert2(luxem::writer().value(false).dump(), std::string("false,"));
	assert2(luxem::writer().value(true).dump(), std::string("true,"));
	assert2(luxem::writer().value("hi").dump(), std::string("hi,"));
	assert2(luxem::writer().value("int", 99).dump(), std::string("(int)99,"));

	{
		bool done = false;
		luxem::reader reader;
		reader.element([&done](std::shared_ptr<luxem::value> &&data) 
			{ assert2(data->as<luxem::primitive_value>().get_bool(), true); done = true; });
		reader.feed("true");
		assert(done);
	}
	
	{
		bool done = false;
		luxem::reader reader;
		reader.element([&done](std::shared_ptr<luxem::value> &&data) 
			{ assert2(data->as<luxem::primitive_value>().get_bool(), false); done = true; });
		reader.feed("false");
		assert(done);
	}
	
	{
		bool done = false;
		luxem::reader reader;
		reader.element([&done](std::shared_ptr<luxem::value> &&data) 
			{ compare_value(*data, luxem::primitive_value{"int", 99}); done = true; });
		reader.feed("(int)99,");
		assert(done);
	}

	auto input = std::make_shared<luxem::array_value>(luxem::ad{
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
	});
	compare_value(*input, *input);
	std::shared_ptr<luxem::value> output;
	luxem::reader reader;
	reader.build_struct([&output](std::shared_ptr<luxem::value> &&value) mutable { output = std::move(value); });
	reader.feed(luxem::writer().value(*input).dump());
	assert(output);
	compare_value(*output, *input);

	{
		size_t walk_count = 0;
		std::shared_ptr<luxem::value> mutable_root(input);
		luxem::walk(mutable_root, [&walk_count](std::string const &, std::shared_ptr<luxem::value> &value) { ++walk_count; });
		luxem::walk(input, [&walk_count](std::string const &, std::shared_ptr<luxem::value> const &value) { ++walk_count; });
		assert(walk_count == 20 * 2);
	}

	return 0;
}

