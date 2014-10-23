DoOnce 'library/c/Tupfile.lua'

local Compiler = Define.Library
{
	Name = 'luxem-cxx',
	Sources = Item 'read.cxx' + 'write.cxx' + 'struct.cxx',
	Objects = LuxemCObjects,
}

