#ifndef luxem_cxx_write_h
#define luxem_cxx_write_h

#include <list>
#include <memory>

#include "struct.h"

struct luxem_rawwrite_context_t;

namespace luxem
{

struct raw_writer
{
	raw_writer(void);
	raw_writer(FILE *file);
	raw_writer(std::function<void(std::string &&chunk)> const &callback);
	raw_writer(raw_writer &&other) = delete;
	raw_writer &operator =(raw_writer &&other) = delete;
	~raw_writer(void);

	raw_writer &set_pretty(char spacer = '\t', size_t multiple = 1);

	raw_writer &object_begin(void);
	raw_writer &object_end(void);
	raw_writer &array_begin(void);
	raw_writer &array_end(void);
	raw_writer &key(std::string const &data);
	raw_writer &type(std::string const &data);
	raw_writer &primitive(std::string const &data);

	std::string dump(void) const;

	private:
		struct object_guard 
		{ 
			raw_writer *base; 
			object_guard(object_guard &&other);
			object_guard(raw_writer &base);
			~object_guard(void);
			object_guard(object_guard const &) = delete;
			object_guard &operator =(object_guard const &) = delete;
			object_guard &operator =(object_guard &&) = delete;
		};
		struct array_guard 
		{ 
			raw_writer *base; 
			array_guard(array_guard &&other);
			array_guard(raw_writer &base);
			~array_guard(void);
			array_guard(array_guard const &) = delete;
			array_guard &operator =(array_guard const &) = delete;
			array_guard &operator =(array_guard &&) = delete;
		};
	public:
	object_guard scope_object(void);
	array_guard scope_array(void);

	private:
		luxem_rawwrite_context_t *context;
		std::function<void(std::string &&chunk)> callback;

		void check_error(bool succeeded);
};

struct writer : raw_writer
{
	using raw_writer::raw_writer;
	
	writer &set_pretty(char spacer = '\t', size_t multiple = 1);

	writer &object_begin(void);
	writer &object_end(void);
	writer &array_begin(void);
	writer &array_end(void);
	writer &key(std::string const &data);
	writer &type(std::string const &data);
	writer &primitive(std::string const &data);

	writer &value(std::shared_ptr<luxem::value> const &data);

	template <typename data_type, class enable = void> struct is_smart_ptr { static constexpr bool value = false; };
	template <typename data_type> struct is_smart_ptr<
		data_type, 
		typename std::enable_if<
			std::is_same<data_type, std::shared_ptr<typename data_type::element_type>>::value || 
			std::is_same<data_type, std::unique_ptr<typename data_type::element_type>>::value
		>::type
	> { static constexpr bool value = true; };
	template <
		typename data_type,
		typename std::enable_if<
			!std::is_pointer<data_type>::value &&
			!is_smart_ptr<data_type>::value
		>::type * = nullptr
	> writer &value(data_type const &data)
		{ primitive(to_string<data_type>(data)); return *this; }
	template <typename data_type> writer &value(std::string const &type_name, data_type const &data)
		{ type(type_name); primitive(to_string<data_type>(data)); return *this; }
	template <typename data_type> writer &value_ascii16(data_type const &data)
		{ primitive(to_string_ascii16<data_type>(data)); return *this; }
	template <typename data_type> writer &value_ascii16(std::string const &type_name, data_type const &data)
		{ type(type_name); primitive(to_string_ascii16<data_type>(data)); return *this; }

	friend struct array_stackable;
	friend struct object_stackable;
	private:
		struct stackable
		{
			virtual ~stackable(void);
			virtual bool step(writer &writer, std::list<std::unique_ptr<stackable>> &stack) = 0;
		};

		void process(std::list<std::unique_ptr<stackable>> &stack, luxem::value const &data);
};

}

#endif

