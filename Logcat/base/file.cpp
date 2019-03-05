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

#include "android-base/file.h"

#include <errno.h>
#include <fcntl.h>
//!!#include <ftw.h>
//!!#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#define O_NOFOLLOW 0
#define OS_PATH_SEPARATOR '\\'
#else
#define OS_PATH_SEPARATOR '/'
#endif

#include "android-base/logging.h"  // and must be after windows.h for ERROR
#include "android-base/macros.h"   // For TEMP_FAILURE_RETRY on Darwin.
#include "android-base/unique_fd.h"
#include "android-base/utf8.h"

#define TRACE_TAG ADB
#include "adb_trace.h"

#ifdef _WIN32
int mkstemp(char* template_name) {
  if (_mktemp(template_name) == nullptr) {
    return -1;
  }
  // Use open() to match the close() that TemporaryFile's destructor does.
  // Use O_BINARY to match base file APIs.
  return _open(template_name, O_CREAT | O_EXCL | O_RDWR | O_BINARY, S_IRUSR | S_IWUSR);
}

char* mkdtemp(char* template_name) {
  if (_mktemp(template_name) == nullptr) {
    return nullptr;
  }
  if (_mkdir(template_name) == -1) {
    return nullptr;
  }
  return template_name;
}
#endif

namespace {

#if _BUILD_ALL
std::string GetSystemTempDir() {
#if defined(__ANDROID__)
  const auto* tmpdir = getenv("TMPDIR");
  if (tmpdir == nullptr) tmpdir = "/data/local/tmp";
  if (access(tmpdir, R_OK | W_OK | X_OK) == 0) {
    return tmpdir;
  }
  // Tests running in app context can't access /data/local/tmp,
  // so try current directory if /data/local/tmp is not accessible.
  return ".";
#elif defined(_WIN32)
  char tmp_dir[MAX_PATH];
  DWORD result = GetTempPathA(sizeof(tmp_dir), tmp_dir);  // checks TMP env
  D("GetTempPathA failed, error: %d", GetLastError());
  D("path truncated to: %d", result);

  // GetTempPath() returns a path with a trailing slash, but init()
  // does not expect that, so remove it.
  CHECK_EQ(tmp_dir[result - 1], '\\');
  tmp_dir[result - 1] = '\0';
  return tmp_dir;
#else
  const auto* tmpdir = getenv("TMPDIR");
  if (tmpdir == nullptr) tmpdir = "/tmp";
  return tmpdir;
#endif
}
#endif //_BUILD_ALL

}  // namespace

#if _BUILD_ALL
TemporaryFile::TemporaryFile() {
  init(GetSystemTempDir());
}
#endif //_BUILD_ALL

#if _BUILD_ALL
TemporaryFile::TemporaryFile(const std::string& tmp_dir) {
  init(tmp_dir);
}
#endif //_BUILD_ALL

#if _BUILD_ALL
TemporaryFile::~TemporaryFile() {
  if (fd != -1) {
    _close(fd);
  }
  if (remove_file_) {
    unlink(path);
  }
}
#endif //_BUILD_ALL

#if _BUILD_ALL
int TemporaryFile::release() {
  int result = fd;
  fd = -1;
  return result;
}
#endif //_BUILD_ALL

#if _BUILD_ALL
void TemporaryFile::init(const std::string& tmp_dir) {
  snprintf(path, sizeof(path), "%s%cTemporaryFile-XXXXXX", tmp_dir.c_str(), OS_PATH_SEPARATOR);
  fd = mkstemp(path);
}
#endif //_BUILD_ALL

#if _BUILD_ALL
TemporaryDir::TemporaryDir() {
  init(GetSystemTempDir());
}
#endif //_BUILD_ALL

//ftw.h

#if _BUILD_ALL
enum
{
	FTW_F,                /* Regular file.  */
	FTW_D,                /* Directory.  */
	FTW_DNR,                /* Unreadable directory.  */
	FTW_NS,                /* Unstatable file.  */
	FTW_SL,                /* Symbolic link.  */
	FTW_DP,                /* Directory, all subdirs have been visited. */
	FTW_SLN                /* Symbolic link naming non-existing file.  */
};
#endif //_BUILD_ALL

#if _BUILD_ALL
TemporaryDir::~TemporaryDir() {
  /*!!TODO!!
  if (!remove_dir_and_contents_) return;

  auto callback = [](const char* child, const struct stat*, int file_type, struct FTW*) -> int {
    switch (file_type) {
      case FTW_D:
      case FTW_DP:
      case FTW_DNR:
        if (rmdir(child) == -1) {
          PLOG(ERROR) << "rmdir " << child;
        }
        break;
      case FTW_NS:
      default:
        if (rmdir(child) != -1) break;
        // FALLTHRU (for gcc, lint, pcc, etc; and following for clang)
        FALLTHROUGH_INTENDED;
      case FTW_F:
      case FTW_SL:
      case FTW_SLN:
        if (unlink(child) == -1) {
          PLOG(ERROR) << "unlink " << child;
        }
        break;
    }
    return 0;
  };

   nftw(path, callback, 128, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
   */
}
#endif //_BUILD_ALL

#if _BUILD_ALL
bool TemporaryDir::init(const std::string& tmp_dir) {
  snprintf(path, sizeof(path), "%s%cTemporaryDir-XXXXXX", tmp_dir.c_str(), OS_PATH_SEPARATOR);
  return (mkdtemp(path) != nullptr);
}
#endif //_BUILD_ALL

namespace android {
namespace base {

// Versions of standard library APIs that support UTF-8 strings.
using namespace android::base::utf8;

#if _BUILD_ALL1
bool ReadFdToString(int fd, std::string* content) {
  content->clear();

  // Although original we had small files in mind, this code gets used for
  // very large files too, where the std::string growth heuristics might not
  // be suitable. https://code.google.com/p/android/issues/detail?id=258500.
  struct stat sb;
  if (fstat(fd, &sb) != -1 && sb.st_size > 0) {
    content->reserve(sb.st_size);
  }

  char buf[BUFSIZ];
  ssize_t n;
  while ((n = _read(fd, &buf[0], sizeof(buf))) > 0) {
    content->append(buf, n);
  }
  return (n == 0) ? true : false;
}
#endif //_BUILD_ALL

#if _BUILD_ALL
bool ReadFileToString(const std::string& path, std::string* content, bool follow_symlinks) {
  content->clear();

  int flags = O_RDONLY | O_CLOEXEC | O_BINARY | (follow_symlinks ? 0 : O_NOFOLLOW);
  android::base::unique_fd fd(_open(path.c_str(), flags));
  if (fd == -1) {
    return false;
  }
  return ReadFdToString(fd, content);
}
#endif //_BUILD_ALL

#if _BUILD_ALL
bool WriteStringToFd(const std::string& content, int fd) {
  const char* p = content.data();
  size_t left = content.size();
  while (left > 0) {
    ssize_t n = _write(fd, p, (int)left);
    if (n == -1) {
      return false;
    }
    p += n;
    left -= n;
  }
  return true;
}
#endif //_BUILD_ALL

#if _BUILD_ALL
static bool CleanUpAfterFailedWrite(const std::string& path) {
  // Something went wrong. Let's not leave a corrupt file lying around.
  int saved_errno = errno;
  unlink(path.c_str());
  errno = saved_errno;
  return false;
}
#endif //_BUILD_ALL

#if !defined(_WIN32)
bool WriteStringToFile(const std::string& content, const std::string& path,
                       mode_t mode, uid_t owner, gid_t group,
                       bool follow_symlinks) {
  int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_BINARY |
              (follow_symlinks ? 0 : O_NOFOLLOW);
  android::base::unique_fd fd(TEMP_FAILURE_RETRY(open(path.c_str(), flags, mode)));
  if (fd == -1) {
    PLOG(ERROR) << "android::WriteStringToFile open failed";
    return false;
  }

  // We do an explicit fchmod here because we assume that the caller really
  // meant what they said and doesn't want the umask-influenced mode.
  if (fchmod(fd, mode) == -1) {
    PLOG(ERROR) << "android::WriteStringToFile fchmod failed";
    return CleanUpAfterFailedWrite(path);
  }
  if (fchown(fd, owner, group) == -1) {
    PLOG(ERROR) << "android::WriteStringToFile fchown failed";
    return CleanUpAfterFailedWrite(path);
  }
  if (!WriteStringToFd(content, fd)) {
    PLOG(ERROR) << "android::WriteStringToFile write failed";
    return CleanUpAfterFailedWrite(path);
  }
  return true;
}
#endif

#if _BUILD_ALL
bool WriteStringToFile(const std::string& content, const std::string& path, bool follow_symlinks) {
  int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_BINARY |
              (follow_symlinks ? 0 : O_NOFOLLOW);
  android::base::unique_fd fd(_open(path.c_str(), flags, 0666));
  if (fd == -1) {
    return false;
  }
  return WriteStringToFd(content, fd) || CleanUpAfterFailedWrite(path);
}
#endif //_BUILD_ALL

#if _BUILD_ALL
bool ReadFully(int fd, void* data, size_t byte_count) {
  uint8_t* p = reinterpret_cast<uint8_t*>(data);
  size_t remaining = byte_count;
  while (remaining > 0) {
    ssize_t n = _read(fd, p, (int)remaining);
    if (n <= 0) return false;
    p += n;
    remaining -= n;
  }
  return true;
}
#endif //_BUILD_ALL

#if defined(_WIN32)
// Windows implementation of pread. Note that this DOES move the file descriptors read position,
// but it does so atomically.

#if _BUILD_ALL
static ssize_t pread(int fd, void* data, size_t byte_count, INT64 offset) {
  DWORD bytes_read;
  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(OVERLAPPED));
  overlapped.Offset = static_cast<DWORD>(offset);
  overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);
  if (!ReadFile(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), data, static_cast<DWORD>(byte_count),
                &bytes_read, &overlapped)) {
    // In case someone tries to read errno (since this is masquerading as a POSIX call)
    errno = EIO;
    return -1;
  }
  return static_cast<ssize_t>(bytes_read);
}
#endif //_BUILD_ALL
#endif

#if _BUILD_ALL
bool ReadFullyAtOffset(int fd, void* data, size_t byte_count, INT64 offset) {
  uint8_t* p = reinterpret_cast<uint8_t*>(data);
  while (byte_count > 0) {
    ssize_t n = pread(fd, p, byte_count, offset);
    if (n <= 0) return false;
    p += n;
    byte_count -= n;
    offset += n;
  }
  return true;
}
#endif //_BUILD_ALL

#if _BUILD_ALL
bool WriteFully(int fd, const void* data, size_t byte_count) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
  size_t remaining = byte_count;
  while (remaining > 0) {
    ssize_t n = _write(fd, p, (int)remaining);
    if (n == -1) return false;
    p += n;
    remaining -= n;
  }
  return true;
}
#endif //_BUILD_ALL

#if _BUILD_ALL
bool RemoveFileIfExists(const std::string& path, std::string* err) {
  struct stat st;
#if defined(_WIN32)
  // TODO: Windows version can't handle symbolic links correctly.
  int result = stat(path.c_str(), &st);
  bool file_type_removable = (result == 0 && S_ISREG(st.st_mode));
#else
  int result = lstat(path.c_str(), &st);
  bool file_type_removable = (result == 0 && (S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)));
#endif
  if (result == -1) {
    if (errno == ENOENT || errno == ENOTDIR) return true;
    if (err != nullptr) *err = strerror(errno);
    return false;
  }

  if (result == 0) {
    if (!file_type_removable) {
      if (err != nullptr) {
        *err = "is not a regular file or symbolic link";
      }
      return false;
    }
    if (unlink(path.c_str()) == -1) {
      if (err != nullptr) {
        *err = strerror(errno);
      }
      return false;
    }
  }
  return true;
}
#endif //_BUILD_ALL

#if !defined(_WIN32)
bool Readlink(const std::string& path, std::string* result) {
  result->clear();

  // Most Linux file systems (ext2 and ext4, say) limit symbolic links to
  // 4095 bytes. Since we'll copy out into the string anyway, it doesn't
  // waste memory to just start there. We add 1 so that we can recognize
  // whether it actually fit (rather than being truncated to 4095).
  std::vector<char> buf(4095 + 1);
  while (true) {
    ssize_t size = readlink(path.c_str(), &buf[0], buf.size());
    // Unrecoverable error?
    if (size == -1) return false;
    // It fit! (If size == buf.size(), it may have been truncated.)
    if (static_cast<size_t>(size) < buf.size()) {
      result->assign(&buf[0], size);
      return true;
    }
    // Double our buffer and try again.
    buf.resize(buf.size() * 2);
  }
}
#endif

#if !defined(_WIN32)
bool Realpath(const std::string& path, std::string* result) {
  result->clear();

  char* realpath_buf = realpath(path.c_str(), nullptr);
  if (realpath_buf == nullptr) {
    return false;
  }
  result->assign(realpath_buf);
  free(realpath_buf);
  return true;
}
#endif

#if _BUILD_ALL1
std::string GetExecutablePath() {
#if defined(__linux__)
  std::string path;
  android::base::Readlink("/proc/self/exe", &path);
  return path;
#elif defined(__APPLE__)
  char path[PATH_MAX + 1];
  uint32_t path_len = sizeof(path);
  int rc = _NSGetExecutablePath(path, &path_len);
  if (rc < 0) {
    std::unique_ptr<char> path_buf(new char[path_len]);
    _NSGetExecutablePath(path_buf.get(), &path_len);
    return path_buf.get();
  }
  return path;
#elif defined(_WIN32)
  char path[PATH_MAX + 1];
  DWORD result = GetModuleFileNameA(NULL, path, sizeof(path) - 1);
  if (result == 0 || result == sizeof(path) - 1) return "";
  path[PATH_MAX - 1] = 0;
  return path;
#else
#error unknown OS
#endif
}
#endif //_BUILD_ALL

#if _BUILD_ALL
std::string GetExecutableDirectory() {
  return Dirname(GetExecutablePath());
}
#endif //_BUILD_ALL

#if _BUILD_ALL1
std::string Basename(const std::string& path) {
  // Copy path because basename may modify the string passed in.
	char szPath[300];
	strncpy_s(szPath, path.c_str(), sizeof(szPath)-1);
	szPath[sizeof(szPath) - 1] = 0;

  char* baseName = strrchr(szPath, '\\');
  if (!baseName)
	  baseName = strrchr(szPath, '/');
  if (baseName)
	  baseName++;
  else
	  baseName = szPath;
  char* dot = strrchr(baseName, '.');
  if (dot)
	  *dot = 0;

  std::string result(baseName);

  return result;
}
#endif //_BUILD_ALL

#if _BUILD_ALL
std::string Dirname(const std::string& path) {

	char szPath[300];
	strncpy_s(szPath, path.c_str(), sizeof(szPath) - 1);
	szPath[sizeof(szPath) - 1] = 0;

	char* slash = strrchr(szPath, '\\');
	if (!slash)
		slash = strrchr(szPath, '/');
	if (slash)
		*slash = 0;
	else {
		if (0 != strcmp(szPath, ".") && 0 != strcmp(szPath, ".."))
		{
			szPath[0] = '.';
			szPath[1] = 0;
		}

		/*
path         dirname    basename
"/usr/lib"    "/usr"    "lib"
"/usr/"       "/"       "usr"
"usr"         "."       "usr"
"/"           "/"       "/"
"."           "."       "."
".."          "."       ".."
		*/
	}

	std::string result(szPath);

  return result;
}
#endif //_BUILD_ALL

}  // namespace base
}  // namespace android
