#ifndef PICHI_H
#define PICHI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef WIN32
#define GLOBAL __declspec(dllimport)
#else // WIN32
#define GLOBAL
#endif // WIN32

GLOBAL extern char const* PICHI_DEFAULT_BIND;
GLOBAL extern char const* PICHI_DEFAULT_MMDB;

/*
 * Start PICHI server according to
 *   - bind: server listening address, PICHI_DEFAULT_BIND if NULL,
 *   - port: server listening port,
 *   - mmdb: IP GEO database, MMDB format, PICHI_DEFAULT_MMDB if NULL.
 * The function doesn't return if no error occurs, otherwise -1.
 */
extern int pichi_run_server(char const* bind, uint16_t port, char const* mmdb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PICHI_H
