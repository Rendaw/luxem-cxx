DoOnce 'library/c/Tupfile.lua'

LuxemCXX = Define.Library
{
	Name = 'luxem-cxx',
	Sources = Item 'read.cxx' + 'write.cxx' + 'struct.cxx',
	Objects = LuxemCObjects,
}

