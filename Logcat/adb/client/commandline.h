/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "adb.h"
#include "adb_commandline.h"

class SilentStandardStreamsCallbackInterface : public StandardStreamsCallbackInterface {
  public:
    SilentStandardStreamsCallbackInterface() = default;
    bool OnStdout(char*, int) override final {}
    bool OnStderr(char*, int) override final {}
    int Done(int status) override final { return status; }
};

// Singleton.
extern DefaultStandardStreamsCallback DEFAULT_STANDARD_STREAMS_CALLBACK;

void copy_to_file(int inFd, int outFd);

// Connects to the device "shell" service with |command| and prints the
// resulting output.
// if |callback| is non-null, stdout/stderr output will be handled by it.
int send_shell_command(
		const std::string& command, bool disable_shell_protocol,
        StandardStreamsCallbackInterface* callback = &DEFAULT_STANDARD_STREAMS_CALLBACK);

#endif  // COMMANDLINE_H
