#include "write.h"

extern "C"
{
#include "c/luxem_rawwrite.h"
}

namespace luxem
{

raw_writer::raw_writer(void) : context(luxem_rawwrite_construct())
	{ luxem_rawwrite_set_buffer_out(context); }

raw_writer::raw_writer(FILE *file) : context(luxem_rawwrite_construct())
	{ luxem_rawwrite_set_file_out(context, file); }

raw_writer::raw_writer(std::function<void(std::string &&chunk)> const &callback) : context(luxem_rawwrite_construct())
{
	luxem_rawwrite_set_write_callback(context, [](luxem_rawwrite_context_t *context, void *user_data, luxem_string_t const *string)
	{
		auto writer = reinterpret_cast<raw_writer *>(user_data);
		if (!writer->callback) return luxem_true;
		writer->callback(std::string(string->pointer, string->length));
		return luxem_true;
	}, this);
}
	
raw_writer::~raw_writer(void)
{
	luxem_rawwrite_destroy(context);
}

raw_writer &raw_writer::set_pretty(char spacer, size_t multiple)
{
	luxem_rawwrite_set_pretty(context, spacer, multiple);
	return *this;
}

raw_writer &raw_writer::object_begin(void) 
	{ check_error(luxem_rawwrite_object_begin(context)); return *this; }

raw_writer &raw_writer::object_end(void) 
	{ check_error(luxem_rawwrite_object_end(context)); return *this; }

raw_writer &raw_writer::array_begin(void) 
	{ check_error(luxem_rawwrite_array_begin(context)); return *this; }

raw_writer &raw_writer::array_end(void) 
	{ check_error(luxem_rawwrite_array_end(context)); return *this; }

raw_writer &raw_writer::key(std::string const &data)
{
	luxem_string_t temp{data.c_str(), data.length()};
	check_error(luxem_rawwrite_key(context, &temp));
	return *this;
}

raw_writer &raw_writer::type(std::string const &data)
{
	luxem_string_t temp{data.c_str(), data.length()};
	check_error(luxem_rawwrite_type(context, &temp));
	return *this;
}

raw_writer &raw_writer::primitive(std::string const &data) 
{
	luxem_string_t temp{data.c_str(), data.length()};
	check_error(luxem_rawwrite_primitive(context, &temp));
	return *this;
}

std::string raw_writer::dump(void) const
{
	auto temp = luxem_rawwrite_buffer_render(context);
	std::string out(temp->pointer, temp->length);
	free(temp);
	return out;
}
			
raw_writer::object_guard::object_guard(object_guard &&other) : base(other.base) 
	{ other.base = nullptr; }

raw_writer::object_guard::object_guard(raw_writer &base) : base(&base) {}

raw_writer::object_guard::~object_guard(void) 
	{ if (base) base->object_end(); }

raw_writer::array_guard::array_guard(array_guard &&other) : base(other.base) 
	{ other.base = nullptr; }

raw_writer::array_guard::array_guard(raw_writer &base) : base(&base) {}

raw_writer::array_guard::~array_guard(void) 
	{ if (base) base->array_end(); }

raw_writer::object_guard raw_writer::scope_object(void) 
	{ return object_guard(*this); }

raw_writer::array_guard raw_writer::scope_array(void) 
	{ return array_guard(*this); }

void raw_writer::check_error(bool succeeded)
{
	if (!succeeded)
	{
		auto message = luxem_rawwrite_get_error(context);
		assert(message->pointer);
		if (message->pointer)
			throw std::runtime_error(std::string(message->pointer, message->length));
		else throw std::runtime_error("Callback provided no error message.");
	}
}

struct array_stackable : writer::stackable
{
	array const &data;
	array::array_data::const_iterator iter;

	array_stackable(array const &data) : data(data), iter(data.get_data().begin()) { }

	bool step(writer &writer, std::list<std::unique_ptr<stackable>> &stack) override
	{
		if (iter == data.get_data().end())
		{
			writer.array_end();
			return false;
		}
		writer.process(stack, **iter);
		++iter;
		return true;
	}
};

struct object_stackable : writer::stackable
{
	object const &data;
	object::object_data::const_iterator iter;

	object_stackable(object const &data) : data(data), iter(data.get_data().begin()) { }

	bool step(writer &writer, std::list<std::unique_ptr<stackable>> &stack) override
	{
		if (iter == data.get_data().end())
		{
			writer.object_end();
			return false;
		}
		writer.key(iter->first);
		writer.process(stack, *iter->second);
		++iter;
		return true;
	}
};

writer::stackable::~stackable(void) {}
	
writer &writer::set_pretty(char spacer, size_t multiple) 
	{ raw_writer::set_pretty(spacer, multiple); return *this; }

writer &writer::object_begin(void)
	{ raw_writer::object_begin(); return *this; }

writer &writer::object_end(void)
	{ raw_writer::object_end(); return *this; }

writer &writer::array_begin(void)
	{ raw_writer::array_begin(); return *this; }

writer &writer::array_end(void)
	{ raw_writer::array_end(); return *this; }

writer &writer::key(std::string const &data)
	{ raw_writer::key(data); return *this; }

writer &writer::type(std::string const &data)
	{ raw_writer::type(data); return *this; }

writer &writer::primitive(std::string const &data)
	{ raw_writer::primitive(data); return *this; }

writer &writer::value(luxem::value const &data)
{
	std::list<std::unique_ptr<stackable>> stack;
	process(stack, data);
	while (!stack.empty())
	{
		while (stack.back()->step(*this, stack)) {}
		stack.pop_back();
	}
	return *this;
}
	
writer &writer::value(char const *data)
	{ return value(luxem::primitive(data)); }

writer &writer::value(std::string const &type, char const *data)
	{ return value(luxem::primitive(type, data)); }

writer &writer::value(bool data) 
	{ return value(luxem::primitive(data)); }

writer &writer::value(std::string const &type, bool data)
	{ return value(luxem::primitive(type, data)); }

writer &writer::value(int data) 
	{ return value(luxem::primitive(data)); }

writer &writer::value(std::string const &type, int data)
	{ return value(luxem::primitive(type, data)); }

writer &writer::value(unsigned int data) 
	{ return value(luxem::primitive(data)); }

writer &writer::value(std::string const &type, unsigned int data)
	{ return value(luxem::primitive(type, data)); }

writer &writer::value(float data) 
	{ return value(luxem::primitive(data)); }

writer &writer::value(std::string const &type, float data)
	{ return value(luxem::primitive(type, data)); }

writer &writer::value(double data) 
	{ return value(luxem::primitive(data)); }

writer &writer::value(std::string const &type, double data)
	{ return value(luxem::primitive(type, data)); }

writer &writer::value(subencodings::ascii16, std::vector<uint8_t> const &data) 
	{ return value(luxem::primitive(subencodings::ascii16{}, data)); }

writer &writer::value(std::string const &type, subencodings::ascii16, std::vector<uint8_t> const &data)
	{ return value(luxem::primitive(type, subencodings::ascii16{}, data)); }

void writer::process(std::list<std::unique_ptr<writer::stackable>> &stack, luxem::value const &data)
{
	if (data.has_type()) type(data.get_type());
	if (data.is<array>())
	{
		array_begin();
		stack.push_back(std::make_unique<array_stackable>(data.as<array>()));
	}
	else if (data.is<object>())
	{
		object_begin();
		stack.push_back(std::make_unique<object_stackable>(data.as<object>()));
	}
	else if (data.is<luxem::primitive>())
		primitive(data.as<luxem::primitive>().get_primitive());
	else 
	{
		std::stringstream message;
		message << "Encountered unwritable type " << data.get_name() << " while trying to writing tree.";
		throw std::runtime_error(message.str());
	}
}

}

