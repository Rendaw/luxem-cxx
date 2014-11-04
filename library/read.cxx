#include "read.h"

#include <sstream>
#include <cassert>
#include <stdexcept>

#include <iostream> // DEBUG

extern "C"
{
#include "c/luxem_rawread.h"
}

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

static void build_struct(
	std::shared_ptr<value> &&data, 
	std::function<void(std::shared_ptr<value> &&data)> &&callback,
	std::function<void(std::string const &key, std::shared_ptr<value> &data)> const &preprocess = {})
{
	if (data->is<reader::object_context>())
	{
		auto object_context = data->as<reader::object_context>();

		std::shared_ptr<object> out;
		if (data->has_type()) out = std::make_shared<object>(data->get_type(), od{});
		else out = std::make_shared<object>();

		object_context.passthrough([out, &preprocess](std::string &&key, std::shared_ptr<value> &&data) 
		{ 
			if (preprocess) preprocess(key, data);
			build_struct(std::move(data), [out, key = std::move(key)](std::shared_ptr<value> &&data)
			{ 
				out->get_data().insert(std::make_pair(std::move(key), std::move(data))); 
			}, preprocess);
		});

		object_context.finally([callback = std::move(callback), out]() mutable
		{ 
			callback(std::move(out)); 
		});
	}
	else if (data->is<reader::array_context>())
	{
		auto array_context = data->as<reader::array_context>();

		std::shared_ptr<array> out;
		if (data->has_type()) out = std::make_shared<array>(data->get_type(), ad{});
		else out = std::make_shared<array>();

		array_context.element([out, &preprocess](std::shared_ptr<value> &&data) 
		{ 
			if (preprocess) preprocess({}, data);
			build_struct(std::move(data), [out](std::shared_ptr<value> &&data)
			{
				out->get_data().emplace_back(std::move(data)); 
			}, preprocess);
		});

		array_context.finally([callback = std::move(callback), out]() mutable 
		{ 
			callback(std::move(out)); 
		});
	}
	else
	{
		callback(std::move(data));
	}
}


reader::object_context::object_context(std::string &&type, object_stackable &base) : value(type), base(base) {}

reader::object_context::object_context(object_stackable &base) : base(base) {}
		
std::string const reader::object_context::name("object_context");

std::string const &reader::object_context::get_name(void) const { return name; }

void reader::object_context::element(std::string &&key, std::function<void(std::shared_ptr<value> &&)> &&callback)
{
	assert(base.callbacks.find(key) == base.callbacks.end());
	base.callbacks.emplace(key, callback);
}

void reader::object_context::build_struct(
	std::string const &key, 
	std::function<void(std::shared_ptr<value> &&data)> &&callback, 
	std::function<void(std::string const &key, std::shared_ptr<value> &data)> const &preprocess)
{
	assert(base.callbacks.find(key) == base.callbacks.end());
	base.callbacks.emplace(key, [callback = std::move(callback), key, preprocess](std::shared_ptr<value> &&data) mutable
	{ 
		if (preprocess) preprocess(key, data);
		luxem::build_struct(std::move(data), std::move(callback), preprocess); 
	});
}

void reader::object_context::passthrough(std::function<void(std::string &&key, std::shared_ptr<value> &&data)> &&callback)
{
	assert(!base.passthrough_callback);
	base.passthrough_callback = std::move(callback);
}

void reader::object_context::finally(std::function<void(void)> &&callback)
{ 
	assert(!base.finish_callback);
	base.finish_callback = callback; 
}

reader::array_context::array_context(std::string &&type, array_stackable &base) : value(type), base(base) {}

reader::array_context::array_context(array_stackable &base) : base(base) { }

std::string const reader::array_context::name("array_context");

std::string const &reader::array_context::get_name(void) const { return name; }

void reader::array_context::element(std::function<void(std::shared_ptr<value> &&data)> &&callback)
{
	assert(!base.callback);
	base.callback = callback;
}

void reader::array_context::build_struct(
	std::function<void(std::shared_ptr<value> &&data)> &&callback,
	std::function<void(std::string const &key, std::shared_ptr<value> &data)> const &preprocess)
{
	assert(!base.callback);
	base.callback = [callback = std::move(callback), preprocess](std::shared_ptr<value> &&data) mutable
	{ 
		if (preprocess) preprocess({}, data);
		luxem::build_struct(std::move(data), std::move(callback), preprocess); 
	};
}

void reader::array_context::finally(std::function<void(void)> &&callback)
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
		[this](std::string &&data) { process(std::make_shared<luxem::primitive>(std::move(data))); }
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

template <typename ...argument_types> 
	std::vector<std::shared_ptr<luxem::value>> read_struct_implementation(argument_types ...arguments)
{
	std::vector<std::shared_ptr<luxem::value>> out;
	reader instance;
	instance.build_struct([&out](std::shared_ptr<luxem::value> &&data) { out.emplace_back(std::move(data)); });
	instance.feed(std::forward<argument_types>(arguments)...);
	return out;
}

std::vector<std::shared_ptr<luxem::value>> read_struct(std::string const &data) 
	{ return read_struct_implementation(data); }

std::vector<std::shared_ptr<luxem::value>> read_struct(char const *pointer, size_t length)
	{ return read_struct_implementation(pointer, length); }

std::vector<std::shared_ptr<luxem::value>> read_struct(FILE *file)
	{ return read_struct_implementation(file); }

}

