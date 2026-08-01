// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgcheck-exec/result_codec.h>

#include <libpkgcheck-exec/error.h>
#include <libpkgexec/result_codec.h>

#include "result_identity.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace pkgcheck_exec {
namespace {

constexpr std::array<std::uint8_t, 8> encoding_magic{
    'P', 'K', 'G', 'C', 'X', 'R', '1', 0};
constexpr std::size_t checksum_size = 32U;

[[noreturn]] void inconsistent(const std::string& message)
{
  throw error(error_code::inconsistent_result, message);
}

[[noreturn]] void corrupt(const std::string& message)
{
  throw error(error_code::corrupt_encoding, message);
}

[[noreturn]] void mismatch(const std::string& message)
{
  throw error(error_code::authority_mismatch, message);
}

std::string sha256_hex(std::string_view value)
{
  using context_pointer =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  context_pointer context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context ||
      EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context.get(), value.data(), value.size()) != 1)
    inconsistent("cannot initialize check-execution record checksum");

  std::array<unsigned char, 32> output{};
  unsigned int size = 0;
  if (EVP_DigestFinal_ex(context.get(), output.data(), &size) != 1 ||
      size != output.size())
    inconsistent("cannot finalize check-execution record checksum");

  static constexpr char digits[] = "0123456789abcdef";
  std::string result(output.size() * 2U, '0');
  for (std::size_t index = 0; index < output.size(); ++index) {
    result[index * 2U] = digits[output[index] >> 4U];
    result[index * 2U + 1U] = digits[output[index] & 0x0fU];
  }
  return result;
}

class writer final {
public:
  void byte(std::uint8_t value)
  {
    output_.push_back(value);
    check_size();
  }

  void u16(std::uint16_t value)
  {
    byte(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    byte(static_cast<std::uint8_t>(value & 0xffU));
  }

  void u32(std::uint32_t value)
  {
    for (int shift = 24; shift >= 0; shift -= 8)
      byte(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }

  void boolean(bool value)
  {
    byte(value ? 1U : 0U);
  }

  void raw(const std::uint8_t* data, std::size_t size)
  {
    if (size == 0U)
      return;
    if (size > maximum_check_execution_result_encoding_size - output_.size())
      inconsistent("check-execution encoding exceeds maximum size");
    output_.insert(output_.end(), data, data + size);
  }

  void bytes(const std::vector<std::uint8_t>& value)
  {
    if (value.size() > std::numeric_limits<std::uint32_t>::max())
      inconsistent("embedded execution evidence is too large");
    u32(static_cast<std::uint32_t>(value.size()));
    raw(value.data(), value.size());
  }

  void identity(std::string_view value)
  {
    if (value.size() != 64U ||
        !std::all_of(value.begin(), value.end(), [](char current) {
          return (current >= '0' && current <= '9') ||
                 (current >= 'a' && current <= 'f');
        }))
      inconsistent("check-execution record contains an invalid identity");
    for (std::size_t index = 0; index < value.size(); index += 2U)
      byte(static_cast<std::uint8_t>((digit(value[index]) << 4U) |
                                     digit(value[index + 1U])));
  }

  const check_execution_result_encoding& output() const noexcept
  {
    return output_;
  }

  check_execution_result_encoding finish()
  {
    return std::move(output_);
  }

private:
  static std::uint8_t digit(char value)
  {
    return value >= '0' && value <= '9'
        ? static_cast<std::uint8_t>(value - '0')
        : static_cast<std::uint8_t>(value - 'a' + 10);
  }

  void check_size() const
  {
    if (output_.size() > maximum_check_execution_result_encoding_size)
      inconsistent("check-execution encoding exceeds maximum size");
  }

  check_execution_result_encoding output_;
};

class reader final {
public:
  reader(const check_execution_result_encoding& input, std::size_t limit)
      : input_(input), limit_(limit)
  {
  }

  std::uint8_t byte()
  {
    require(1U);
    return input_[offset_++];
  }

  std::uint16_t u16()
  {
    std::uint16_t value = 0U;
    for (int index = 0; index < 2; ++index)
      value = static_cast<std::uint16_t>((value << 8U) | byte());
    return value;
  }

  std::uint32_t u32()
  {
    std::uint32_t value = 0U;
    for (int index = 0; index < 4; ++index)
      value = (value << 8U) | byte();
    return value;
  }

  bool boolean()
  {
    const auto value = byte();
    if (value > 1U)
      corrupt("check-execution encoding contains an invalid boolean");
    return value == 1U;
  }

  std::string identity()
  {
    static constexpr char digits[] = "0123456789abcdef";
    require(32U);
    std::string value(64U, '0');
    for (std::size_t index = 0; index < 32U; ++index) {
      const auto current = input_[offset_++];
      value[index * 2U] = digits[(current >> 4U) & 0x0fU];
      value[index * 2U + 1U] = digits[current & 0x0fU];
    }
    return value;
  }

  std::vector<std::uint8_t> bytes(std::size_t maximum)
  {
    const auto size = static_cast<std::size_t>(u32());
    if (size > maximum)
      corrupt("embedded execution evidence exceeds its limit");
    require(size);
    std::vector<std::uint8_t> value(
        input_.begin() + static_cast<std::ptrdiff_t>(offset_),
        input_.begin() + static_cast<std::ptrdiff_t>(offset_ + size));
    offset_ += size;
    return value;
  }

  void finish() const
  {
    if (offset_ != limit_)
      corrupt("check-execution encoding contains trailing payload bytes");
  }

private:
  void require(std::size_t size) const
  {
    if (offset_ > limit_ || size > limit_ - offset_)
      corrupt("check-execution encoding is truncated");
  }

  const check_execution_result_encoding& input_;
  std::size_t limit_;
  std::size_t offset_ = 0U;
};

pkgcheck::check_outcome decode_outcome(std::uint8_t value)
{
  if (value > static_cast<std::uint8_t>(pkgcheck::check_outcome::failed))
    corrupt("check-execution encoding contains an unknown check outcome");
  return static_cast<pkgcheck::check_outcome>(value);
}

pkgcheck::check_failure_kind decode_failure(std::uint8_t value)
{
  if (value > static_cast<std::uint8_t>(
                  pkgcheck::check_failure_kind::cancelled))
    corrupt("check-execution encoding contains an unknown check failure");
  return static_cast<pkgcheck::check_failure_kind>(value);
}

void validate_result(const check_execution_result& result)
{
  const auto expected = detail::expected_check_result(
      result.execution(), result.check().request());
  if (result.check().identity() != expected.identity() ||
      result.check().outcome() != expected.outcome() ||
      result.check().execution_evidence() !=
          expected.execution_evidence() ||
      result.check().failure() != expected.failure() ||
      result.check().failure_evidence() != expected.failure_evidence())
    inconsistent("check result does not match retained execution evidence");
}

} // namespace

namespace detail {

class codec_access final {
public:
  static check_execution_result make(
      pkgexec::execution_result execution,
      pkgcheck::check_result check)
  {
    return check_execution_result(std::move(execution), std::move(check));
  }
};

} // namespace detail

check_execution_result_encoding encode_check_execution_result(
    const check_execution_result& result)
{
  validate_result(result);

  writer output;
  output.raw(encoding_magic.data(), encoding_magic.size());
  output.u16(check_execution_result_encoding_version);
  output.identity(result.check().request().identity().hex());
  output.identity(result.execution().request().identity().hex());
  output.identity(result.execution().backend().identity().hex());
  output.identity(result.execution().identity().hex());
  output.identity(result.check().identity().hex());
  output.bytes(pkgexec::encode_execution_result(result.execution()));
  output.byte(static_cast<std::uint8_t>(result.check().outcome()));
  output.identity(result.check().execution_evidence().hex());
  output.boolean(result.check().failure().has_value());
  if (result.check().failure()) {
    output.byte(static_cast<std::uint8_t>(*result.check().failure()));
    if (!result.check().failure_evidence())
      inconsistent("failed check result lacks failure evidence");
    output.identity(result.check().failure_evidence()->hex());
  }

  const auto& payload = output.output();
  output.identity(sha256_hex(std::string_view(
      reinterpret_cast<const char*>(payload.data()), payload.size())));
  return output.finish();
}

check_execution_result decode_check_execution_result(
    const check_execution_result_encoding& encoding,
    pkgcheck::check_request check_request,
    pkgexec::execution_request execution_request,
    pkgexec::backend_capability_profile backend)
{
  try {
    if (encoding.size() > maximum_check_execution_result_encoding_size)
      corrupt("check-execution encoding exceeds maximum size");
    if (encoding.size() < encoding_magic.size() + 2U + checksum_size)
      corrupt("check-execution encoding is truncated");

    const auto payload_size = encoding.size() - checksum_size;
    const auto actual_checksum = sha256_hex(std::string_view(
        reinterpret_cast<const char*>(encoding.data()), payload_size));
    static constexpr char digits[] = "0123456789abcdef";
    std::string retained_checksum(64U, '0');
    for (std::size_t index = 0; index < checksum_size; ++index) {
      const auto current = encoding[payload_size + index];
      retained_checksum[index * 2U] = digits[(current >> 4U) & 0x0fU];
      retained_checksum[index * 2U + 1U] = digits[current & 0x0fU];
    }
    if (retained_checksum != actual_checksum)
      corrupt("check-execution encoding checksum mismatch");

    reader input(encoding, payload_size);
    for (const auto expected : encoding_magic) {
      if (input.byte() != expected)
        corrupt("check-execution encoding has invalid magic");
    }
    if (input.u16() != check_execution_result_encoding_version)
      corrupt("check-execution encoding version is unsupported");

    const auto check_request_identity = input.identity();
    const auto execution_request_identity = input.identity();
    const auto backend_identity = input.identity();
    const auto execution_identity = input.identity();
    const auto check_identity = input.identity();
    if (check_request.identity().hex() != check_request_identity)
      mismatch("check-execution record belongs to another check request");
    if (execution_request.identity().hex() != execution_request_identity)
      mismatch("check-execution record belongs to another execution request");
    if (backend.identity().hex() != backend_identity)
      mismatch("check-execution record belongs to another backend profile");
    if (execution_request.purpose().kind() !=
        pkgexec::execution_purpose_kind::check)
      mismatch("supplied execution request is not a check request");

    auto execution_encoding = input.bytes(
        pkgexec::maximum_execution_result_encoding_size);
    auto execution = [&]() -> pkgexec::execution_result {
      try {
        return pkgexec::decode_execution_result(
            execution_encoding, std::move(execution_request),
            std::move(backend));
      } catch (const pkgexec::error& problem) {
        if (problem.code() == pkgexec::error_code::authority_mismatch)
          mismatch("embedded execution evidence belongs to another authority");
        corrupt("embedded execution evidence is invalid: " +
                std::string(problem.what()));
      }
    }();
    if (execution.identity().hex() != execution_identity)
      corrupt("embedded execution evidence identity mismatch");

    const auto outcome = decode_outcome(input.byte());
    const auto execution_evidence = input.identity();
    const auto has_failure = input.boolean();
    std::optional<pkgcheck::check_failure_kind> failure;
    std::optional<std::string> failure_evidence;
    if (has_failure) {
      failure = decode_failure(input.byte());
      failure_evidence = input.identity();
    }
    input.finish();

    auto check = detail::expected_check_result(
        execution, std::move(check_request));
    if (check.outcome() != outcome)
      corrupt("check outcome does not match execution evidence");
    if (check.execution_evidence().hex() != execution_evidence)
      corrupt("check execution-evidence identity mismatch");
    if (check.failure() != failure)
      corrupt("check failure kind does not match execution evidence");
    if (check.failure_evidence().has_value() != failure_evidence.has_value())
      corrupt("check failure-evidence presence mismatch");
    if (check.failure_evidence() &&
        check.failure_evidence()->hex() != *failure_evidence)
      corrupt("check failure-evidence identity mismatch");
    if (check.identity().hex() != check_identity)
      corrupt("check result identity mismatch");

    auto decoded = detail::codec_access::make(
        std::move(execution), std::move(check));
    validate_result(decoded);
    if (encode_check_execution_result(decoded) != encoding)
      corrupt("check-execution encoding is not canonical");
    return decoded;
  } catch (const error& problem) {
    if (problem.code() == error_code::corrupt_encoding ||
        problem.code() == error_code::authority_mismatch)
      throw;
    throw error(error_code::corrupt_encoding,
                "check-execution encoding violates the result contract: " +
                    std::string(problem.what()));
  } catch (const std::exception& problem) {
    throw error(error_code::corrupt_encoding,
                "check-execution encoding violates a subordinate contract: " +
                    std::string(problem.what()));
  }
}

} // namespace pkgcheck_exec
