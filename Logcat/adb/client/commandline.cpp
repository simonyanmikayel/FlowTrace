/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#if !defined(_WIN32)
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "adb.h"
#include "adb_auth.h"
#include "adb_client.h"
#include "adb_install.h"
#include "adb_io.h"
#include "adb_unique_fd.h"
#include "adb_utils.h"
#include "bugreport.h"
#include "client/file_sync_client.h"
#include "commandline.h"
#include "fastdeploy.h"
#include "services.h"
#include "shell_protocol.h"
#include "sysdeps/chrono.h"

#if _BUILD_ALL1
extern int gListenAll;
DefaultStandardStreamsCallback DEFAULT_STANDARD_STREAMS_CALLBACK(nullptr, nullptr);
#endif //_BUILD_ALL

//#if _BUILD_ALL
// Reads from |fd| and prints received data. If |use_shell_protocol| is true
// this expects that incoming data will use the shell protocol, in which case
// stdout/stderr are routed independently and the remote exit code will be
// returned.
// if |callback| is non-null, stdout/stderr output will be handled by it.
int read_and_dump(int fd, bool use_shell_protocol = false,
	StandardStreamsCallbackInterface* callback = &DEFAULT_STANDARD_STREAMS_CALLBACK) {
	int exit_code = 0;
	if (fd < 0) return exit_code;

	std::unique_ptr<ShellProtocol> protocol;
	int length = 0;

	static char local_buffer[4 * 512]; //512
	char* buffer_ptr;
	size_t buffer_size;

	if (use_shell_protocol) {
		protocol = std::make_unique<ShellProtocol>(fd);
		if (!protocol) {
			LOG(ERROR) << "failed to allocate memory for ShellProtocol object";
			return 1;
		}
		buffer_ptr = protocol->data();
	}

	while (true) {
		callback->GetStreamBuf(&buffer_ptr, &buffer_size);
		if (!buffer_ptr || !buffer_size) {
			buffer_size = sizeof(local_buffer);
			buffer_ptr = local_buffer;
		}
		if (use_shell_protocol) {
			if (!protocol->Read()) {
				break;
			}
			length = (int)(protocol->data_length());
			switch (protocol->id()) {
			case ShellProtocol::kIdStdout:
				callback->OnStdout(buffer_ptr, length);
				break;
			case ShellProtocol::kIdStderr:
				callback->OnStderr(buffer_ptr, length);
				break;
			case ShellProtocol::kIdExit:
				exit_code = protocol->data()[0];
				continue;
			default:
				continue;
			}
			length = (int)(protocol->data_length());
		}
		else {
			D("read_and_dump(): pre adb_read(fd=%d)", fd);
			length = adb_read(fd, buffer_ptr, (int)(buffer_size));
			D("read_and_dump(): post adb_read(fd=%d): length=%d", fd, length);
			if (length <= 0) {
				break;
			}
			if (!callback->OnStdout(buffer_ptr, length))
				break;
		}
	}

	return callback->Done(exit_code);
}
//#endif //_BUILD_ALL

#if _BUILD_ALL1
static bool wait_for_device(const char* service) {
	std::vector<std::string> components = android::base::Split(service, "-");
	if (components.size() < 3 || components.size() > 4) {
		fprintf(stderr, "adb: couldn't parse 'wait-for' command: %s\n", service);
		return false;
	}

	TransportType t;
	adb_get_transport(&t, nullptr, nullptr);

	// Was the caller vague about what they'd like us to wait for?
	// If so, check they weren't more specific in their choice of transport type.
	if (components.size() == 3) {
		auto it = components.begin() + 2;
		if (t == kTransportUsb) {
			components.insert(it, "usb");
		}
		else if (t == kTransportLocal) {
			components.insert(it, "local");
		}
		else {
			components.insert(it, "any");
		}
	}
	else if (components[2] != "any" && components[2] != "local" && components[2] != "usb") {
		fprintf(stderr, "adb: unknown type %s; expected 'any', 'local', or 'usb'\n",
			components[2].c_str());
		return false;
	}

	if (components[3] != "any" && components[3] != "bootloader" && components[3] != "device" &&
		components[3] != "recovery" && components[3] != "sideload") {
		fprintf(stderr,
			"adb: unknown state %s; "
			"expected 'any', 'bootloader', 'device', 'recovery', or 'sideload'\n",
			components[3].c_str());
		return false;
	}

	std::string cmd = format_host_command(android::base::Join(components, "-").c_str());
	return adb_command(cmd);
}
#endif //_BUILD_ALL

//#if _BUILD_ALL
// Returns a shell service string with the indicated arguments and command.
std::string ShellServiceString(bool use_shell_protocol,
	const std::string& type_arg,
	const std::string& command) {
	std::vector<std::string> args;
	if (use_shell_protocol) {
		args.push_back(kShellServiceArgShellProtocol);

		const char* terminal_type = getenv("TERM");
		if (terminal_type != nullptr) {
			args.push_back(std::string("TERM=") + terminal_type);
		}
	}
	if (!type_arg.empty()) {
		args.push_back(type_arg);
	}

	// Shell service string can look like: shell[,arg1,arg2,...]:[command].
	return android::base::StringPrintf("shell%s%s:%s",
		args.empty() ? "" : ",",
		android::base::Join(args, ',').c_str(),
		command.c_str());
}
//#endif //_BUILD_ALL

//#if _BUILD_ALL
int send_shell_command(const std::string& command, StandardStreamsCallbackInterface* callback, bool disable_shell_protocol	) {
	unique_fd fd;
	bool use_shell_protocol = false;
	if (callback)
		callback->serverFd = -1;

	while (true) {
		bool attempt_connection = true;

		// Use shell protocol if it's supported and the caller doesn't explicitly
		// disable it.
		if (!disable_shell_protocol) {
			FeatureSet features;
			std::string error;
			if (adb_get_feature_set(&features, &error)) {
				use_shell_protocol = CanUseFeature(features, kFeatureShell2);
			}
			else {
				// Device was unreachable.
				attempt_connection = false;
			}
		}

		if (attempt_connection) {
			std::string error;
			std::string service_string = ShellServiceString(use_shell_protocol, "", command);

			fd.reset(adb_connect(service_string, &error));
			if (fd >= 0) {
				break;
			}
		}

		fprintf(stderr, "- waiting for device -\n");
		if (!wait_for_device("wait-for-device")) {
			return 1;
		}
	}

	if (callback)
		callback->serverFd = fd.get();
	return read_and_dump(fd.get(), use_shell_protocol, callback);
}
//#endif //_BUILD_ALL

#if _BUILD_ALL
static int RemoteShell(bool use_shell_protocol, const std::string& type_arg, char escape_char,
	bool empty_command, const std::string& service_string) {
	// Old devices can't handle a service string that's longer than MAX_PAYLOAD_V1.
	// Use |use_shell_protocol| to determine whether to allow a command longer than that.
	if (service_string.size() > MAX_PAYLOAD_V1 && !use_shell_protocol) {
		fprintf(stderr, "error: shell command too long\n");
		return 1;
	}

	// Make local stdin raw if the device allocates a PTY, which happens if:
	//   1. We are explicitly asking for a PTY shell, or
	//   2. We don't specify shell type and are starting an interactive session.
	bool raw_stdin = (type_arg == kShellServiceArgPty || (type_arg.empty() && empty_command));

	std::string error;
	int fd = adb_connect(service_string, &error);
	if (fd < 0) {
		fprintf(stderr, "error: %s\n", error.c_str());
		return 1;
	}

	StdinReadArgs* args = new StdinReadArgs;
	if (!args) {
		LOG(ERROR) << "couldn't allocate StdinReadArgs object";
		return 1;
	}
	args->stdin_fd = STDIN_FILENO;
	args->write_fd = fd;
	args->raw_stdin = raw_stdin;
	args->escape_char = escape_char;
	if (use_shell_protocol) {
		args->protocol = std::make_unique<ShellProtocol>(args->write_fd);
	}

	if (raw_stdin) stdin_raw_init();

#if !defined(_WIN32)
	// Ensure our process is notified if the local window size changes.
	// We use sigaction(2) to ensure that the SA_RESTART flag is not set,
	// because the whole reason we're sending signals is to unblock the read(2)!
	// That also means we don't need to do anything in the signal handler:
	// the side effect of delivering the signal is all we need.
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = [](int) {};
	sa.sa_flags = 0;
	sigaction(SIGWINCH, &sa, nullptr);

	// Now block SIGWINCH in this thread (the main thread) and all threads spawned
	// from it. The stdin read thread will unblock this signal to ensure that it's
	// the thread that receives the signal.
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGWINCH);
	pthread_sigmask(SIG_BLOCK, &mask, nullptr);
#endif

	// TODO: combine read_and_dump with stdin_read_thread to make life simpler?
	std::thread(stdin_read_thread_loop, args).detach();
	int exit_code = read_and_dump(fd, use_shell_protocol);

	// TODO: properly exit stdin_read_thread_loop and close |fd|.

	// TODO: we should probably install signal handlers for this.
	// TODO: can we use atexit? even on Windows?
	if (raw_stdin) stdin_raw_restore();

	return exit_code;
}
static int adb_shell(int argc, const char** argv) {
	FeatureSet features;
	std::string error_message;
	if (!adb_get_feature_set(&features, &error_message)) {
		fprintf(stderr, "error: %s\n", error_message.c_str());
		return 1;
	}

	enum PtyAllocationMode { kPtyAuto, kPtyNo, kPtyYes, kPtyDefinitely };

	// Defaults.
	char escape_char = '~'; // -e
	bool use_shell_protocol = CanUseFeature(features, kFeatureShell2); // -x
	PtyAllocationMode tty = use_shell_protocol ? kPtyAuto : kPtyDefinitely; // -t/-T

	// Parse shell-specific command-line options.
	argv[0] = "adb shell"; // So getopt(3) error messages start "adb shell".
#ifdef _WIN32
	// fixes "adb shell -l" crash on Windows, b/37284906
	__argv = const_cast<char**>(argv);
#endif
	optind = 1; // argv[0] is always "shell", so set `optind` appropriately.
	int opt;
	while ((opt = getopt(argc, const_cast<char**>(argv), "+e:ntTx")) != -1) {
		switch (opt) {
		case 'e':
			if (!(strlen(optarg) == 1 || strcmp(optarg, "none") == 0)) {
				error_exit("-e requires a single-character argument or 'none'");
			}
			escape_char = (strcmp(optarg, "none") == 0) ? 0 : optarg[0];
			break;
		case 'n':
			close_stdin();
			break;
		case 'x':
			// This option basically asks for historical behavior, so set options that
			// correspond to the historical defaults. This is slightly weird in that -Tx
			// is fine (because we'll undo the -T) but -xT isn't, but that does seem to
			// be our least worst choice...
			use_shell_protocol = false;
			tty = kPtyDefinitely;
			escape_char = '~';
			break;
		case 't':
			// Like ssh, -t arguments are cumulative so that multiple -t's
			// are needed to force a PTY.
			tty = (tty >= kPtyYes) ? kPtyDefinitely : kPtyYes;
			break;
		case 'T':
			tty = kPtyNo;
			break;
		default:
			// getopt(3) already printed an error message for us.
			return 1;
		}
	}

	bool is_interactive = (optind == argc);

	std::string shell_type_arg = kShellServiceArgPty;
	if (tty == kPtyNo) {
		shell_type_arg = kShellServiceArgRaw;
	}
	else if (tty == kPtyAuto) {
		// If stdin isn't a TTY, default to a raw shell; this lets
		// things like `adb shell < my_script.sh` work as expected.
		// Non-interactive shells should also not have a pty.
		if (!unix_isatty(STDIN_FILENO) || !is_interactive) {
			shell_type_arg = kShellServiceArgRaw;
		}
	}
	else if (tty == kPtyYes) {
		// A single -t arg isn't enough to override implicit -T.
		if (!unix_isatty(STDIN_FILENO)) {
			fprintf(stderr,
				"Remote PTY will not be allocated because stdin is not a terminal.\n"
				"Use multiple -t options to force remote PTY allocation.\n");
			shell_type_arg = kShellServiceArgRaw;
		}
	}

	D("shell -e 0x%x t=%d use_shell_protocol=%s shell_type_arg=%s\n",
		escape_char, tty,
		use_shell_protocol ? "true" : "false",
		(shell_type_arg == kShellServiceArgPty) ? "pty" : "raw");

	// Raw mode is only supported when talking to a new device *and* using the shell protocol.
	if (!use_shell_protocol) {
		if (shell_type_arg != kShellServiceArgPty) {
			fprintf(stderr, "error: %s only supports allocating a pty\n",
				!CanUseFeature(features, kFeatureShell2) ? "device" : "-x");
			return 1;
		}
		else {
			// If we're not using the shell protocol, the type argument must be empty.
			shell_type_arg = "";
		}
	}

	std::string command;
	if (optind < argc) {
		// We don't escape here, just like ssh(1). http://b/20564385.
		command = android::base::Join(std::vector<const char*>(argv + optind, argv + argc), ' ');
	}

	std::string service_string = ShellServiceString(use_shell_protocol, shell_type_arg, command);
	return RemoteShell(use_shell_protocol, shell_type_arg, escape_char, command.empty(),
		service_string);
}
#endif //_BUILD_ALL

static int logcat(int argc, const char** argv, StandardStreamsCallbackInterface* callback = nullptr) {
	if (!argc)
		return 1;
	char* log_tags = getenv("ANDROID_LOG_TAGS");
	std::string quoted = escape_arg(log_tags == nullptr ? "" : log_tags);

	std::string cmd = "export ANDROID_LOG_TAGS=\"" + quoted + "\"; exec logcat";

	if (!strcmp(argv[0], "longcat")) {
		cmd += " -v long";
	}

	--argc;
	++argv;
	while (argc-- > 0) {
		cmd += " " + escape_arg(*argv++);
	}

	return send_shell_command(cmd, callback ? callback : &DEFAULT_STANDARD_STREAMS_CALLBACK);
}

static int cmd_shell(int argc, const char** argv, StandardStreamsCallbackInterface* callback = nullptr) {
	if (!argc)
		return 1;

	std::string cmd = "exec ";

	--argc;
	++argv;
	while (argc-- > 0) {
		cmd += " " + escape_arg(*argv++);
	}

	return send_shell_command(cmd, callback ? callback : &DEFAULT_STANDARD_STREAMS_CALLBACK);
}

static int adb_query_command(const std::string& command) {
	std::string result;
	std::string error;
	if (!adb_query(command, &result, &error)) {
		fprintf(stderr, "error: %s\n", error.c_str());
		return 1;
	}
	printf("%s\n", result.c_str());
	return 0;
}

int adb_commandline(int argc, const char** argv, StandardStreamsCallbackInterface* callback) {
#if _BUILD_ALL
	bool is_server = false;
	bool no_daemon = false;
	bool is_daemon = false;
	int r;
#endif //_BUILD_ALL
	TransportType transport_type = kTransportAny;
	int ack_reply_fd = -1;

#if !defined(_WIN32)
	// We'd rather have EPIPE than SIGPIPE.
	signal(SIGPIPE, SIG_IGN);
#endif

	const char* server_host_str = nullptr;
	const char* server_port_str = nullptr;
	const char* server_socket_str = nullptr;

	// We need to check for -d and -e before we look at $ANDROID_SERIAL.
	const char* serial = nullptr;
	TransportId transport_id = 0;

	while (argc > 0) {
#if _BUILD_ALL
		if (!strcmp(argv[0], "server")) {
			is_server = true;
		}
		else if (!strcmp(argv[0], "nodaemon")) {
			no_daemon = true;
		}
		else if (!strcmp(argv[0], "fork-server")) {
			/* this is a special flag used only when the ADB client launches the ADB Server */
			is_daemon = true;
		}
		else if (!strcmp(argv[0], "--reply-fd")) {
			if (argc < 2) error_exit("--reply-fd requires an argument");
			const char* reply_fd_str = argv[1];
			argc--;
			argv++;
			ack_reply_fd = strtol(reply_fd_str, nullptr, 10);
			if (!_is_valid_ack_reply_fd(ack_reply_fd)) {
				fprintf(stderr, "adb: invalid reply fd \"%s\"\n", reply_fd_str);
				return 1;
			}
		}
		else
#endif //_BUILD_ALL
		if (!strncmp(argv[0], "-s", 2)) {
			if (isdigit(argv[0][2])) {
				serial = argv[0] + 2;
			}
			else {
				if (argc < 2 || argv[0][2] != '\0') { fprintf(stderr, "-s requires an argument\n"); return 1; }
				serial = argv[1];
				argc--;
				argv++;
			}
		}
		else if (!strncmp(argv[0], "-t", 2)) {
			const char* id;
			if (isdigit(argv[0][2])) {
				id = argv[0] + 2;
			}
			else {
				id = argv[1];
				argc--;
				argv++;
			}
			transport_id = strtoll(id, const_cast<char**>(&id), 10);
			if (*id != '\0') {
				fprintf(stderr, "invalid transport id\n"); return 1;
			}
		}
		else if (!strcmp(argv[0], "-d")) {
			transport_type = kTransportUsb;
		}
		else if (!strcmp(argv[0], "-e")) {
			transport_type = kTransportLocal;
		}
		else if (!strcmp(argv[0], "-a")) {
			gListenAll = 1;
		}
		else if (!strncmp(argv[0], "-H", 2)) {
			if (argv[0][2] == '\0') {
				if (argc < 2) { fprintf(stderr, "-H requires an argument\n"); return 1; }
				server_host_str = argv[1];
				argc--;
				argv++;
			}
			else {
				server_host_str = argv[0] + 2;
			}
		}
		else if (!strncmp(argv[0], "-P", 2)) {
			if (argv[0][2] == '\0') {
				if (argc < 2) { fprintf(stderr, "-P requires an argument\n"); return 1; }
				server_port_str = argv[1];
				argc--;
				argv++;
			}
			else {
				server_port_str = argv[0] + 2;
			}
		}
		else if (!strcmp(argv[0], "-L")) {
			if (argc < 2) { fprintf(stderr, "-L requires an argument\n"); return 1; }
			server_socket_str = argv[1];
			argc--;
			argv++;
		}
		else {
			/* out of recognized modifiers and flags */
			break;
		}
		argc--;
		argv++;
	}

	if ((server_host_str || server_port_str) && server_socket_str) {
		fprintf(stderr, "-L is incompatible with -H or -P\n"); return 1; 
	}

	// If -L, -H, or -P are specified, ignore environment variables.
	// Otherwise, prefer ADB_SERVER_SOCKET over ANDROID_ADB_SERVER_ADDRESS/PORT.
	if (!server_host_str && !server_port_str && !server_socket_str) {
		server_socket_str = getenv("ADB_SERVER_SOCKET");
	}

	if (!server_socket_str) {
		// tcp:1234 and tcp:localhost:1234 are different with -a, so don't default to localhost
		server_host_str = server_host_str ? server_host_str : getenv("ANDROID_ADB_SERVER_ADDRESS");

		int server_port = DEFAULT_ADB_PORT;
		server_port_str = server_port_str ? server_port_str : getenv("ANDROID_ADB_SERVER_PORT");
		if (server_port_str && strlen(server_port_str) > 0) {
			if (!android::base::ParseInt(server_port_str, &server_port, 1, 65535)) {
				fprintf(stderr,
					"$ANDROID_ADB_SERVER_PORT must be a positive number less than 65535: "
					"got \"%s\"",
					server_port_str);
			}
		}

		int rc;
		char* temp;
		char temp2[256];
		if (server_host_str) {
			rc = _snprintf(temp2, sizeof(temp2), "tcp:%s:%d", server_host_str, server_port);
		}
		else {
			rc = _snprintf(temp2, sizeof(temp2), "tcp:%d", server_port);
		}
		if (rc < 0) {
			LOG(FATAL) << "failed to allocate server socket specification";
		}
		temp = _strdup(temp2);
		server_socket_str = temp;
	}

	adb_set_socket_spec(server_socket_str);

	// If none of -d, -e, or -s were specified, try $ANDROID_SERIAL.
	if (transport_type == kTransportAny && serial == nullptr) {
		serial = getenv("ANDROID_SERIAL");
	}

	adb_set_transport(transport_type, serial, transport_id);
#if _BUILD_ALL
	if (is_server) {
		if (no_daemon || is_daemon) {
			if (is_daemon && (ack_reply_fd == -1)) {
				fprintf(stderr, "reply fd for adb server to client communication not specified.\n");
				return 1;
			}
			r = adb_server_main(is_daemon, server_socket_str, ack_reply_fd);
		}
		else {
			r = launch_server(server_socket_str);
		}
		if (r) {
			fprintf(stderr, "* could not start server *\n");
		}
		return r;
	}
#endif //_BUILD_ALL
	if (argc == 0) {
		//!!help();
		return 1;
	}

	/* handle wait-for-* prefix */
	if (!strncmp(argv[0], "wait-for-", strlen("wait-for-"))) {
		const char* service = argv[0];

		if (!wait_for_device(service)) {
			return 1;
		}

		// Allow a command to be run after wait-for-device,
		// e.g. 'adb wait-for-device shell'.
		if (argc == 1) {
			return 0;
		}

		/* Fall through */
		argc--;
		argv++;
	}

	//this is only command in this lib for now
	if (!strcmp(argv[0], "logcat") || !strcmp(argv[0], "lolcat") ||
		!strcmp(argv[0], "longcat")) {
		return logcat(argc, argv, callback);
	}
	else if (!strcmp(argv[0], "disconnect")) {

		std::string query = android::base::StringPrintf("host:disconnect:%s",
			(argc == 2) ? argv[1] : "");
		return adb_query_command(query);
	}
	else if (!strcmp(argv[0], "start-server")) {
		std::string error;
		const int result = adb_connect("host:start-server", &error);
		if (result < 0) {
			fprintf(stderr, "error: %s\n", error.c_str());
		}
		return result;
	}
	else if (!strcmp(argv[0], "cmd_shell")) { //added by me
		return cmd_shell(argc, argv, callback);
	}
	fprintf(stderr, "unknown command %s\n", argv[0]);
	return 1;
#if _BUILD_ALL
	/* adb_connect() commands */
	if (!strcmp(argv[0], "devices")) {
		const char *listopt;
		if (argc < 2) {
			listopt = "";
		}
		else if (argc == 2 && !strcmp(argv[1], "-l")) {
			listopt = argv[1];
		}
		else {
			fprintf(stderr, "adb devices [-l]\n"); return 1;
		}

		std::string query = android::base::StringPrintf("host:%s%s", argv[0], listopt);
		printf("List of devices attached\n");
		return adb_query_command(query);
	}
	else if (!strcmp(argv[0], "connect")) {
		if (argc != 2) error_exit("usage: adb connect <host>[:<port>]");

		std::string query = android::base::StringPrintf("host:connect:%s", argv[1]);
		return adb_query_command(query);
	}
	else if (!strcmp(argv[0], "abb")) {
		return adb_abb(argc, argv);
	}
	else if (!strcmp(argv[0], "emu")) {
		return adb_send_emulator_command(argc, argv, serial);
	}
	else if (!strcmp(argv[0], "exec-in") || !strcmp(argv[0], "exec-out")) {
		int exec_in = !strcmp(argv[0], "exec-in");

		if (argc < 2) error_exit("usage: adb %s command", argv[0]);

		std::string cmd = "exec:";
		cmd += argv[1];
		argc -= 2;
		argv += 2;
		while (argc-- > 0) {
			cmd += " " + escape_arg(*argv++);
		}

		std::string error;
		unique_fd fd(adb_connect(cmd, &error));
		if (fd < 0) {
			fprintf(stderr, "error: %s\n", error.c_str());
			return -1;
		}

		if (exec_in) {
			copy_to_file(STDIN_FILENO, fd.get());
		}
		else {
			copy_to_file(fd.get(), STDOUT_FILENO);
		}
		return 0;
	}
	else if (!strcmp(argv[0], "kill-server")) {
		return adb_kill_server() ? 0 : 1;
	}
	else if (!strcmp(argv[0], "sideload")) {
		if (argc != 2) error_exit("sideload requires an argument");
		if (adb_sideload_host(argv[1])) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else if (!strcmp(argv[0], "tcpip")) {
		if (argc != 2) error_exit("tcpip requires an argument");
		int port;
		if (!android::base::ParseInt(argv[1], &port, 1, 65535)) {
			error_exit("tcpip: invalid port: %s", argv[1]);
		}
		return adb_connect_command(android::base::StringPrintf("tcpip:%d", port));
	}
	// clang-format off
	else if (!strcmp(argv[0], "remount") ||
		!strcmp(argv[0], "reboot") ||
		!strcmp(argv[0], "reboot-bootloader") ||
		!strcmp(argv[0], "reboot-fastboot") ||
		!strcmp(argv[0], "usb") ||
		!strcmp(argv[0], "disable-verity") ||
		!strcmp(argv[0], "enable-verity")) {
		// clang-format on
		std::string command;
		if (!strcmp(argv[0], "reboot-bootloader")) {
			command = "reboot:bootloader";
		}
		else if (!strcmp(argv[0], "reboot-fastboot")) {
			command = "reboot:fastboot";
		}
		else if (argc > 1) {
			command = android::base::StringPrintf("%s:%s", argv[0], argv[1]);
		}
		else {
			command = android::base::StringPrintf("%s:", argv[0]);
		}
		return adb_connect_command(command);
	}
	else if (!strcmp(argv[0], "root") || !strcmp(argv[0], "unroot")) {
		return adb_root(argv[0]) ? 0 : 1;
	}
	else if (!strcmp(argv[0], "bugreport")) {
		Bugreport bugreport;
		return bugreport.DoIt(argc, argv);
	}
	else if (!strcmp(argv[0], "forward") || !strcmp(argv[0], "reverse")) {
		bool reverse = !strcmp(argv[0], "reverse");
		--argc;
		if (argc < 1) error_exit("%s requires an argument", argv[0]);
		++argv;

		// Determine the <host-prefix> for this command.
		std::string host_prefix;
		if (reverse) {
			host_prefix = "reverse";
		}
		else {
			if (serial) {
				host_prefix = android::base::StringPrintf("host-serial:%s", serial);
			}
			else if (transport_type == kTransportUsb) {
				host_prefix = "host-usb";
			}
			else if (transport_type == kTransportLocal) {
				host_prefix = "host-local";
			}
			else {
				host_prefix = "host";
			}
		}

		std::string cmd, error_message;
		if (strcmp(argv[0], "--list") == 0) {
			if (argc != 1) error_exit("--list doesn't take any arguments");
			return adb_query_command(host_prefix + ":list-forward");
		}
		else if (strcmp(argv[0], "--remove-all") == 0) {
			if (argc != 1) error_exit("--remove-all doesn't take any arguments");
			cmd = host_prefix + ":killforward-all";
		}
		else if (strcmp(argv[0], "--remove") == 0) {
			// forward --remove <local>
			if (argc != 2) error_exit("--remove requires an argument");
			cmd = host_prefix + ":killforward:" + argv[1];
		}
		else if (strcmp(argv[0], "--no-rebind") == 0) {
			// forward --no-rebind <local> <remote>
			if (argc != 3) error_exit("--no-rebind takes two arguments");
			if (forward_targets_are_valid(argv[1], argv[2], &error_message)) {
				cmd = host_prefix + ":forward:norebind:" + argv[1] + ";" + argv[2];
			}
		}
		else {
			// forward <local> <remote>
			if (argc != 2) error_exit("forward takes two arguments");
			if (forward_targets_are_valid(argv[0], argv[1], &error_message)) {
				cmd = host_prefix + ":forward:" + argv[0] + ";" + argv[1];
			}
		}

		if (!error_message.empty()) {
			error_exit("error: %s", error_message.c_str());
		}

		unique_fd fd(adb_connect(cmd, &error_message));
		if (fd < 0 || !adb_status(fd.get(), &error_message)) {
			error_exit("error: %s", error_message.c_str());
		}

		// Server or device may optionally return a resolved TCP port number.
		std::string resolved_port;
		if (ReadProtocolString(fd, &resolved_port, &error_message) && !resolved_port.empty()) {
			printf("%s\n", resolved_port.c_str());
		}

		ReadOrderlyShutdown(fd);
		return 0;
	}
	/* do_sync_*() commands */
	else if (!strcmp(argv[0], "ls")) {
		if (argc != 2) error_exit("ls requires an argument");
		return do_sync_ls(argv[1]) ? 0 : 1;
	}
	else if (!strcmp(argv[0], "push")) {
		bool copy_attrs = false;
		bool sync = false;
		std::vector<const char*> srcs;
		const char* dst = nullptr;

		parse_push_pull_args(&argv[1], argc - 1, &srcs, &dst, &copy_attrs, &sync);
		if (srcs.empty() || !dst) error_exit("push requires an argument");
		return do_sync_push(srcs, dst, sync) ? 0 : 1;
	}
	else if (!strcmp(argv[0], "pull")) {
		bool copy_attrs = false;
		std::vector<const char*> srcs;
		const char* dst = ".";

		parse_push_pull_args(&argv[1], argc - 1, &srcs, &dst, &copy_attrs, nullptr);
		if (srcs.empty()) error_exit("pull requires an argument");
		return do_sync_pull(srcs, dst, copy_attrs) ? 0 : 1;
	}
	else if (!strcmp(argv[0], "install")) {
		if (argc < 2) error_exit("install requires an argument");
		return install_app(argc, argv);
	}
	else if (!strcmp(argv[0], "install-multiple")) {
		if (argc < 2) error_exit("install-multiple requires an argument");
		return install_multiple_app(argc, argv);
	}
	else if (!strcmp(argv[0], "install-multi-package")) {
		if (argc < 3) error_exit("install-multi-package requires an argument");
		return install_multi_package(argc, argv);
	}
	else if (!strcmp(argv[0], "uninstall")) {
		if (argc < 2) error_exit("uninstall requires an argument");
		return uninstall_app(argc, argv);
	}
	else if (!strcmp(argv[0], "sync")) {
		std::string src;
		bool list_only = false;
		if (argc < 2) {
			// No partition specified: sync all of them.
		}
		else if (argc >= 2 && strcmp(argv[1], "-l") == 0) {
			list_only = true;
			if (argc == 3) src = argv[2];
		}
		else if (argc == 2) {
			src = argv[1];
		}
		else {
			error_exit("usage: adb sync [-l] [PARTITION]");
		}

		if (src.empty()) src = "all";
		std::vector<std::string> partitions{ "data",   "odm",   "oem", "product", "product_services",
											"system", "vendor" };
		bool found = false;
		for (const auto& partition : partitions) {
			if (src == "all" || src == partition) {
				std::string src_dir{ product_file(partition) };
				if (!directory_exists(src_dir)) continue;
				found = true;
				if (!do_sync_sync(src_dir, "/" + partition, list_only)) return 1;
			}
		}
		if (!found) error_exit("don't know how to sync %s partition", src.c_str());
		return 0;
	}
	/* passthrough commands */
	else if (!strcmp(argv[0], "get-state") || !strcmp(argv[0], "get-serialno") ||
		!strcmp(argv[0], "get-devpath")) {
		return adb_query_command(format_host_command(argv[0]));
	}
	/* other commands */
	else if (!strcmp(argv[0], "ppp")) {
		return ppp(argc, argv);
	}
	else if (!strcmp(argv[0], "backup")) {
		return backup(argc, argv);
	}
	else if (!strcmp(argv[0], "restore")) {
		return restore(argc, argv);
	}
	else if (!strcmp(argv[0], "keygen")) {
		if (argc != 2) error_exit("keygen requires an argument");
		// Always print key generation information for keygen command.
		adb_trace_enable(AUTH);
		return adb_auth_keygen(argv[1]);
	}
	else if (!strcmp(argv[0], "pubkey")) {
		if (argc != 2) error_exit("pubkey requires an argument");
		return adb_auth_pubkey(argv[1]);
	}
	else if (!strcmp(argv[0], "jdwp")) {
		return adb_connect_command("jdwp");
	}
	else if (!strcmp(argv[0], "track-jdwp")) {
		return adb_connect_command("track-jdwp");
	}
	else if (!strcmp(argv[0], "track-devices")) {
		return adb_connect_command("host:track-devices");
	}
	else if (!strcmp(argv[0], "raw")) {
		if (argc != 2) {
			error_exit("usage: adb raw SERVICE");
		}
		return adb_connect_command_bidirectional(argv[1]);
	}

	/* "adb /?" is a common idiom under Windows */
	else if (!strcmp(argv[0], "--help") || !strcmp(argv[0], "help") || !strcmp(argv[0], "/?")) {
		help();
		return 0;
	}
	else if (!strcmp(argv[0], "--version") || !strcmp(argv[0], "version")) {
		fprintf(stdout, "%s", adb_version().c_str());
		return 0;
	}
	else if (!strcmp(argv[0], "features")) {
		// Only list the features common to both the adb client and the device.
		FeatureSet features;
		std::string error;
		if (!adb_get_feature_set(&features, &error)) {
			fprintf(stderr, "error: %s\n", error.c_str());
			return 1;
		}

		for (const std::string& name : features) {
			if (CanUseFeature(features, name)) {
				printf("%s\n", name.c_str());
			}
		}
		return 0;
	}
	else if (!strcmp(argv[0], "host-features")) {
		return adb_query_command("host:host-features");
	}
	else if (!strcmp(argv[0], "reconnect")) {
		if (argc == 1) {
			return adb_query_command(format_host_command(argv[0]));
		}
		else if (argc == 2) {
			if (!strcmp(argv[1], "device")) {
				std::string err;
				adb_connect("reconnect", &err);
				return 0;
			}
			else if (!strcmp(argv[1], "offline")) {
				std::string err;
				return adb_query_command("host:reconnect-offline");
			}
			else {
				error_exit("usage: adb reconnect [device|offline]");
			}
		}
	}

	error_exit("unknown command %s", argv[0]);
	return 1;
#endif //_BUILD_ALL
}
