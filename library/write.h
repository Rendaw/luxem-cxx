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

	writer &value(luxem::value const &data);
	writer &value(char const *data);
	writer &value(std::string const &type, char const *data);
	writer &value(bool data);
	writer &value(std::string const &type, bool data);
	writer &value(int data);
	writer &value(std::string const &type, int data);
	writer &value(unsigned int data);
	writer &value(std::string const &type, unsigned int data);
	writer &value(float data);
	writer &value(std::string const &type, float data);
	writer &value(double data);
	writer &value(std::string const &type, double data);
	writer &value(subencodings::ascii16, std::vector<uint8_t> const &data);
	writer &value(std::string const &type, subencodings::ascii16, std::vector<uint8_t> const &data);

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

