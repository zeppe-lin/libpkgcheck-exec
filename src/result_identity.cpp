// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "result_identity.h"

#include <libpkgcheck-exec/error.h>

#include <openssl/evp.h>

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace pkgcheck_exec::detail {
namespace {

std::string sha256_hex(std::string_view material)
{
  using context_pointer =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

  context_pointer context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context ||
      EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context.get(), material.data(), material.size()) != 1)
    throw error(error_code::identity_failed,
                "cannot hash check execution evidence");

  std::array<unsigned char, 32> digest{};
  unsigned int digest_size = 0;
  if (EVP_DigestFinal_ex(context.get(), digest.data(), &digest_size) != 1 ||
      digest_size != digest.size())
    throw error(error_code::identity_failed,
                "cannot finalize check execution evidence");

  static constexpr char digits[] = "0123456789abcdef";
  std::string result(digest.size() * 2U, '0');
  for (std::size_t index = 0; index < digest.size(); ++index) {
    result[index * 2U] = digits[digest[index] >> 4U];
    result[index * 2U + 1U] = digits[digest[index] & 0x0fU];
  }
  return result;
}

std::string identity_material(std::string_view domain,
                              std::string_view authority)
{
  std::string material;
  material.reserve(domain.size() + authority.size());
  material.append(domain);
  material.append(authority);
  return material;
}

} // namespace

pkgcheck::check_execution_evidence_identity execution_evidence_identity(
    const pkgexec::execution_result& execution)
{
  const auto material = identity_material(
      "pkgcheck/execution-evidence/v1", execution.identity().hex());
  return pkgcheck::check_execution_evidence_identity::from_sha256(
      sha256_hex(material));
}

pkgcheck::check_failure_evidence_identity failure_evidence_identity(
    const pkgexec::execution_result& execution)
{
  const auto material = identity_material(
      "pkgcheck/failure-evidence/v1", execution.identity().hex());
  return pkgcheck::check_failure_evidence_identity::from_sha256(
      sha256_hex(material));
}

pkgcheck::check_failure_kind classify_failure(
    const pkgexec::execution_result& execution) noexcept
{
  if (execution.failure() &&
      *execution.failure() == pkgexec::execution_failure_kind::cancelled)
    return pkgcheck::check_failure_kind::cancelled;

  if (execution.start_state() ==
      pkgexec::execution_start_state::not_started)
    return pkgcheck::check_failure_kind::execution_unavailable;

  return pkgcheck::check_failure_kind::program_failed;
}

pkgcheck::check_result expected_check_result(
    const pkgexec::execution_result& execution,
    pkgcheck::check_request request)
{
  auto evidence = execution_evidence_identity(execution);
  if (execution.status() == pkgexec::execution_status::succeeded)
    return pkgcheck::check_result::passed(
        std::move(request), std::move(evidence));

  return pkgcheck::check_result::failed(
      std::move(request), std::move(evidence), classify_failure(execution),
      failure_evidence_identity(execution));
}

} // namespace pkgcheck_exec::detail
