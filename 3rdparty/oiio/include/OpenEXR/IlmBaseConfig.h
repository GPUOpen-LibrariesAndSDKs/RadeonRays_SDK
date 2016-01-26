// Hand-crafted version of config/IlmBaseConfig.h for Windows
//
// Define and set to 1 if the target system has POSIX thread support
// and you want IlmBase to use it for multithreaded file I/O.
//

#undef HAVE_PTHREAD

//
// Define and set to 1 if the target system supports POSIX semaphores
// and you want OpenEXR to use them; otherwise, OpenEXR will use its
// own semaphore implementation.
//

#undef HAVE_POSIX_SEMAPHORES

#define ILMBASE_INTERNAL_NAMESPACE_CUSTOM 1
#define IMATH_INTERNAL_NAMESPACE Imath_2_1
#define IEX_INTERNAL_NAMESPACE Iex_2_1
#define ILMTHREAD_INTERNAL_NAMESPACE IlmThread_2_1
#define IMATH_NAMESPACE Imath
#define IEX_NAMESPACE Iex
#define ILMTHREAD_NAMESPACE IlmThread
#define ILMBASE_VERSION_STRING "2.1.0"
#define ILMBASE_PACKAGE_STRING "IlmBase 2.1.0"
#define ILMBASE_VERSION_MAJOR 2
#define ILMBASE_VERSION_MINOR 1
#define ILMBASE_VERSION_PATCH 0

// Version as a single hex number, e.g. 0x01000300 == 1.0.3
#define ILMBASE_VERSION_HEX ((ILMBASE_VERSION_MAJOR << 24) | \
                             (ILMBASE_VERSION_MINOR << 16) | \
                             (ILMBASE_VERSION_PATCH <<  8))
