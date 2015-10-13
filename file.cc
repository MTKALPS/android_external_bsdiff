// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "file.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// TEMP_FAILURE_RETRY is defined by some versions of <unistd.h>.
#ifndef TEMP_FAILURE_RETRY
#include <utils/Compat.h>
#endif

#include <algorithm>

namespace bsdiff {

std::unique_ptr<File> File::FOpen(const char* pathname, int flags) {
  int fd = TEMP_FAILURE_RETRY(open(pathname, flags));
  if (fd < 0)
    return std::unique_ptr<File>();
  return std::unique_ptr<File>(new File(fd));
}

File::~File() {
  Close();
}

bool File::Read(void* buf, size_t count, size_t* bytes_read) {
  if (fd_ < 0) {
    errno = EBADF;
    return false;
  }
  ssize_t rc = TEMP_FAILURE_RETRY(read(fd_, buf, count));
  if (rc == -1)
    return false;
  *bytes_read = static_cast<size_t>(rc);
  return true;
}

bool File::Write(const void* buf, size_t count, size_t* bytes_written) {
  if (fd_ < 0) {
    errno = EBADF;
    return false;
  }
  ssize_t rc = TEMP_FAILURE_RETRY(write(fd_, buf, count));
  if (rc == -1)
    return false;
  *bytes_written = static_cast<size_t>(rc);
  return true;
}

bool File::Seek(off_t pos) {
  if (fd_ < 0) {
    errno = EBADF;
    return false;
  }
  // fseek() uses a long value for the offset which could be smaller than off_t.
  if (pos > std::numeric_limits<long>::max()) {
    errno = EOVERFLOW;
    return false;
  }
  off_t newpos = lseek(fd_, pos, SEEK_SET) == pos;
  if (newpos < 0)
    return false;
  if (newpos != pos) {
    errno = EINVAL;
    return false;
  }
  return true;
}

bool File::Close() {
  if (fd_ < 0) {
    errno = EBADF;
    return false;
  }
  bool success = close(fd_) == 0;
  if (!success && errno == EINTR)
    success = true;
  fd_ = -1;
  return success;
}

File::File(int fd)
    : fd_(fd) {}

}  // namespace bsdiff
