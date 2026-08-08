// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <libpkgcheck-exec/error.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

namespace pkgcheck_exec_test {

inline void require(bool condition, const char* expression,
                    const char* file, int line)
{
  if (condition)
    return;
  std::cerr << file << ':' << line << ": check failed: " << expression << '\n';
  std::exit(1);
}

template<typename Function>
void expect_error(pkgcheck_exec::error_code expected, Function&& function)
{
  bool caught = false;
  try {
    std::forward<Function>(function)();
  } catch (const pkgcheck_exec::error& value) {
    caught = true;
    require(value.code() == expected, "error code matches", __FILE__, __LINE__);
  }
  require(caught, "expected pkgcheck_exec::error", __FILE__, __LINE__);
}

} // namespace pkgcheck_exec_test

#define TEST_CHECK(expr) \
  ::pkgcheck_exec_test::require((expr), #expr, __FILE__, __LINE__)
