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

inline pkgbuild::materialized_package_input check_input(
    const pkgsource::source_snapshot& source, char seed = 'a')
{
  const std::string hex(64, seed);
  return pkgbuild::materialized_package_input(
      pkgbuild::resolved_package_input::make(
          pkgbuild::input_scope::check,
          source.recipe().release().package(),
          source.recipe().release(), source.identity(),
          pkgbuild::build_result_identity::from_sha256(hex),
          pkgbuild::artifact_identity::from_sha256(hex)),
      pkgbuild::input_tree_identity::from_sha256(hex));
}

inline pkgbuild::build_request request(
    const pkgsource::source_snapshot& checked,
    const pkgsource::source_snapshot& tester)
{
  return pkgbuild::build_request::seal(
      checked, {}, {check_input(tester)},
      pkgsource::architecture_reference("x86_64"),
      pkgsource::architecture_reference("x86_64"),
      pkgbuild::build_policy::make(
          pkgbuild::environment_policy::hermetic(2, 0022, 1700000000)));
}

inline pkgbuild::build_result successful_build(
    const pkgsource::source_snapshot& checked,
    const pkgsource::source_snapshot& tester,
    char seed = 'b')
{
  return pkgbuild::build_result::succeeded(
      request(checked, tester), pkgbuild::payload_manifest::seal({}),
      pkgbuild::sealed_artifact::make(
          pkgbuild::artifact_encoding::package_tar_v1,
          pkgbuild::artifact_compression::none, 128,
          pkgbuild::sha256_digest(std::string(64, seed))),
      pkgbuild::execution_evidence_identity::from_sha256(
          std::string(64, static_cast<char>(seed + 1))));
}

inline pkgbuild::build_result failed_build(
    const pkgsource::source_snapshot& checked,
    const pkgsource::source_snapshot& tester,
    char seed = 'd')
{
  return pkgbuild::build_result::failed(
      request(checked, tester),
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
      successful_build(value.checked, value.tester));
}

} // namespace check_fixture
