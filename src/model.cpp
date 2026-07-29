// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/model.h>

#include <libpkgcheck-exec/error.h>

#include <algorithm>
#include <string>
#include <utility>

namespace pkgcheck_exec {
namespace fs = std::filesystem;
namespace {

fs::path require_absolute_path(fs::path path, const char* description)
{
  if (path.empty() || !path.is_absolute())
    throw error(error_code::invalid_path,
                std::string(description) + " must be absolute");
  return path.lexically_normal();
}

void normalize_paths(source_tree& source,
                     checked_package_tree& package,
                     session_paths& paths)
{
  source.path = require_absolute_path(std::move(source.path), "source tree");
  package.path = require_absolute_path(std::move(package.path),
                                       "checked package tree");
  paths.root_view_path = require_absolute_path(
      std::move(paths.root_view_path), "root view");
  paths.temporary_root = require_absolute_path(
      std::move(paths.temporary_root), "temporary root");
}

void require_source_authority(const pkgcheck::check_request& request,
                              const source_tree& source)
{
  if (source.source != request.build().request().source().identity())
    throw error(error_code::inconsistent_authority,
                "source tree does not name the checked source snapshot");
}

void require_package_authority(const pkgcheck::check_request& request,
                               const checked_package_tree& package)
{
  const auto& artifact = request.build().artifact();
  if (!artifact || package.artifact != artifact->identity())
    throw error(error_code::inconsistent_authority,
                "checked package tree does not name the successful "
                "build artifact");
}

void normalize_and_validate_inputs(
    const pkgcheck::check_request& request,
    std::vector<package_input_tree>& inputs)
{
  std::sort(inputs.begin(), inputs.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.input < rhs.input;
            });

  for (std::size_t index = 1; index < inputs.size(); ++index) {
    if (inputs[index - 1].input == inputs[index].input)
      throw error(error_code::duplicate_input,
                  "duplicate check input tree");
  }

  const auto& expected = request.inputs().inputs();
  if (inputs.size() != expected.size())
    throw error(error_code::missing_input,
                "check input tree set is incomplete");

  for (std::size_t index = 0; index < inputs.size(); ++index) {
    auto& input = inputs[index];
    input.path = require_absolute_path(std::move(input.path),
                                       "check input tree");

    if (input.input != expected[index].resolved().identity() ||
        input.tree != expected[index].tree())
      throw error(error_code::inconsistent_authority,
                  "check input tree does not match the sealed request");
  }
}

} // namespace

admitted_check_session::admitted_check_session(
    pkgcheck::check_request request,
    source_tree source,
    checked_package_tree package,
    std::vector<package_input_tree> inputs,
    session_paths paths,
    execution_identity identity,
    pkgexec::resource_limits limits)
    : request_(std::move(request)), source_(std::move(source)),
      package_(std::move(package)), inputs_(std::move(inputs)),
      paths_(std::move(paths)), identity_(std::move(identity)),
      limits_(std::move(limits))
{
}

admitted_check_session admitted_check_session::admit(
    pkgcheck::check_request request,
    source_tree source,
    checked_package_tree package,
    std::vector<package_input_tree> inputs,
    session_paths paths,
    execution_identity identity,
    pkgexec::resource_limits limits)
{
  normalize_paths(source, package, paths);
  require_source_authority(request, source);
  require_package_authority(request, package);
  normalize_and_validate_inputs(request, inputs);

  return admitted_check_session(
      std::move(request), std::move(source), std::move(package),
      std::move(inputs), std::move(paths), std::move(identity),
      std::move(limits));
}

const pkgcheck::check_request&
admitted_check_session::request() const noexcept
{
  return request_;
}

const source_tree& admitted_check_session::source() const noexcept
{
  return source_;
}

const checked_package_tree& admitted_check_session::package() const noexcept
{
  return package_;
}

const std::vector<package_input_tree>&
admitted_check_session::inputs() const noexcept
{
  return inputs_;
}

const session_paths& admitted_check_session::paths() const noexcept
{
  return paths_;
}

const execution_identity&
admitted_check_session::identity() const noexcept
{
  return identity_;
}

const pkgexec::resource_limits&
admitted_check_session::limits() const noexcept
{
  return limits_;
}

check_execution_result::check_execution_result(
    pkgexec::execution_result execution,
    pkgcheck::check_result check)
    : execution_(std::move(execution)), check_(std::move(check))
{
}

const pkgexec::execution_result&
check_execution_result::execution() const noexcept
{
  return execution_;
}

const pkgcheck::check_result&
check_execution_result::check() const noexcept
{
  return check_;
}

} // namespace pkgcheck_exec
