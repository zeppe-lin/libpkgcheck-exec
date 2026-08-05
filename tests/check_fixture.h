// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "fixture_transaction.h"

#include <libpkgbuild/libpkgbuild.h>
#include <libpkgcheck/libpkgcheck.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace check_fixture {

struct scenario final {
  pkgsource::source_snapshot checked;
  pkgsource::source_snapshot tester;
  pkgtransaction::transaction_program transaction;
};

struct multi_input_scenario final {
  pkgsource::source_snapshot checked;
  pkgsource::source_snapshot tester_a;
  pkgsource::source_snapshot tester_b;
  pkgtransaction::transaction_program transaction;
};

inline scenario make_scenario(
    std::string check_program = "printf 'checked\\n'\n")
{
  auto profiles = fixture::profiles();
  auto checked = fixture::source(
      profiles, "checked",
      {fixture::requirement(pkgsource::requirement_scope::check(),
                            "tester", "requirements.check[0]")},
      {"x86_64"}, {"x86_64"}, "1.0.0", 1, {},
      std::move(check_program));
  auto tester = fixture::source(profiles, "tester");
  auto catalog = fixture::catalog(profiles, {checked, tester});
  auto resolution = fixture::resolution(
      std::move(catalog), fixture::empty_state(),
      {fixture::package_goal(pkgsource::requirement_scope::check(),
                             "checked")});
  auto transaction = pkgtransaction::compose(
      pkgtransaction::transaction_request::seal(std::move(resolution)));
  return {std::move(checked), std::move(tester), std::move(transaction)};
}

inline multi_input_scenario make_multi_input_scenario()
{
  auto profiles = fixture::profiles();
  auto checked = fixture::source(
      profiles, "checked",
      {
          fixture::requirement(pkgsource::requirement_scope::check(),
                               "tester-a", "requirements.check[0]"),
          fixture::requirement(pkgsource::requirement_scope::check(),
                               "tester-b", "requirements.check[1]"),
      },
      {"x86_64"}, {"x86_64"}, "1.0.0", 1, {}, "true\n");
  auto tester_a = fixture::source(profiles, "tester-a");
  auto tester_b = fixture::source(profiles, "tester-b");
  auto catalog = fixture::catalog(
      profiles, {checked, tester_a, tester_b});
  auto resolution = fixture::resolution(
      std::move(catalog), fixture::empty_state(),
      {fixture::package_goal(pkgsource::requirement_scope::check(),
                             "checked")});
  auto transaction = pkgtransaction::compose(
      pkgtransaction::transaction_request::seal(std::move(resolution)));
  return {
      std::move(checked), std::move(tester_a), std::move(tester_b),
      std::move(transaction),
  };
}

inline const pkgtransaction::transaction_node& node(
    const pkgtransaction::transaction_program& transaction,
    pkgtransaction::transaction_action_kind action,
    const char* package = "checked")
{
  const auto found = std::find_if(
      transaction.nodes().begin(), transaction.nodes().end(),
      [&](const auto& value) {
        return value.action() == action &&
               value.package().name() == package;
      });
  if (found == transaction.nodes().end())
    throw std::runtime_error("transaction fixture lacks requested node");
  return *found;
}

inline pkgbuild::build_request request(
    const pkgtransaction::transaction_program& transaction,
    std::uint32_t parallelism = 2)
{
  const auto& build = node(
      transaction, pkgtransaction::transaction_action_kind::build);
  if (!build.selection())
    throw std::runtime_error("build fixture lacks selected package authority");
  return pkgbuild::build_request::seal(
      transaction.request().resolution(), build.selection()->identity(),
      pkgbuild::build_policy::make(
          pkgbuild::environment_policy::hermetic(
              parallelism, 0022, 1700000000)));
}

inline pkgbuild::build_result successful_build(
    pkgbuild::build_request request,
    char seed = 'b')
{
  return pkgbuild::build_result::succeeded(
      std::move(request), pkgbuild::payload_manifest::seal({}),
      pkgbuild::sealed_artifact::make(
          pkgbuild::artifact_encoding::package_tar,
          pkgbuild::artifact_compression::none, 128,
          pkgbuild::sha256_digest(std::string(64, seed))),
      pkgbuild::execution_evidence_identity::from_sha256(
          std::string(64, static_cast<char>(seed + 1))));
}

inline pkgbuild::build_result successful_build(
    const pkgtransaction::transaction_program& transaction,
    char seed = 'b')
{
  return successful_build(request(transaction), seed);
}

inline pkgbuild::build_result successful_multi_input_build(
    const multi_input_scenario& scenario)
{
  return successful_build(scenario.transaction, 'b');
}

inline pkgbuild::build_result failed_build(
    const pkgtransaction::transaction_program& transaction,
    char seed = 'd')
{
  return pkgbuild::build_result::failed(
      request(transaction),
      pkgbuild::execution_evidence_identity::from_sha256(
          std::string(64, seed)),
      pkgbuild::failure_evidence_identity::from_sha256(
          std::string(64, static_cast<char>(seed + 1))));
}

inline pkgcheck::check_request admitted_request()
{
  auto value = make_scenario();
  return pkgcheck::check_request::seal(
      value.transaction,
      node(value.transaction,
           pkgtransaction::transaction_action_kind::check).identity(),
      successful_build(value.transaction));
}

} // namespace check_fixture
