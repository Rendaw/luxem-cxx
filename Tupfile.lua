local inputs = {}
for index, source in ipairs(tup.glob('*.cxx'))
	do inputs[#inputs + 1] = source end
for index, object in ipairs(tup.glob('c/*.o'))
	do inputs[#inputs + 1] = object end
tup.definerule{
	inputs = inputs,
	outputs = {'luxem-cxx.so'},
	command = 'clang++ -std=c++1y -pedantic -Wall -Werror -ggdb -O0 -fPIC -shared -o luxem-cxx.so ' .. table.concat(inputs, ' ')
}
