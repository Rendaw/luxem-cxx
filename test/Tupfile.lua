for index, source in ipairs(tup.glob('*.cxx'))
do 
	tup.definerule{
		inputs = {source, '../luxem-cxx.so'},
		outputs = {source .. '.bin'},
		command = 'g++ -std=c++1y -pedantic -Wall -Werror -Wno-error=unused-function -ggdb -O0 -o ' .. source .. '.bin ' .. source .. ' ../luxem-cxx.so'
	}
end

