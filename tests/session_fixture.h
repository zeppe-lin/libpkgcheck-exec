// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "check_fixture.h"

#include <libpkgcheck-exec/libpkgcheck-exec.h>

#include <string>
#include <utility>
#include <vector>

namespace execution_fixture {

inline std::string hex(char digit)
{
  return std::string(64, digit);
}

struct admission_fixture final {
  pkgcheck::check_request request;
  pkgcheck_exec::source_tree source;
  pkgcheck_exec::checked_package_tree package;
  std::vector<pkgcheck_exec::package_input_tree> inputs;
  pkgcheck_exec::session_paths paths;
  pkgcheck_exec::execution_identity identity;
  pkgexec::resource_limits limits;
};

inline admission_fixture single_input_fixture()
{
  auto scenario = check_fixture::make_scenario();
  auto build = check_fixture::successful_build(
      scenario.checked, scenario.tester);
  auto request = pkgcheck::check_request::seal(
      scenario.transaction,
      check_fixture::node(
          scenario.transaction,
          pkgtransaction::transaction_action_kind::check).identity(),
      build);

  std::vector<pkgcheck_exec::package_input_tree> inputs;
  for (const auto& input : request.inputs().inputs()) {
    inputs.push_back({
        input.resolved().identity(),
        input.tree(),
        pkgexec::resource_identity::from_sha256(hex('d')),
        "/trees/input/tester",
    });
  }

  return {
      std::move(request),
      {
          scenario.checked.identity(),
          pkgexec::resource_identity::from_sha256(hex('a')),
          "/trees/source",
      },
      {
          build.artifact()->identity(),
          pkgexec::resource_identity::from_sha256(hex('b')),
          "/trees/package",
      },
      std::move(inputs),
      {
          pkgexec::root_view_identity::from_sha256(hex('c')),
          "/",
          "/tmp/check-session",
      },
      {
          pkgexec::interpreter_identity::from_sha256(hex('e')),
          1000,
          1000,
          {},
      },
      pkgexec::resource_limits::make(),
  };
}

inline pkgcheck_exec::admitted_check_session admit(
    admission_fixture fixture)
{
  return pkgcheck_exec::admitted_check_session::admit(
      std::move(fixture.request),
      std::move(fixture.source),
      std::move(fixture.package),
      std::move(fixture.inputs),
      std::move(fixture.paths),
      std::move(fixture.identity),
      std::move(fixture.limits));
}

} // namespace execution_fixture
