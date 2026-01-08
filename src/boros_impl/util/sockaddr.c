/* This source file is part of the boros project. */
/* SPDX-License-Identifier: ISC */

#include "util/sockaddr.h"

#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

/* Helpers functions for parsing different address families from Python values. */
static int parse_inet(PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len);
static int parse_inet6(PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len);
static int parse_unix(PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len);

/* Helper functions for formatting different address families to Python values. */
static PyObject *format_inet(const struct sockaddr *addr);
static PyObject *format_inet6(const struct sockaddr *addr);
static PyObject *format_unix(const struct sockaddr *addr, socklen_t len);

int parse_sockaddr(int af, PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len) {
    assert(addrobj != NULL && out != NULL && len != NULL);

    switch (af) {
    case AF_INET:
        return parse_inet(addrobj, out, len);
    case AF_INET6:
        return parse_inet6(addrobj, out, len);
    case AF_UNIX:
        return parse_unix(addrobj, out, len);

    default:
        PyErr_Format(PyExc_ValueError, "Unsupported address family: %d", af);
        return -1;
    }
}

PyObject *format_sockaddr(const struct sockaddr *addr, socklen_t len) {
    assert(addr != NULL);

    switch (addr->sa_family) {
    case AF_INET:
        return format_inet(addr);
    case AF_INET6:
        return format_inet6(addr);
    case AF_UNIX:
        return format_unix(addr, len);

    default:
        PyErr_Format(PyExc_ValueError, "Unsupported address family: %d", addr->sa_family);
        return NULL;
    }
}

static inline bool extract_hostaddr(PyObject *hostobj, const void **hostbuf, size_t *hostlen) {
    if (PyUnicode_Check(hostobj) && PyUnicode_IS_COMPACT_ASCII(hostobj)) {
        *hostbuf = PyUnicode_DATA(hostobj);
        *hostlen = PyUnicode_GET_LENGTH(hostobj);
    } else if (PyBytes_Check(hostobj)) {
        *hostbuf = PyBytes_AS_STRING(hostobj);
        *hostlen = PyBytes_GET_SIZE(hostobj);
    } else if (PyByteArray_Check(hostobj)) {
        *hostbuf = PyByteArray_AS_STRING(hostobj);
        *hostlen = PyByteArray_GET_SIZE(hostobj);
    } else {
        PyErr_Format(PyExc_TypeError, "expected ASCII str, bytes or bytearray, not %.500s", Py_TYPE(hostobj)->tp_name);
        return false;
    }

    if (strlen(*hostbuf) != *hostlen) {
        PyErr_SetString(PyExc_ValueError, "host address must not contain null characters");
        return false;
    }

    return true;
}

static inline bool extract_port(PyObject *portobj, unsigned short *port) {
    int value;
    if (!python_parse_int(&value, portobj)) {
        return false;
    }

    if (value < 0 || value > 0xFFFF) {
        PyErr_SetString(PyExc_OverflowError, "port must be 0-65535");
        return false;
    }

    *port = value;
    return true;
}

static int parse_inet(PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len) {
    if (!PyTuple_Check(addrobj) || PyTuple_GET_SIZE(addrobj) != 2) {
        PyErr_Format(PyExc_TypeError, "AF_INET address must be a pair (host, port), not %.500s",
                     Py_TYPE(addrobj)->tp_name);
        return -1;
    }

    PyObject *hostobj = PyTuple_GET_ITEM(addrobj, 0);
    PyObject *portobj = PyTuple_GET_ITEM(addrobj, 1);

    const void *host;
    size_t hostlen;
    if (!extract_hostaddr(hostobj, &host, &hostlen)) {
        return -1;
    }

    unsigned short port;
    if (!extract_port(portobj, &port)) {
        return -1;
    }

    struct sockaddr_in *sin = (struct sockaddr_in *)out;
    memset(sin, 0, sizeof(*sin));

    if (inet_pton(AF_INET, host, &sin->sin_addr) != 1) {
        PyErr_Format(PyExc_ValueError, "invalid IPv4 address for AF_INET: %s", host);
        return -1;
    }
    sin->sin_family = AF_INET;
    sin->sin_port   = htons(port);

    *len = sizeof(*sin);
    return 0;
}

static int parse_inet6(PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len) {
    if (!PyTuple_Check(addrobj) || PyTuple_GET_SIZE(addrobj) < 2 || PyTuple_GET_SIZE(addrobj) > 4) {
        PyErr_Format(PyExc_TypeError, "AF_INET6 address must be a tuple (host, port, flowinfo?, scope_id?), not %.500s",
                     Py_TYPE(addrobj)->tp_name);
        return -1;
    }

    PyObject *hostobj = PyTuple_GET_ITEM(addrobj, 0);
    PyObject *portobj = PyTuple_GET_ITEM(addrobj, 1);

    const void *host;
    size_t hostlen;
    if (!extract_hostaddr(hostobj, &host, &hostlen)) {
        return -1;
    }

    unsigned short port;
    if (!extract_port(portobj, &port)) {
        return -1;
    }

    unsigned int flowinfo = 0;
    if (PyTuple_GET_SIZE(addrobj) > 2 && !python_parse_unsigned_int(&flowinfo, PyTuple_GET_ITEM(addrobj, 2))) {
        return -1;
    }

    unsigned int scope_id = 0;
    if (PyTuple_GET_SIZE(addrobj) > 3 && !python_parse_unsigned_int(&scope_id, PyTuple_GET_ITEM(addrobj, 3))) {
        return -1;
    }

    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)out;
    memset(sin6, 0, sizeof(*sin6));

    if (inet_pton(AF_INET6, host, &sin6->sin6_addr) != 1) {
        PyErr_Format(PyExc_ValueError, "invalid IPv6 address for AF_INET6: %s", host);
        return -1;
    }
    sin6->sin6_family   = AF_INET6;
    sin6->sin6_port     = htons(port);
    sin6->sin6_flowinfo = htonl(flowinfo);
    sin6->sin6_scope_id = htonl(scope_id);

    *len = sizeof(*sin6);
    return 0;
}

static int parse_unix(PyObject *addrobj, struct sockaddr_storage *out, socklen_t *len) {
    /* Encode the addr using PEP 383 filesystem encoding, if applicable. */
    if (PyUnicode_Check(addrobj)) {
        addrobj = PyUnicode_EncodeFSDefault(addrobj);
        if (addrobj == NULL) {
            return -1;
        }
    } else {
        Py_INCREF(addrobj);
    }

    /* Extract a raw buffer from the address. */
    Py_buffer pathbuf;
    if (PyObject_GetBuffer(addrobj, &pathbuf, 0) != 0) {
        Py_DECREF(addrobj);
        return -1;
    }

    struct sockaddr_un *sun = (struct sockaddr_un *)out;
    memset(sun, 0, sizeof(*sun));
    int res = -1;

    /*
     * Abstract namespaces are a Linux extension where paths start with
     * a null byte and are not null-terminated. We have to account for
     * that in sun_path length calculations.
     */
    int null_terminated = (pathbuf.len != 0 && *(const char *)pathbuf.buf != '\0');
    if ((size_t)pathbuf.len + null_terminated > sizeof(sun->sun_path)) {
        PyErr_SetString(PyExc_OSError, "AF_UNIX path too long");
        goto cleanup;
    }

    /* Populate the sockaddr object. */
    sun->sun_family = AF_UNIX;
    memcpy(sun->sun_path, pathbuf.buf, pathbuf.len);
    *len = pathbuf.len + offsetof(struct sockaddr_un, sun_path) + null_terminated;

    res = 0;
cleanup:
    PyBuffer_Release(&pathbuf);
    Py_DECREF(addrobj);
    return res;
}

static PyObject *format_inet(const struct sockaddr *addr) {
    const struct sockaddr_in *sin = (const struct sockaddr_in *)addr;

    char ipv4[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &sin->sin_addr, ipv4, sizeof(ipv4)) == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    return Py_BuildValue("(sH)", ipv4, ntohs(sin->sin_port));
}

static PyObject *format_inet6(const struct sockaddr *addr) {
    const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)addr;

    char ipv6[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &sin6->sin6_addr, ipv6, sizeof(ipv6)) == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    unsigned short port   = ntohs(sin6->sin6_port);
    unsigned int flowinfo = ntohl(sin6->sin6_flowinfo);
    unsigned int scope_id = ntohl(sin6->sin6_scope_id);

    return Py_BuildValue("(sHII)", ipv6, port, flowinfo, scope_id);
}

static PyObject *format_unix(const struct sockaddr *addr, socklen_t len) {
    const struct sockaddr_un *sun = (const struct sockaddr_un *)addr;

    size_t path_len = len - offsetof(struct sockaddr_un, sun_path);
    if (path_len != 0 && sun->sun_path[0] != '\0') {
        --path_len;
    }

    return PyUnicode_DecodeFSDefaultAndSize(sun->sun_path, path_len);
}
