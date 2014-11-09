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

value::value(void) : typed(false) {}

value::value(std::string const &type) : typed(true), type(type) {}

value::~value(void) {}

bool value::has_type(void) const { return typed; }

std::string const &value::get_type(void) const { assert(has_type()); return type; }
	
void value::set_type(std::string const &type) { this->type = type; typed = true; }
	
void value::set_type(std::string &&type) { this->type = std::move(type); typed = true; }
	
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

bool convert_to_bool(std::string const &data)
{
	auto lower = data;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return (lower != "0") &&
		(lower != "false") &&
		(lower != "no");
}

std::string convert_from_bool(bool const &data)
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

std::string const primitive::name("primitive");

std::string const &primitive::get_name(void) const { return name; }

primitive::primitive(std::string const &data) :
	data(data) {}
primitive::primitive(std::string &&data) :
	data(std::move(data)) {}
primitive::primitive(std::string const &type, std::string const &data) :
	value(type), data(data) {}
primitive::primitive(std::string &&type, std::string &&data) :
	value(std::move(type)), data(std::move(data)) {}
	
primitive::primitive(char const *data) :
	data(data) {}
primitive::primitive(std::string const &type, char const *data) :
	value(type), data(data) {}
	
primitive::primitive(bool data) :
	data(convert_from_bool(data)) {}
primitive::primitive(std::string const &type, bool data) :
	value(type), data(convert_from_bool(data)) {}

primitive::primitive(int data) :
	data(convert_from<int>(data)) {}
primitive::primitive(std::string const &type, int data) :
	value(type), data(convert_from<int>(data)) {}

primitive::primitive(unsigned int data) :
	data(convert_from<unsigned int>(data)) {}
primitive::primitive(std::string const &type, unsigned int data) :
	value(type), data(convert_from<unsigned int>(data)) {}

primitive::primitive(float data) :
	data(convert_from<float>(data)) {}
primitive::primitive(std::string const &type, float data) :
	value(type), data(convert_from<float>(data)) {}

primitive::primitive(double data) :
	data(convert_from<double>(data)) {}
primitive::primitive(std::string const &type, double data) :
	value(type), data(convert_from<double>(data)) {}

primitive::primitive(subencodings::ascii16, std::vector<uint8_t> const &data) :
	data(convert_from(subencodings::ascii16{}, data)) {}
primitive::primitive(std::string const &type, subencodings::ascii16, std::vector<uint8_t> const &data) :
	value(type), data(convert_from(subencodings::ascii16{}, data)) {}

void primitive::set(std::string const &data) 
	{ this->data = data; }
	
void primitive::set(char const *data) 
	{ this->data = data; }

void primitive::set(bool data) 
	{ this->data = convert_from_bool(data); }

void primitive::set(int data) 
	{ this->data = convert_from<int>(data); }

void primitive::set(unsigned int data) 
	{ this->data = convert_from<unsigned int>(data); }

void primitive::set(float data) 
	{ this->data = convert_from<float>(data); }

void primitive::set(double data) 
	{ this->data = convert_from<double>(data); }

void primitive::set(subencodings::ascii16, std::vector<uint8_t> const &data) 
	{ this->data = convert_from(subencodings::ascii16{}, data); }

std::string const &primitive::get_primitive(void) const 
	{ return data; }

bool primitive::get_bool(void) const 
	{ return convert_to_bool(data); }

int primitive::get_int(void) const 
	{ return convert_to<int>(data); }

unsigned int primitive::get_uint(void) const 
	{ return convert_to<unsigned int>(data); }

float primitive::get_float(void) const 
	{ return convert_to<float>(data); }

double primitive::get_double(void) const 
	{ return convert_to<double>(data); }

std::string const &primitive::get_string(void) const 
	{ return data; }

std::vector<uint8_t> primitive::get_ascii16(void) const 
	{ return convert_to(subencodings::ascii16{}, data); }

std::string const object::name("object");

object::object(void) {}

std::string const &object::get_name(void) const { return name; }

object::object(object_data &&data) : value(), data(std::move(data)) {}

object::object(std::string const &type, object_data &&data) : value(type), data(std::move(data)) {}

object::object(std::string &&type, object_data &&data) : value(std::move(type)), data(std::move(data)) {}

object::object_data &object::get_data(void) { return data; }

object::object_data const &object::get_data(void) const { return data; }
	
std::string const array::name("array");

std::string const &array::get_name(void) const { return name; }

array::array(void) {}

array::array(array_data &&data) : value(), data(std::move(data)) {}

array::array(std::string const &type, array_data &&data) : value(type), data(std::move(data)) {}

array::array(std::string &&type, array_data &&data) : value(std::move(type)), data(std::move(data)) {}

array::array_data &array::get_data(void) { return data; }

array::array_data const &array::get_data(void) const { return data; }

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
	if (node->template is<object>())
		stack.emplace_back(std::make_unique<object_walk_stackable<
			node_type, 
			callback_type>>(node->template as<object>()));
	else if (node->template is<array>())
		stack.emplace_back(std::make_unique<array_walk_stackable<
			node_type, 
			callback_type>>(node->template as<array>()));
}

template 
<
	typename node_type, 
	typename callback_type
> struct object_walk_stackable : walk_stackable<callback_type>
{
	typename std::conditional<std::is_const<node_type>::value, object const, object>::type &node;
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
	typename std::conditional<std::is_const<node_type>::value, array const, array>::type &node;
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

void walk(std::shared_ptr<object> const &root, const_walk_callback const &callback) 
	{ walk_implementation(root, callback); }

void walk(std::shared_ptr<array> const &root, const_walk_callback const &callback) 
	{ walk_implementation(root, callback); }

}

