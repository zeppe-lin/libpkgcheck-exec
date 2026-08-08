// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../fixtures/session.h"
#include "../support/test.h"

#include <iostream>
#include <utility>

namespace {

using execution_fixture::admit;
using execution_fixture::single_input_fixture;

void prove_exact_materialization()
{
  const auto session = admit(single_input_fixture());
  const auto projected = pkgcheck_exec::seal_execution_request(session);
  const auto prepared = pkgcheck_exec::prepare(session);

  TEST_CHECK(prepared.request == projected);
  TEST_CHECK(prepared.resources.root_view() == session.paths().root_view);
  TEST_CHECK(prepared.resources.root_view_path() ==
             session.paths().root_view_path);
  TEST_CHECK(prepared.resources.materialization(session.source().tree)
                 .host_path() == session.source().path);
  TEST_CHECK(prepared.resources.materialization(session.package().tree)
                 .host_path() == session.package().path);
  for (const auto& input : session.inputs()) {
    TEST_CHECK(prepared.resources.materialization(input.resource).host_path() ==
               input.path);
  }

  const auto temporary_slot = pkgexec::resource_slot::singleton(
      pkgexec::resource_role::private_temporary_root);
  const auto temporary =
      prepared.request.resources().binding(temporary_slot).resource();
  TEST_CHECK(prepared.resources.materialization(temporary).host_path() ==
             session.paths().temporary_root);
}

void prove_relocation_preserves_request()
{
  auto first = single_input_fixture();
  auto second = single_input_fixture();
  second.source.path = "/relocated/source";
  second.package.path = "/relocated/package";
  second.inputs[0].path = "/relocated/input";
  second.paths.root_view_path = "/relocated/root";
  second.paths.temporary_root = "/relocated/tmp";

  const auto lhs = pkgcheck_exec::prepare(admit(std::move(first)));
  const auto rhs = pkgcheck_exec::prepare(admit(std::move(second)));
  TEST_CHECK(lhs.request.identity() == rhs.request.identity());
  TEST_CHECK(lhs.resources.root_view_path() != rhs.resources.root_view_path());
}

} // namespace

int main()
{
  try {
    prove_exact_materialization();
    prove_relocation_preserves_request();
    return 0;
  } catch (const std::exception& value) {
    std::cerr << value.what() << '\n';
    return 1;
  }
}
