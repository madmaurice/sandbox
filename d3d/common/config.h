#ifndef _INCGUARD_CONFIG_H
#define _INCGUARD_CONFIG_H

#include <assert.h>

#define OC_ASSERT(x) _ASSERTE(x)

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Enable run-time memory check for debug builds.
// And configure assert report
// link: http://msdn.microsoft.com/en-US/library/8hyw4sy7(v=vs.80).aspx
#define OC_DBG_CONFIG() \
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); \
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
#else
#define OC_DBG_CONFIG()
#endif

#endif // _INCGUARD_CONFIG_H