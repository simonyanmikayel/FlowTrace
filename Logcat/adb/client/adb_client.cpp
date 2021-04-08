/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TRACE_TAG ADB

#include "sysdeps.h"
#include "adb_client.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/thread_annotations.h>
#include <cutils/sockets.h>

#include "adb_io.h"
#include "adb_utils.h"
#include "socket_spec.h"
#include "sysdeps/chrono.h"
#include "adb_commandline.h"

static TransportType __adb_transport = kTransportAny;
static const char* __adb_serial = nullptr;
static TransportId __adb_transport_id = 0;
static const char* __adb_server_socket_spec;

#if _BUILD_ALL1
void adb_set_transport(TransportType type, const char* serial, TransportId transport_id) {
    __adb_transport = type;
    __adb_serial = serial;
    __adb_transport_id = transport_id;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
void adb_get_transport(TransportType* type, const char** serial, TransportId* transport_id) {
    if (type) *type = __adb_transport;
    if (serial) *serial = __adb_serial;
    if (transport_id) *transport_id = __adb_transport_id;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
void adb_set_socket_spec(const char* socket_spec) {
    if (__adb_server_socket_spec) {
        //TODO LOG(FATAL) << "attempted to reinitialize adb_server_socket_spec " << socket_spec << " (was " << __adb_server_socket_spec << ")";
    }
    __adb_server_socket_spec = socket_spec;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
static int switch_socket_transport(int fd, std::string* error) {
    std::string service;
    if (__adb_transport_id) {
        service += "host:transport-id:";
        service += std::to_string(__adb_transport_id);
    } else if (__adb_serial) {
        service += "host:transport:";
        service += __adb_serial;
    } else {
        const char* transport_type = "???";
        switch (__adb_transport) {
          case kTransportUsb:
            transport_type = "transport-usb";
            break;
          case kTransportLocal:
            transport_type = "transport-local";
            break;
          case kTransportAny:
            transport_type = "transport-any";
            break;
          case kTransportHost:
            // no switch necessary
            return 0;
        }
        service += "host:";
        service += transport_type;
    }

    if (!SendProtocolString(fd, service)) {
        *error = perror_str("write failure during connection");
        return -1;
    }
    D("Switch transport in progress");

    if (!adb_status(fd, error)) {
        D("Switch transport failed: %s", error->c_str());
        return -1;
    }
    D("Switch transport success");
    return 0;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
bool adb_status(int fd, std::string* error) {
    char buf[5];
    if (!ReadFdExactly(fd, buf, 4)) {
        *error = perror_str("protocol fault (couldn't read status)");
        return false;
    }

    if (!memcmp(buf, "OKAY", 4)) {
        return true;
    }

    if (memcmp(buf, "FAIL", 4)) {
        *error = android::base::StringPrintf("protocol fault (status %02x %02x %02x %02x?!)",
                                             buf[0], buf[1], buf[2], buf[3]);
        return false;
    }

    ReadProtocolString(fd, error, error);
    return false;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
static int _adb_connect(const std::string& service, std::string* error) {
    D("_adb_connect: %s", service.c_str());
    if (service.empty() || service.size() > MAX_PAYLOAD) {
        *error = android::base::StringPrintf("bad service name length (%zd)",
                                             service.size());
        return -1;
    }

    std::string reason;
    unique_fd fd;
    if (!socket_spec_connect(&fd, __adb_server_socket_spec, nullptr, nullptr, &reason)) {
        *error = android::base::StringPrintf("cannot connect to daemon at %s: %s",
                                             __adb_server_socket_spec, reason.c_str());
        return -2;
    }

    if (memcmp(&service[0], "host", 4) != 0 && switch_socket_transport(fd.get(), error)) {
        return -1;
    }

    if (!SendProtocolString(fd.get(), service)) {
        *error = perror_str("write failure during connection");
        return -1;
    }

    if (!adb_status(fd.get(), error)) {
        return -1;
    }

    D("_adb_connect: return fd %d", fd.get());
    return fd.release();
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
bool adb_kill_server() {
    D("adb_kill_server");
    std::string reason;
    unique_fd fd;
    if (!socket_spec_connect(&fd, __adb_server_socket_spec, nullptr, nullptr, &reason)) {
        fprintf(stderr, "cannot connect to daemon at %s: %s\n", __adb_server_socket_spec,
                reason.c_str());
        return true;
    }

    if (!SendProtocolString(fd.get(), "host:kill")) {
        fprintf(stderr, "error: write failure during connection: %s\n", strerror(errno));
        return false;
    }

    // The server might send OKAY, so consume that.
    char buf[4];
    ReadFdExactly(fd.get(), buf, 4);
    // Now that no more data is expected, wait for socket orderly shutdown or error, indicating
    // server death.
    ReadOrderlyShutdown(fd.get());
    return true;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
static int orig_adb_connect(const std::string& service, std::string* error) {
    // first query the adb server's version
    unique_fd fd(_adb_connect("host:version", error));


    D("adb_connect: service %s", service.c_str());
    if (fd == -2 && !is_local_socket_spec(__adb_server_socket_spec)) {
        fprintf(stderr, "* cannot start server on remote host\n");
        // error is the original network connection error
        return fd;
    } else if (fd == -2) {
		//fprintf(stderr, "** daemon not running; starting now at %s\n", __adb_server_socket_spec);
		fprintf(stderr, "** daemon not running at %s; starting it by calling 'adb start-server' \n", __adb_server_socket_spec);
		return -1;
		/* !!TODO!!
	start_server:
        if (launch_server(__adb_server_socket_spec)) {
            fprintf(stderr, "* failed to start daemon\n");
            // launch_server() has already printed detailed error info, so just
            // return a generic error string about the overall adb_connect()
            // that the caller requested.
            *error = "cannot connect to daemon";
            return -1;
        } else {
            fprintf(stderr, "* daemon started successfully\n");
        }
        // The server will wait until it detects all of its connected devices before acking.
        // Fall through to _adb_connect.
		*/
    } else {
        // If a server is already running, check its version matches.
        int version = ADB_SERVER_VERSION - 1;

        // If we have a file descriptor, then parse version result.
        if (fd >= 0) {
            std::string version_string;
            if (!ReadProtocolString(fd, &version_string, error)) {
                return -1;
            }

            ReadOrderlyShutdown(fd);

            if (sscanf(&version_string[0], "%04x", &version) != 1) {
                *error = android::base::StringPrintf("cannot parse version string: %s",
                                                     version_string.c_str());
                return -1;
            }
        } else {
            // If fd is -1 check for "unknown host service" which would
            // indicate a version of adb that does not support the
            // version command, in which case we should fall-through to kill it.
            if (*error != "unknown host service") {
                return fd;
            }
        }
#ifdef _DEBUG
        if (version != ADB_SERVER_VERSION) {
            fprintf(stderr, "adb server version (%d) doesn't match this client (%d); killing...\n",
                    version, ADB_SERVER_VERSION);
   //         adb_kill_server();
			//return -1;
			//!!TODO!! goto start_server;
        }
#endif

    }

    // if the command is start-server, we are done.
    if (service == "host:start-server") {
        return 0;
    }

    fd.reset(_adb_connect(service, error));
    if (fd == -1) {
        D("_adb_connect error: %s", error->c_str());
    } else if(fd == -2) {
        fprintf(stderr, "* daemon still not running\n");
    }
    D("adb_connect: return fd %d", fd.get());

    return fd.release();
}

int adb_connect(const std::string& service, std::string* error) {
	int serverFd = orig_adb_connect(service, error);
	if (serverFd == -1 && gAdbProp.restartAdbIfNeeded) {
		//start server and try again
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		wchar_t cmd[32];
		wcscpy_s(cmd, L"adb.exe start-server");
		if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi)) {
			D("successfully run of 'adb start-server'");
			WaitForSingleObject(pi.hProcess, 5000);
			// Close the handles.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			serverFd = orig_adb_connect(service, error);
		}
		else {
			LPVOID lpMsgBuf;
			DWORD dw = GetLastError();
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
			LPTSTR strError = (LPTSTR)lpMsgBuf;
			// Display the error
			// Free resources created by the system
			LocalFree(lpMsgBuf);
		}
	}
	return serverFd;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
bool adb_command(const std::string& service) {
    std::string error;
    unique_fd fd(adb_connect(service, &error));
    if (fd < 0) {
        fprintf(stderr, "error: %s\n", error.c_str());
        return false;
    }

    if (!adb_status(fd.get(), &error)) {
        fprintf(stderr, "error: %s\n", error.c_str());
        return false;
    }

    ReadOrderlyShutdown(fd.get());
    return true;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
bool adb_query(const std::string& service, std::string* result, std::string* error) {
    D("adb_query: %s", service.c_str());
    unique_fd fd(adb_connect(service, error));
    if (fd < 0) {
        return false;
    }

    result->clear();
    if (!ReadProtocolString(fd.get(), result, error)) {
        return false;
    }

    ReadOrderlyShutdown(fd.get());
    return true;
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
std::string format_host_command(const char* command) {
    if (__adb_transport_id) {
        return android::base::StringPrintf("host-transport-id:%" PRIu64 ":%s", __adb_transport_id,
                                           command);
    } else if (__adb_serial) {
        return android::base::StringPrintf("host-serial:%s:%s", __adb_serial, command);
    }

    const char* prefix = "host";
    if (__adb_transport == kTransportUsb) {
        prefix = "host-usb";
    } else if (__adb_transport == kTransportLocal) {
        prefix = "host-local";
    }
    return android::base::StringPrintf("%s:%s", prefix, command);
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
bool adb_get_feature_set(FeatureSet* feature_set, std::string* error) {
    std::string result;
    if (adb_query(format_host_command("features"), &result, error)) {
        *feature_set = StringToFeatureSet(result);
        return true;
    }
    feature_set->clear();
    return false;
}
#endif //_BUILD_ALL
