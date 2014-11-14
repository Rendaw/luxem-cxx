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

template <typename data_type> inline std::string to_string(data_type const &data)
{
	std::stringstream render;
	render << data;
	return render.str();
}

template <> inline std::string to_string<std::string>(std::string const &data)
	{ return data; }

template <> inline std::string to_string<bool>(bool const &data)
{
	return data ? "true" : "false";
}

std::string to_string_ascii16(uint8_t const *pointer, size_t const length);
template <typename data_type> inline std::string to_string_ascii16(data_type const &data)
{
	return to_string_ascii16(
		reinterpret_cast<uint8_t const *>(&data[0]), 
		sizeof(typename data_type::value_type) * data.size()
	);
}

struct value
{
	value(void);
	value(std::string const &type);
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
		bool typed;
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

	primitive(std::string &&data);
	primitive(std::string &&type, std::string &&data);

	template <typename data_type> primitive(data_type const &data) :
		data(to_string<data_type>(data))
		{}
	template <typename data_type> primitive(std::string const &type_name, data_type const &data) : 
		value(type_name), data(to_string<data_type>(data))
		{}
	template <typename data_type> primitive(subencodings::ascii16, data_type const &data) :
		data(to_string_ascii16<data_type>(data))
		{}
	template <typename data_type> primitive(std::string const &type_name, subencodings::ascii16, data_type const &data) : 
		value(type_name), data(to_string_ascii16<data_type>(data))
		{}

	primitive(primitive const &) = delete;
	primitive(primitive &&) = delete;
	primitive &operator =(primitive const &) = delete;
	primitive &operator =(primitive &&) = delete;
	
	template <typename data_type> void set(data_type const &data)
		{ data = to_string<data_type>(data); }
	template <typename data_type> void set(subencodings::ascii16, data_type const &data)
		{ data = to_string_ascii16<data_type>(data); }

	std::string const &get_primitive(void) const;
	bool get_bool(void) const;
	int64_t get_int(void) const;
	uint64_t get_uint(void) const;
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
	
	object(object const &) = delete;
	object(object &&) = delete;
	object &operator =(object const &) = delete;
	object &operator =(object &&) = delete;
	
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
	
	array(array const &) = delete;
	array(array &&) = delete;
	array &operator =(array const &) = delete;
	array &operator =(array &&) = delete;
	
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

