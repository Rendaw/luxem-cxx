#include "read.h"

#include <sstream>
#include <cassert>
#include <stdexcept>

#include <iostream> // DEBUG

static char cxx_error_token;

namespace luxem
{

static luxem_bool_t translate_object_begin(luxem_rawread_context_t *context, void *user_data)
{
	raw_reader &reader = *reinterpret_cast<raw_reader *>(user_data);
	try
	{
		reader.object_begin();
		return true;
	}
	catch (std::exception &e)
	{
		reader.exception_message = e.what();
		luxem_rawread_get_error(context)->pointer = &cxx_error_token;
		return false;
	}
}

static luxem_bool_t translate_object_end(luxem_rawread_context_t *context, void *user_data)
{
	raw_reader &reader = *reinterpret_cast<raw_reader *>(user_data);
	try
	{
		reader.object_end();
		return true;
	}
	catch (std::exception &e)
	{
		reader.exception_message = e.what();
		luxem_rawread_get_error(context)->pointer = &cxx_error_token;
		return false;
	}
}

static luxem_bool_t translate_array_begin(luxem_rawread_context_t *context, void *user_data)
{
	raw_reader &reader = *reinterpret_cast<raw_reader *>(user_data);
	try
	{
		reader.array_begin();
		return true;
	}
	catch (std::exception &e)
	{
		reader.exception_message = e.what();
		luxem_rawread_get_error(context)->pointer = &cxx_error_token;
		return false;
	}
}

static luxem_bool_t translate_array_end(luxem_rawread_context_t *context, void *user_data)
{
	raw_reader &reader = *reinterpret_cast<raw_reader *>(user_data);
	try
	{
		reader.array_end();
		return true;
	}
	catch (std::exception &e)
	{
		reader.exception_message = e.what();
		luxem_rawread_get_error(context)->pointer = &cxx_error_token;
		return false;
	}
}

static luxem_bool_t translate_key(luxem_rawread_context_t *context, void *user_data, luxem_string_t const *data)
{
	raw_reader &reader = *reinterpret_cast<raw_reader *>(user_data);
	try
	{
		reader.key(std::string(data->pointer, data->length));
		return true;
	}
	catch (std::exception &e)
	{
		reader.exception_message = e.what();
		luxem_rawread_get_error(context)->pointer = &cxx_error_token;
		return false;
	}
}

static luxem_bool_t translate_type(luxem_rawread_context_t *context, void *user_data, luxem_string_t const *data)
{
	raw_reader &reader = *reinterpret_cast<raw_reader *>(user_data);
	try
	{
		reader.type(std::string(data->pointer, data->length));
		return true;
	}
	catch (std::exception &e)
	{
		reader.exception_message = e.what();
		luxem_rawread_get_error(context)->pointer = &cxx_error_token;
		return false;
	}
}

static luxem_bool_t translate_primitive(luxem_rawread_context_t *context, void *user_data, luxem_string_t const *data)
{
	raw_reader &reader = *reinterpret_cast<raw_reader *>(user_data);
	try
	{
		reader.primitive(std::string(data->pointer, data->length));
		return true;
	}
	catch (std::exception &e)
	{
		reader.exception_message = e.what();
		luxem_rawread_get_error(context)->pointer = &cxx_error_token;
		return false;
	}
}

raw_reader::raw_reader
(
	std::function<void(void)> object_begin,
	std::function<void(void)> object_end,
	std::function<void(void)> array_begin,
	std::function<void(void)> array_end,
	std::function<void(std::string &&data)> key,
	std::function<void(std::string &&data)> type,
	std::function<void(std::string &&data)> primitive
) : 
	context(luxem_rawread_construct()),
	object_begin(object_begin),
	object_end(object_end),
	array_begin(array_begin),
	array_end(array_end),
	key(key),
	type(type),
	primitive(primitive)
{
	auto callbacks = luxem_rawread_callbacks(context);
	callbacks->object_begin = translate_object_begin;
	callbacks->object_end = translate_object_end;
	callbacks->array_begin = translate_array_begin;
	callbacks->array_end = translate_array_end;
	callbacks->key = translate_key;
	callbacks->type = translate_type;
	callbacks->primitive = translate_primitive;
	callbacks->user_data = this;
}

raw_reader::~raw_reader(void)
	{ luxem_rawread_destroy(context); }

size_t raw_reader::feed(std::string const &data, bool finish)
	{ return feed(data.c_str(), data.length(), finish); }

static void throw_feed_error(raw_reader &reader)
{
	auto message = luxem_rawread_get_error(reader.context);

	std::stringstream combined_message;
	combined_message << "Encountered error at offset " << luxem_rawread_get_position(reader.context) << ": ";
	assert(message->pointer);
	if (message->pointer != &cxx_error_token)
		combined_message.write(message->pointer, message->length);
	else if (!reader.exception_message.empty())
		combined_message << reader.exception_message;
	else combined_message << "Callback provided no error message.";
	throw std::runtime_error(combined_message.str());
}

size_t raw_reader::feed(char const *pointer, size_t length, bool finish)
{
	size_t eaten = 0;
	luxem_string_t temp{pointer, length};
	if (!luxem_rawread_feed(context, &temp, &eaten, finish)) { throw_feed_error(*this); }
	return eaten;
}

void raw_reader::feed(FILE *file)
{
	if (!luxem_rawread_feed_file(context, file, nullptr, nullptr)) { throw_feed_error(*this); }
}

static void build_struct(std::shared_ptr<value> &&data, std::function<void(std::shared_ptr<value> &&data)> &&callback)
{
	if (data->is<reader::object_context>())
	{
		auto object_context = data->as<reader::object_context>();

		std::shared_ptr<object_value> out;
		if (data->has_type()) out = std::make_shared<object_value>(data->get_type(), od{});
		else out = std::make_shared<object_value>();

		object_context.passthrough([out](std::string &&key, std::shared_ptr<value> &&data) 
		{ 
			build_struct(std::move(data), [out, key = std::move(key)](std::shared_ptr<value> &&data)
			{ 
				out->get_object().insert(std::make_pair(std::move(key), std::move(data))); 
			});
		});

		object_context.finished([callback = std::move(callback), out]() mutable
		{ 
			callback(std::move(out)); 
		});
	}
	else if (data->is<reader::array_context>())
	{
		auto array_context = data->as<reader::array_context>();

		std::shared_ptr<array_value> out;
		if (data->has_type()) out = std::make_shared<array_value>(data->get_type(), ad{});
		else out = std::make_shared<array_value>();

		array_context.element([out](std::shared_ptr<value> &&data) 
		{ 
			build_struct(std::move(data), [out](std::shared_ptr<value> &&data)
			{
				out->get_array().emplace_back(std::move(data)); 
			});
		});

		array_context.finished([callback = std::move(callback), out]() mutable 
		{ 
			callback(std::move(out)); 
		});
	}
	else
	{
		assert(data->is<primitive_value>());
		callback(std::move(data));
	}
}

reader::object_context::object_context(object_stackable &base) : base(base) { }

void reader::object_context::element(std::string &&key, std::function<void(std::shared_ptr<value> &&)> &&callback)
{
	assert(base.callbacks.find(key) == base.callbacks.end());
	base.callbacks.emplace(key, callback);
}

void reader::object_context::build_struct(std::string const &key, std::function<void(std::shared_ptr<value> &&data)> &&callback)
{
	assert(base.callbacks.find(key) == base.callbacks.end());
	base.callbacks.emplace(key, [callback = std::move(callback)](std::shared_ptr<value> &&data) mutable
	{ 
		luxem::build_struct(std::move(data), std::move(callback)); 
	});
}

void reader::object_context::passthrough(std::function<void(std::string &&key, std::shared_ptr<value> &&data)> &&callback)
{
	assert(!base.passthrough_callback);
	base.passthrough_callback = std::move(callback);
}

void reader::object_context::finished(std::function<void(void)> &&callback)
{ 
	assert(!base.finish_callback);
	base.finish_callback = callback; 
}

reader::array_context::array_context(array_stackable &base) : base(base) { }

void reader::array_context::element(std::function<void(std::shared_ptr<value> &&data)> &&callback)
{
	assert(!base.callback);
	base.callback = callback;
}

void reader::array_context::build_struct(std::function<void(std::shared_ptr<value> &&data)> &&callback)
{
	assert(!base.callback);
	base.callback = [callback = std::move(callback)](std::shared_ptr<value> &&data) mutable
	{ 
		luxem::build_struct(std::move(data), std::move(callback)); 
	};
}

void reader::array_context::finished(std::function<void(void)> &&callback)
{ 
	assert(!base.finish_callback);
	base.finish_callback = callback; 
}

reader::reader(void) : 
	raw_reader(
		[this]() 
		{
			auto object = std::make_unique<object_stackable>();
			process(std::make_unique<object_context>(*object));
			stack.emplace_back(std::move(object));
		},
		[this]() { pop(); },
		[this]() 
		{
			auto array = std::make_unique<array_stackable>();
			process(std::make_unique<array_context>(*array));
			stack.emplace_back(std::move(array));
		},
		[this]() { pop(); },
		[this](std::string &&data) { has_key = true; current_key = std::move(data); },
		[this](std::string &&data) { has_type = true; current_type = std::move(data); },
		[this](std::string &&data) { process(std::make_shared<primitive_value>(std::move(data))); }
	), 
	has_key(false),
	has_type(false)
{
	stack.emplace_back(std::make_unique<array_stackable>());
}

reader &reader::element(std::function<void(std::shared_ptr<value> &&data)> &&callback)
{
	assert(!stack.empty());
	assert(typeid(*stack.front()) == typeid(array_stackable));
	array_context(*reinterpret_cast<array_stackable *>(stack.front().get())).element(std::move(callback));
	return *this;
}

reader &reader::build_struct(std::function<void(std::shared_ptr<value> &&data)> &&callback)
{
	assert(!stack.empty());
	assert(typeid(*stack.front()) == typeid(array_stackable));
	array_context(*reinterpret_cast<array_stackable *>(stack.front().get())).build_struct(std::move(callback));
	return *this;
}
			
reader::stackable::~stackable(void) {}
		
void reader::object_stackable::process(std::shared_ptr<value> &&data, std::string &&key)
{
	if (passthrough_callback)
	{
		passthrough_callback(std::move(key), std::move(data));
		return;
	}
	auto callback = callbacks.find(key);
	if (callback == callbacks.end())
		return;
	callback->second(std::move(data));
}

void reader::object_stackable::finish(void)
	{ if (finish_callback) finish_callback(); }

void reader::array_stackable::process(std::shared_ptr<value> &&data, std::string &&key)
{
	if (!callback) return;
	callback(std::move(data));
}

void reader::array_stackable::finish(void)
	{ if (finish_callback) finish_callback(); }

void reader::process(std::shared_ptr<value> &&data)
{
	assert(!stack.empty());
	if (stack.empty()) return;
	if (has_type) data->set_type(std::move(current_type));
	stack.back()->process(std::move(data), std::move(current_key));
	has_type = false;
	has_key = false;
}

void reader::pop(void)
{
	stack.back()->finish();
	stack.pop_back();
}

}

