extern bool bios_debug;
#define TRACE(fmt, ...) if (bios_debug) { printf(fmt "\r\n", ## __VA_ARGS__); }
// build with this instead, to disable trace for performance.
// #define TRACE(...) ;
