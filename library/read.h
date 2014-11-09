#ifndef luxem_cxx_read_h
#define luxem_cxx_read_h

#include <string>
#include <functional>
#include <list>
#include <memory>

#include "struct.h"

struct luxem_rawread_context_t;

namespace luxem
{

struct raw_reader
{
	raw_reader(
		std::function<void(void)> object_begin,
		std::function<void(void)> object_end,
		std::function<void(void)> array_begin,
		std::function<void(void)> array_end,
		std::function<void(std::string &&data)> key,
		std::function<void(std::string &&data)> type,
		std::function<void(std::string &&data)> primitive
	);
	~raw_reader(void);

	size_t feed(std::string const &data, bool finish=true);
	size_t feed(char const *pointer, size_t length, bool finish=true);
	void feed(FILE *file);

	// PRIVATE - but not actually, since cxx has near useless visibility definition
		luxem_rawread_context_t *context;
		std::function<void(void)> object_begin;
		std::function<void(void)> object_end;
		std::function<void(void)> array_begin;
		std::function<void(void)> array_end;
		std::function<void(std::string &&data)> key;
		std::function<void(std::string &&data)> type;
		std::function<void(std::string &&data)> primitive;
		std::string exception_message;
};

struct reader : raw_reader
{
	private:
		struct object_stackable;
		struct array_stackable;
	public:

	struct object_context : value
	{
		object_context(std::string &&type, object_stackable &base);
		object_context(object_stackable &base);
	
		object_context(object_context const &) = delete;
		object_context(object_context &&) = delete;
		object_context &operator =(object_context const &) = delete;
		object_context &operator =(object_context &&) = delete;

		static std::string const name;
		std::string const &get_name(void) const override;

		void set_austerity_measures(bool on);

		void element(std::string &&key, std::function<void(std::shared_ptr<value> &&)> &&callback);
		void build_struct(
			std::string const &key, 
			std::function<void(std::shared_ptr<value> &&data)> &&callback, 
			std::function<void(std::string const &key, std::shared_ptr<value> &data)> const &preprocess = {});
		void passthrough(std::function<void(std::string &&key, std::shared_ptr<value> &&data)> &&callback);
		void finally(std::function<void(void)> &&callback);

		private:
			object_stackable &base;
	};

	struct array_context : value
	{
		array_context(std::string &&type, array_stackable &base);
		array_context(array_stackable &base);
	
		array_context(array_context const &) = delete;
		array_context(array_context &&) = delete;
		array_context &operator =(array_context const &) = delete;
		array_context &operator =(array_context &&) = delete;

		static std::string const name;
		std::string const &get_name(void) const override;

		void element(std::function<void(std::shared_ptr<value> &&)> &&callback);
		void build_struct(
			std::function<void(std::shared_ptr<value> &&data)> &&callback, 
			std::function<void(std::string const &key, std::shared_ptr<value> &data)> const &preprocess = {});
		void finally(std::function<void(void)> &&callback);
		private:
			array_stackable &base;
	};

	reader(bool austerity_measures = true);
	reader &element(std::function<void(std::shared_ptr<value> &&data)> &&callback);
	reader &build_struct(std::function<void(std::shared_ptr<value> &&data)> &&callback);

	private:
		struct stackable
		{
			virtual ~stackable(void);
			virtual void process(std::shared_ptr<value> &&data, std::string &&key) = 0;
			virtual void finish(void) = 0;
		};

		struct object_stackable : stackable
		{
			object_stackable(bool austerity_measures);
			void process(std::shared_ptr<value> &&data, std::string &&key) override;
			void finish(void) override;

			friend struct object_context;
			private:
				bool austerity_measures;
				std::function<void(std::string &&key, std::shared_ptr<value> &&data)> passthrough_callback;
				std::function<void(void)> finish_callback;
				std::map<std::string, std::function<void(std::shared_ptr<value> &&)>> callbacks;
		};
	
		struct array_stackable : stackable
		{
			void process(std::shared_ptr<value> &&element, std::string &&key) override;
			void finish(void) override;

			friend struct array_context;
			private:
				std::function<void(void)> finish_callback;
				std::function<void(std::shared_ptr<value> &&data)> callback;
		};

		std::list<std::unique_ptr<stackable>> stack;
		bool has_key;
		std::string current_key;
		bool has_type;
		std::string current_type;

		void process(std::shared_ptr<value> &&data);
		void pop(void);
};

std::vector<std::shared_ptr<luxem::value>> read_struct(std::string const &data) ;
std::vector<std::shared_ptr<luxem::value>> read_struct(char const *pointer, size_t length);
std::vector<std::shared_ptr<luxem::value>> read_struct(FILE *file);

}

#endif

