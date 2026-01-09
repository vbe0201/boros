/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#pragma once

#include "util/python.h"

#include <sys/socket.h>

/*
 * Converts a Python address object to a sockaddr structure.
 *
 * Supported address formats:
 * - AF_INET: (host, port) tuple
 * - AF_INET6: (host, port, flowinfo, scope_id) tuple
 * - AF_UNIX: path object for UNIX domain sockets
 */
bool parse_sockaddr(int af, PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len);

/* Converts a sockaddr to a Python address object. */
PyObject *format_sockaddr(const struct sockaddr *addr, socklen_t len);
