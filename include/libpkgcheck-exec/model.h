// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <libpkgcheck/libpkgcheck.h>
#include <libpkgexec/libpkgexec.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace pkgcheck_exec {

namespace detail {
class codec_access;
}

struct source_tree final {
  pkgsource::source_snapshot_identity source;
  pkgexec::resource_identity tree;
  std::filesystem::path path;
};

struct checked_package_tree final {
  pkgbuild::artifact_identity artifact;
  pkgexec::resource_identity tree;
  std::filesystem::path path;
};

struct package_input_resource final {
  pkgbuild::build_input_identity input;
  pkgexec::resource_identity resource;
  std::filesystem::path path;
};

struct session_paths final {
  pkgexec::root_view_identity root_view;
  std::filesystem::path root_view_path;
  std::filesystem::path temporary_root;
};

struct execution_identity final {
  pkgexec::interpreter_identity interpreter;
  std::uint64_t user_id = 0;
  std::uint64_t group_id = 0;
  std::vector<std::uint64_t> supplementary_groups;
};

class admitted_check_session final {
public:
  [[nodiscard]] static admitted_check_session admit(
      pkgcheck::check_request request,
      source_tree source,
      checked_package_tree package,
      std::vector<package_input_resource> inputs,
      session_paths paths,
      execution_identity identity,
      pkgexec::resource_limits limits = pkgexec::resource_limits::make());

  [[nodiscard]] const pkgcheck::check_request& request() const noexcept;
  [[nodiscard]] const source_tree& source() const noexcept;
  [[nodiscard]] const checked_package_tree& package() const noexcept;
  [[nodiscard]] const std::vector<package_input_resource>&
  inputs() const noexcept;
  [[nodiscard]] const session_paths& paths() const noexcept;
  [[nodiscard]] const execution_identity& identity() const noexcept;
  [[nodiscard]] const pkgexec::resource_limits& limits() const noexcept;

private:
  admitted_check_session(
      pkgcheck::check_request request,
      source_tree source,
      checked_package_tree package,
      std::vector<package_input_resource> inputs,
      session_paths paths,
      execution_identity identity,
      pkgexec::resource_limits limits);

  pkgcheck::check_request request_;
  source_tree source_;
  checked_package_tree package_;
  std::vector<package_input_resource> inputs_;
  session_paths paths_;
  execution_identity identity_;
  pkgexec::resource_limits limits_;
};

struct prepared_execution final {
  pkgexec::execution_request request;
  pkgexec::execution_resources resources;
};

class check_execution_result final {
public:
  [[nodiscard]] const pkgexec::execution_result& execution() const noexcept;
  [[nodiscard]] const pkgcheck::check_result& check() const noexcept;

private:
  check_execution_result(
      pkgexec::execution_result execution,
      pkgcheck::check_result check);

  pkgexec::execution_result execution_;
  pkgcheck::check_result check_;

  friend class detail::codec_access;

  friend check_execution_result execute(
      const admitted_check_session&,
      pkgexec::execution_backend&);
};

} // namespace pkgcheck_exec
