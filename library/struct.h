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

struct primitive : value
{
	using value::value;

	static std::string const name;
	std::string const &get_name(void) const override;

	primitive(std::string const &data);
	primitive(std::string &&data);
	primitive(std::string const &type, std::string const &data);
	primitive(std::string &&type, std::string &&data);

	primitive(char const *data);
	primitive(std::string const &type, char const *data);
	primitive(bool data);
	primitive(std::string const &type, bool data);
	primitive(int data);
	primitive(std::string const &type, int data);
	primitive(unsigned int data);
	primitive(std::string const &type, unsigned int data);
	primitive(float data);
	primitive(std::string const &type, float data);
	primitive(double data);
	primitive(std::string const &type, double data);
	primitive(subencodings::ascii16, std::vector<uint8_t> const &data);
	primitive(std::string const &type, subencodings::ascii16, std::vector<uint8_t> const &data);
	
	void set(std::string const &data);
	void set(char const *data);
	void set(bool data);
	void set(int data);
	void set(unsigned int data);
	void set(float data);
	void set(double data);
	void set(subencodings::ascii16, std::vector<uint8_t> const &data);

	std::string const &get_primitive(void) const;
	bool get_bool(void) const;
	int get_int(void) const;
	unsigned int get_uint(void) const;
	float get_float(void) const;
	double get_double(void) const;
	std::string const &get_string(void) const;
	std::vector<uint8_t> get_ascii16(void) const;

	private:
		std::string data;
};

struct object : value
{
	typedef std::map<std::string, std::shared_ptr<value>> object_data;

	object(void);
	object(object_data &&data);
	object(std::string const &type, object_data &&data);
	object(std::string &&type, object_data &&data);
	
	static std::string const name;
	std::string const &get_name(void) const override;

	object_data &get_data(void);
	object_data const &get_data(void) const;

	private:
		object_data data;
};

typedef object::object_data od;

struct array : value
{
	typedef std::vector<std::shared_ptr<value>> array_data;

	array(void);
	array(array_data &&data);
	array(std::string const &type, array_data &&data);
	array(std::string &&type, array_data &&data);
	
	static std::string const name;
	std::string const &get_name(void) const override;

	array_data &get_data(void);
	array_data const &get_data(void) const;

	private:
		array_data data;
};

typedef array::array_data ad;

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
void walk(std::shared_ptr<object> const &root, const_walk_callback const &callback);
void walk(std::shared_ptr<array> const &root, const_walk_callback const &callback);

}

#endif

