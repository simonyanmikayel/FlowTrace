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

#ifndef ADD_COMMANDLINE_H
#define ADD_COMMANDLINE_H

// Callback used to handle the standard streams (stdout and stderr) sent by the
// device's upon receiving a command.
//
extern int adb_shutdown(int fd, int direction);

class StandardStreamsCallbackInterface {
  public:
    StandardStreamsCallbackInterface() {
		serverFd = -1;
	}
    // Handles the stdout output from devices supporting the Shell protocol.
    virtual bool OnStdout(char* buffer, int length) = 0;

    // Handles the stderr output from devices supporting the Shell protocol.
    virtual bool OnStderr(char* buffer, int length) = 0;

    // Indicates the communication is finished and returns the appropriate error
    // code.
    //
    // |status| has the status code returning by the underlying communication
    // channels
	virtual int Done(int status)
	{
		if (serverFd >= 0)
			adb_shutdown(serverFd, 2);
		serverFd = -1;
		return status;
	}

    virtual void GetStreamBuf(char** p, size_t* c) { *p = nullptr; *c = 0; };

	int serverFd;

  protected:
    static bool OnStream(std::string* string, FILE* stream, const char* buffer, int length) {
        if (string != nullptr) {
            string->append(buffer, length);
        } else {
            fwrite(buffer, 1, length, stream);
            fflush(stream);
        }
		return true;
    }

  private:
	  //DISALLOW_COPY_AND_ASSIGN(StandardStreamsCallbackInterface);
	  StandardStreamsCallbackInterface(const StandardStreamsCallbackInterface&) = delete;
	  void operator=(const StandardStreamsCallbackInterface&) = delete;
};

// Default implementation that redirects the streams to the equilavent host
// stream or to a string
// passed to the constructor.
class DefaultStandardStreamsCallback : public StandardStreamsCallbackInterface {
  public:
    // If |stdout_str| is non-null, OnStdout will append to it.
    // If |stderr_str| is non-null, OnStderr will append to it.
    DefaultStandardStreamsCallback(std::string* stdout_str, std::string* stderr_str)
        : stdout_str_(stdout_str), stderr_str_(stderr_str) {
    }

	bool OnStdout(char* buffer, int length) {
        return OnStream(stdout_str_, stdout, buffer, length);
    }

	bool OnStderr(char* buffer, int length) {
        return OnStream(stderr_str_, stderr, buffer, length);
    }

    int Done(int status) {
        return status;
    }

  private:
    std::string* stdout_str_;
    std::string* stderr_str_;

    //DISALLOW_COPY_AND_ASSIGN(DefaultStandardStreamsCallback);
	DefaultStandardStreamsCallback(const DefaultStandardStreamsCallback&) = delete;
	void operator=(const DefaultStandardStreamsCallback&) = delete;
};

int adb_commandline(int argc, const char** argv, StandardStreamsCallbackInterface* callback = nullptr);


#endif  // ADD_COMMANDLINE_H
