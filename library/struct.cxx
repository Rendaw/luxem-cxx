#include "struct.h"

#include <sstream>
#include <cstdlib>
#include <list>
#include <algorithm>
#include <utility>

extern "C"
{
#include "c/luxem_rawread.h"
#include "c/luxem_rawwrite.h"
}

namespace luxem
{

value::value(std::string const &type) : type(type) { }

value::~value(void) { }

bool value::has_type(void) const { return !type.empty(); }

std::string const &value::get_type(void) const { assert(has_type()); return type; }
	
void value::set_type(std::string const &type) { this->type = type; }
	
void value::set_type(std::string &&type) { this->type = std::move(type); }
	
template <typename type> type convert_to(std::string const &data)
{
	type out;
	std::stringstream temp(data);
	temp >> out;
	return out;
}

template <typename type> std::string convert_from(type const &data)
{
	std::stringstream render;
	render << data;
	return render.str();
}

template <> bool convert_to(std::string const &data)
{
	auto lower = data;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::toupper);
	return (lower != "0") &&
		(lower != "false") &&
		(lower != "no");
}

template <> std::string convert_from(bool const &data)
{
	return data ? "true" : "false";
}

std::vector<uint8_t> convert_to(subencodings::ascii16, std::string const &data)
{
	luxem_string_t input{&data[0], data.length()};
	luxem_string_t error;
	auto converted = luxem_from_ascii16(&input, &error);
	if (!converted)
		throw std::runtime_error(std::string(error.pointer, error.length));
	std::vector<uint8_t> out;
	out.resize(converted->length);
	memcpy(&out[0], converted->pointer, converted->length);
	free((void *)converted);
	return out;
}

std::string convert_from(subencodings::ascii16, std::vector<uint8_t> const &data)
{
	luxem_string_t input{(char const *)&data[0], data.size()};
	luxem_string_t error;
	auto converted = luxem_to_ascii16(&input, &error);
	if (!converted)
		throw std::runtime_error(std::string(error.pointer, error.length));
	std::string out(converted->pointer, converted->length);
	free((void *)converted);
	return out;
}

std::string const primitive_value::name("primitive_value");

primitive_value::primitive_value(std::string const &data) :
	data(data) {}
primitive_value::primitive_value(std::string &&data) :
	data(std::move(data)) {}
primitive_value::primitive_value(std::string const &type, std::string const &data) :
	value(type), data(data) {}
primitive_value::primitive_value(std::string &&type, std::string &&data) :
	value(std::move(type)), data(std::move(data)) {}
	
primitive_value::primitive_value(char const *data) :
	data(data) {}
primitive_value::primitive_value(std::string const &type, char const *data) :
	value(type), data(data) {}
	
primitive_value::primitive_value(bool data) :
	data(convert_from<bool>(data)) {}
primitive_value::primitive_value(std::string const &type, bool data) :
	value(type), data(convert_from<bool>(data)) {}

primitive_value::primitive_value(int data) :
	data(convert_from<int>(data)) {}
primitive_value::primitive_value(std::string const &type, int data) :
	value(type), data(convert_from<int>(data)) {}

primitive_value::primitive_value(unsigned int data) :
	data(convert_from<unsigned int>(data)) {}
primitive_value::primitive_value(std::string const &type, unsigned int data) :
	value(type), data(convert_from<unsigned int>(data)) {}

primitive_value::primitive_value(float data) :
	data(convert_from<float>(data)) {}
primitive_value::primitive_value(std::string const &type, float data) :
	value(type), data(convert_from<float>(data)) {}

primitive_value::primitive_value(double data) :
	data(convert_from<double>(data)) {}
primitive_value::primitive_value(std::string const &type, double data) :
	value(type), data(convert_from<double>(data)) {}

primitive_value::primitive_value(subencodings::ascii16, std::vector<uint8_t> const &data) :
	data(convert_from(subencodings::ascii16{}, data)) {}
primitive_value::primitive_value(std::string const &type, subencodings::ascii16, std::vector<uint8_t> const &data) :
	value(type), data(convert_from(subencodings::ascii16{}, data)) {}
	
std::string const &primitive_value::get_name(void) const { return name; }

std::string const &primitive_value::get_primitive(void) const 
	{ return data; }

bool primitive_value::get_bool(void) const 
	{ return convert_to<bool>(data); }

int primitive_value::get_int(void) const 
	{ return convert_to<int>(data); }

unsigned int primitive_value::get_uint(void) const 
	{ return convert_to<unsigned int>(data); }

float primitive_value::get_float(void) const 
	{ return convert_to<float>(data); }

double primitive_value::get_double(void) const 
	{ return convert_to<double>(data); }

std::string const &primitive_value::get_string(void) const 
	{ return data; }

std::vector<uint8_t> primitive_value::get_ascii16(void) const 
	{ return convert_to(subencodings::ascii16{}, data); }

std::string const object_value::name("object_value");

object_value::object_value(void) {}

object_value::object_value(object_data &&data) : value(), data(std::move(data)) {}

object_value::object_value(std::string const &type, object_data &&data) : value(type), data(std::move(data)) {}

object_value::object_value(std::string &&type, object_data &&data) : value(std::move(type)), data(std::move(data)) {}

std::string const &object_value::get_name(void) const { return name; }

object_value::object_data &object_value::get_data(void) { return data; }

object_value::object_data const &object_value::get_data(void) const { return data; }
	
std::string const array_value::name("array_value");

array_value::array_value(void) {}

array_value::array_value(array_data &&data) : value(), data(std::move(data)) {}

array_value::array_value(std::string const &type, array_data &&data) : value(type), data(std::move(data)) {}

array_value::array_value(std::string &&type, array_data &&data) : value(std::move(type)), data(std::move(data)) {}

std::string const &array_value::get_name(void) const { return name; }

array_value::array_data &array_value::get_data(void) { return data; }

array_value::array_data const &array_value::get_data(void) const { return data; }

template <typename callback_type> struct walk_stackable 
{
	virtual ~walk_stackable(void) {}
	virtual bool step(std::list<std::unique_ptr<walk_stackable<callback_type>>> &stack, callback_type const &callback) = 0;
};

template 
<
	typename node_type, 
	typename callback_type
> struct object_walk_stackable;

template 
<
	typename node_type, 
	typename callback_type
> struct array_walk_stackable;

template // The costs of DRY in c++
<
	typename node_type, 
	typename callback_type
> static void walk_one
(
	std::list<std::unique_ptr<walk_stackable<callback_type>>> &stack, 
	std::string const &key, 
	node_type &node, 
	callback_type const &callback
)
{
	callback(key, node);
	if (!node) return;
	if (node->template is<object_value>())
		stack.emplace_back(std::make_unique<object_walk_stackable<
			node_type, 
			callback_type>>(node->template as<object_value>()));
	else if (node->template is<array_value>())
		stack.emplace_back(std::make_unique<array_walk_stackable<
			node_type, 
			callback_type>>(node->template as<array_value>()));
}

template 
<
	typename node_type, 
	typename callback_type
> struct object_walk_stackable : walk_stackable<callback_type>
{
	typename std::conditional<std::is_const<node_type>::value, object_value const, object_value>::type &node;
	typename std::conditional<std::is_const<node_type>::value, od::const_iterator, od::iterator>::type iterator;

	object_walk_stackable(decltype(node) &node) : node(node), iterator(node.get_data().begin()) {}

	bool step(std::list<std::unique_ptr<walk_stackable<callback_type>>> &stack, callback_type const &callback) override
	{
		if (iterator == node.get_data().end()) return false;
		walk_one(stack, iterator->first, iterator->second, callback);
		++iterator;
		return true;
	}
};

template 
<
	typename node_type, 
	typename callback_type
> struct array_walk_stackable : walk_stackable<callback_type>
{
	typename std::conditional<std::is_const<node_type>::value, array_value const, array_value>::type &node;
	typename std::conditional<std::is_const<node_type>::value, ad::const_iterator, ad::iterator>::type iterator;

	array_walk_stackable(decltype(node) &node) : node(node), iterator(node.get_data().begin()) {}

	bool step(std::list<std::unique_ptr<walk_stackable<callback_type>>> &stack, callback_type const &callback) override
	{
		if (iterator == node.get_data().end()) return false;
		walk_one(stack, {}, *iterator, callback);
		++iterator;
		return true;
	}
};

template <typename node_type, typename callback_type> static void walk_implementation(node_type &root, callback_type const &callback)
{
	std::list<std::unique_ptr<walk_stackable<callback_type>>> stack;
	walk_one(stack, {}, root, callback);
	while (!stack.empty())
		if (!stack.back()->step(stack, callback))
			stack.pop_back();
}

void walk(std::shared_ptr<value> &root, walk_callback const &callback) 
	{ walk_implementation(root, callback); }

void walk(std::shared_ptr<value> const &root, const_walk_callback const &callback) 
	{ walk_implementation(root, callback); }

void walk(std::shared_ptr<object_value> const &root, const_walk_callback const &callback) 
	{ walk_implementation(root, callback); }

void walk(std::shared_ptr<array_value> const &root, const_walk_callback const &callback) 
	{ walk_implementation(root, callback); }

}

