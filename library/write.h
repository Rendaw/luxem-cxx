#ifndef luxem_cxx_write_h
#define luxem_cxx_write_h

#include <list>
#include <memory>

extern "C"
{
#include "c/luxem_rawwrite.h"
}
#include "struct.h"

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

	void set_pretty(char spacer = '\t', size_t multiple = 1);

	void object_begin(void);
	void object_end(void);
	void array_begin(void);
	void array_end(void);
	void key(std::string const &data);
	void type(std::string const &data);
	void primitive(std::string const &data);

	std::string dump(void) const;

	private:
		luxem_rawwrite_context_t *context;
		std::function<void(std::string &&chunk)> callback;

		void check_error(bool succeeded);
};

struct writer : raw_writer
{
	writer &value(luxem::value const &data);
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

