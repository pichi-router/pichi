#ifndef PICHI_H
#define PICHI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
 * Start PICHI server according to
 *   - bind: server listening address, NOT NULL,
 *   - port: server listening port,
 *   - mmdb: IP GEO database, MMDB format, NOT NULL.
 * The function doesn't return if no error occurs, otherwise -1.
 */
extern int pichi_run_server(char const* bind, uint16_t port, char const* mmdb);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PICHI_H
