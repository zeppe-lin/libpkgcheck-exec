// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/model.h>

#include <libpkgcheck-exec/error.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace pkgcheck_exec {
namespace fs = std::filesystem;
namespace {

fs::path require_absolute_path(fs::path path,
                               const char* description,
                               bool root_allowed = false)
{
  if (path.empty() || !path.is_absolute())
    throw error(error_code::invalid_path,
                std::string(description) + " must be absolute");

  path = path.lexically_normal();
  if (!root_allowed && path == path.root_path())
    throw error(error_code::invalid_path,
                std::string(description) + " must not be the filesystem root");
  return path;
}

bool path_prefix(const fs::path& prefix, const fs::path& value)
{
  auto prefix_component = prefix.begin();
  auto value_component = value.begin();
  while (prefix_component != prefix.end() && value_component != value.end()) {
    if (*prefix_component != *value_component)
      return false;
    ++prefix_component;
    ++value_component;
  }
  return prefix_component == prefix.end();
}

bool paths_overlap(const fs::path& lhs, const fs::path& rhs)
{
  return path_prefix(lhs, rhs) || path_prefix(rhs, lhs);
}

void normalize_primary_paths(source_tree& source,
                             checked_package_tree& package,
                             session_paths& paths)
{
  source.path = require_absolute_path(std::move(source.path), "source tree");
  package.path = require_absolute_path(std::move(package.path),
                                       "checked package tree");
  paths.root_view_path = require_absolute_path(
      std::move(paths.root_view_path), "root view", true);
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

std::vector<package_input_resource> normalize_and_validate_inputs(
    const pkgcheck::check_request& request,
    std::vector<package_input_resource> supplied)
{
  for (auto& input : supplied)
    input.path = require_absolute_path(std::move(input.path),
                                       "check input tree");

  std::sort(supplied.begin(), supplied.end(),
            [](const auto& lhs, const auto& rhs) {
              return lhs.input < rhs.input;
            });

  for (std::size_t index = 1; index < supplied.size(); ++index) {
    if (supplied[index - 1].input == supplied[index].input)
      throw error(error_code::duplicate_input,
                  "duplicate check input resource authority");
  }

  const auto& expected = request.inputs().inputs();
  if (supplied.size() != expected.size())
    throw error(error_code::missing_input,
                "check input resource set is incomplete");

  std::vector<package_input_resource> normalized;
  normalized.reserve(expected.size());
  for (const auto& authority : expected) {
    const auto& identity = authority.identity();
    const auto found = std::lower_bound(
        supplied.begin(), supplied.end(), identity,
        [](const package_input_resource& value, const auto& key) {
          return value.input < key;
        });

    if (found == supplied.end() || found->input != identity)
      throw error(error_code::missing_input,
                  "logical check input resource is missing");
    normalized.push_back(*found);
  }
  return normalized;
}

void normalize_execution_identity(execution_identity& identity)
{
  auto& groups = identity.supplementary_groups;
  std::sort(groups.begin(), groups.end());
  if (std::adjacent_find(groups.begin(), groups.end()) != groups.end())
    throw error(error_code::invalid_session,
                "supplementary credential groups must be unique");
  if (std::binary_search(groups.begin(), groups.end(), identity.group_id))
    throw error(error_code::invalid_session,
                "primary group must not be repeated as a supplementary group");
}

struct concrete_path final {
  const char* description;
  fs::path path;
};

void require_unique_resource_identities(
    const source_tree& source,
    const checked_package_tree& package,
    const std::vector<package_input_resource>& inputs)
{
  std::vector<pkgexec::resource_identity> identities;
  identities.reserve(inputs.size() + 2);
  identities.push_back(source.tree);
  identities.push_back(package.tree);
  for (const auto& input : inputs)
    identities.push_back(input.resource);

  std::sort(identities.begin(), identities.end());
  if (std::adjacent_find(identities.begin(), identities.end()) !=
      identities.end())
    throw error(error_code::invalid_session,
                "distinct check resources share one concrete identity");
}

void require_disjoint_resource_paths(
    const source_tree& source,
    const checked_package_tree& package,
    const std::vector<package_input_resource>& inputs,
    const session_paths& paths)
{
  std::vector<concrete_path> resources;
  resources.reserve(inputs.size() + 3);
  resources.push_back({"source tree", source.path});
  resources.push_back({"checked package tree", package.path});
  for (const auto& input : inputs)
    resources.push_back({"check input tree", input.path});
  resources.push_back({"temporary root", paths.temporary_root});

  for (std::size_t lhs = 0; lhs < resources.size(); ++lhs) {
    for (std::size_t rhs = lhs + 1; rhs < resources.size(); ++rhs) {
      if (paths_overlap(resources[lhs].path, resources[rhs].path)) {
        throw error(
            error_code::invalid_path,
            std::string(resources[lhs].description) + " path overlaps " +
                resources[rhs].description + " path");
      }
    }
  }
}

} // namespace

admitted_check_session::admitted_check_session(
    pkgcheck::check_request request,
    source_tree source,
    checked_package_tree package,
    std::vector<package_input_resource> inputs,
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
    std::vector<package_input_resource> inputs,
    session_paths paths,
    execution_identity identity,
    pkgexec::resource_limits limits)
{
  normalize_primary_paths(source, package, paths);
  require_source_authority(request, source);
  require_package_authority(request, package);
  inputs = normalize_and_validate_inputs(request, std::move(inputs));
  normalize_execution_identity(identity);

  require_unique_resource_identities(source, package, inputs);
  require_disjoint_resource_paths(source, package, inputs, paths);

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

const std::vector<package_input_resource>&
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
