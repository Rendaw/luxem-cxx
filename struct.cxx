#include "struct.h"

#include <sstream>
#include <cstdlib>

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

std::string &value::get_type(void) { assert(has_type()); return type; }

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

primitive_value::primitive_value(std::string const &data) :
	data(data) {}
primitive_value::primitive_value(std::string &&data) :
	data(std::move(data)) {}
primitive_value::primitive_value(std::string const &type, std::string const &data) :
	value(type), data(data) {}
primitive_value::primitive_value(std::string &&type, std::string &&data) :
	value(std::move(type)), data(std::move(data)) {}
	
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

object_value::object_value(void) {}

object_value::object_value(object_data &&data) : value(), data(std::move(data)) {}

object_value::object_value(std::string const &type, object_data &&data) : value(type), data(std::move(data)) {}

object_value::object_value(std::string &&type, object_data &&data) : value(std::move(type)), data(std::move(data)) {}

object_value::object_data &object_value::get_object(void) { return data; }

object_value::object_data const &object_value::get_object(void) const { return data; }
	
array_value::array_value(void) {}

array_value::array_value(array_data &&data) : value(), data(std::move(data)) {}

array_value::array_value(std::string const &type, array_data &&data) : value(type), data(std::move(data)) {}

array_value::array_value(std::string &&type, array_data &&data) : value(std::move(type)), data(std::move(data)) {}

array_value::array_data &array_value::get_array(void) { return data; }

array_value::array_data const &array_value::get_array(void) const { return data; }

}

