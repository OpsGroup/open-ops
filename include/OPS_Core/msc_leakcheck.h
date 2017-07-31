//блок проверки утечек памяти
//этот блок надо вставлять в конце всех include 
//и вызывать при завершении программы функцию _CrtDumpMemoryLeaks();
//тогда в окне output в Visual Studio будет указаны все 
//неосвобожденные блоки памяти и имена файлов, в которых эта память выделялась 
//(если в эти файлы был включен этот текст)
#if 0 // This is deprecated now
#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif
#endif
