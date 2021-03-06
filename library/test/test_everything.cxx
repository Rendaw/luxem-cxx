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
	if (expected.is<luxem::object>())
	{
		auto &expected_object = expected.as<luxem::object>();
		auto &got_object = got.as<luxem::object>();
		assert2(got_object.get_data().size(), expected_object.get_data().size());
		for (auto &expected_pair : expected_object.get_data())
		{
			auto got_pair = got_object.get_data().find(expected_pair.first);
			assert(got_pair != got_object.get_data().end());
			compare_value(*got_pair->second, *expected_pair.second);
		}
	}
	else if (expected.is<luxem::array>())
	{
		auto &expected_array = expected.as<luxem::array>();
		auto &got_array = got.as<luxem::array>();
		assert2(got_array.get_data().size(), expected_array.get_data().size());
		for (size_t index = 0; index < expected_array.get_data().size(); ++index)
			compare_value(*got_array.get_data()[index], *expected_array.get_data()[index]);
	}
	else if (expected.is<luxem::primitive>())
	{
		auto &expected_primitive = expected.as<luxem::primitive>();
		auto &got_primitive = got.as<luxem::primitive>();
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
			{ assert2(data->as<luxem::primitive>().get_bool(), true); done = true; });
		reader.feed("true");
		assert(done);
	}
	
	{
		bool done = false;
		luxem::reader reader;
		reader.element([&done](std::shared_ptr<luxem::value> &&data) 
			{ assert2(data->as<luxem::primitive>().get_bool(), false); done = true; });
		reader.feed("false");
		assert(done);
	}
	
	{
		bool done = false;
		luxem::reader reader;
		reader.element([&done](std::shared_ptr<luxem::value> &&data) 
			{ compare_value(*data, luxem::primitive{"int", 99}); done = true; });
		reader.feed("(int)99,");
		assert(done);
	}

	{
		try
		{
			luxem::reader reader;
			reader.element([](std::shared_ptr<luxem::value> &&data) 
			{ 
				data->as<luxem::reader::object_context>().build_struct(
					"a", 
					[](std::shared_ptr<luxem::value> &&) {});
			});
			reader.feed("{a: {}, a: {}}");
			assert(false);
		}
		catch (...) {}
	}
	
	{
		try
		{
			luxem::reader reader;
			reader.build_struct([](std::shared_ptr<luxem::value> &&data) {});
			reader.feed("{}, {}");
		}
		catch (...) { assert(false); }
	}
	
	{
		try
		{
			luxem::reader reader;
			reader.feed("{key: bet_not}");
			assert(false);
		}
		catch (...) {}
	}
	
	{
		try
		{
			luxem::reader reader(false);
			reader.feed("{key: bet_not}");
		}
		catch (...) { assert(false); }
	}

	auto input = std::make_shared<luxem::array>(luxem::ad{
		std::make_shared<luxem::primitive>(-4),
		std::make_shared<luxem::primitive>(23u),
		std::make_shared<luxem::primitive>(12.7f),
		std::make_shared<luxem::primitive>(29.3),
		std::make_shared<luxem::primitive>(true),
		std::make_shared<luxem::primitive>("hog"),
		std::make_shared<luxem::primitive>("int", 4),
		std::make_shared<luxem::array>(),
		std::make_shared<luxem::array>(luxem::ad{
			std::make_shared<luxem::primitive>("programming language")
		}),
		std::make_shared<luxem::array>("peanut", luxem::ad{
			std::make_shared<luxem::primitive>("equality"),
			std::make_shared<luxem::primitive>(999)
		}),
		std::make_shared<luxem::object>(),
		std::make_shared<luxem::object>(luxem::od{
			{"key", std::make_shared<luxem::primitive>("value")}
		}),
		std::make_shared<luxem::object>("horse", luxem::od{
			{"equestrianism", std::make_shared<luxem::primitive>(true)}
		}),
		std::make_shared<luxem::primitive>(luxem::subencodings::ascii16{}, std::vector<uint8_t>{1, 12, 255})
	});
	compare_value(*input, *input);
	std::shared_ptr<luxem::value> output;
	luxem::reader reader;
	reader.build_struct([&output](std::shared_ptr<luxem::value> &&value) mutable { output = std::move(value); });
	reader.feed(luxem::writer().value(input).dump());
	assert(output);
	std::cout << "input: " << luxem::writer().value(input).dump() << std::endl;
	std::cout << "output: " << luxem::writer().value(output).dump() << std::endl;
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

