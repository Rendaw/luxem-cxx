#ifndef luxem_cxx_struct_h
#define luxem_cxx_struct_h

#include <vector>
#include <map>
#include <cassert>
#include <typeinfo>
#include <memory>
#include <sstream>

namespace luxem
{

struct value
{
	value(std::string const &type = std::string());
	virtual ~value(void);

	bool has_type(void) const;
	std::string const &get_type(void) const;
	void set_type(std::string const &type);
	void set_type(std::string &&type);

	// derivates must also specify a static string member named 'name'
	virtual std::string const &get_name(void) const = 0;

	template <typename type> bool is(void) const 
		{ return typeid(*this) == typeid(type); }
	
	template <typename type> bool is_derived(void) const 
		{ return dynamic_cast<type const *>(this); }

	template <typename type> type &as(void) 
	{ 
		if (!is<type>()) 
		{
			std::stringstream message;
			message << "Expected " << type::name << ", found " << get_name();
			throw std::runtime_error(message.str());
		}
		return *static_cast<type *>(this); 
	}

	template <typename type> type const &as(void) const 
	{ 
		if (!is<type>()) 
		{
			std::stringstream message;
			message << "Expected " << type::name << ", found " << get_name();
			throw std::runtime_error(message.str());
		}
		return *static_cast<type const *>(this); 
	}

	template <typename type> type &as_derived(void) 
	{ 
		if (!is_derived<type>()) 
		{
			std::stringstream message;
			message << "Expected subtype of " << type::name << ", found " << get_name();
			throw std::runtime_error(message.str());
		}
		return *static_cast<type *>(this); 
	}
	
	template <typename type> type const &as_derived(void) const
	{ 
		if (!is_derived<type>()) 
		{
			std::stringstream message;
			message << "Expected subtype of " << type::name << ", found " << get_name();
			throw std::runtime_error(message.str());
		}
		return *static_cast<type const *>(this); 
	}

	private:
		std::string type;
};

struct subencodings
{
	struct ascii16 {};
};

struct primitive_value : value
{
	using value::value;
	primitive_value(std::string const &data);
	primitive_value(std::string &&data);
	primitive_value(std::string const &type, std::string const &data);
	primitive_value(std::string &&type, std::string &&data);

	primitive_value(char const *data);
	primitive_value(std::string const &type, char const *data);
	primitive_value(bool data);
	primitive_value(std::string const &type, bool data);
	primitive_value(int data);
	primitive_value(std::string const &type, int data);
	primitive_value(unsigned int data);
	primitive_value(std::string const &type, unsigned int data);
	primitive_value(float data);
	primitive_value(std::string const &type, float data);
	primitive_value(double data);
	primitive_value(std::string const &type, double data);
	primitive_value(subencodings::ascii16, std::vector<uint8_t> const &data);
	primitive_value(std::string const &type, subencodings::ascii16, std::vector<uint8_t> const &data);

	static std::string const name;
	std::string const &get_name(void) const override;

	std::string const &get_primitive(void) const;
	bool get_bool(void) const;
	int get_int(void) const;
	unsigned int get_uint(void) const;
	float get_float(void) const;
	double get_double(void) const;
	std::string const &get_string(void) const;
	std::vector<uint8_t> get_ascii16(void) const;

	private:
		std::string const data;
};

struct object_value : value
{
	typedef std::map<std::string, std::shared_ptr<value>> object_data;

	object_value(void);
	object_value(object_data &&data);
	object_value(std::string const &type, object_data &&data);
	object_value(std::string &&type, object_data &&data);
	
	static std::string const name;
	std::string const &get_name(void) const override;

	object_data &get_data(void);
	object_data const &get_data(void) const;

	private:
		object_data data;
};

typedef object_value::object_data od;

struct array_value : value
{
	typedef std::vector<std::shared_ptr<value>> array_data;

	array_value(void);
	array_value(array_data &&data);
	array_value(std::string const &type, array_data &&data);
	array_value(std::string &&type, array_data &&data);
	
	static std::string const name;
	std::string const &get_name(void) const override;

	array_data &get_data(void);
	array_data const &get_data(void) const;

	private:
		array_data data;
};

typedef array_value::array_data ad;

typedef std::function
<
	void (std::string const &opt_key, std::shared_ptr<value> &node)
> walk_callback;

typedef std::function
<
	void (std::string const &opt_key, std::shared_ptr<value> const &node)
> const_walk_callback;

void walk(std::shared_ptr<value> &root, walk_callback const &callback);
void walk(std::shared_ptr<value> const &root, const_walk_callback const &callback);
void walk(std::shared_ptr<object_value> const &root, const_walk_callback const &callback);
void walk(std::shared_ptr<array_value> const &root, const_walk_callback const &callback);

}

#endif

