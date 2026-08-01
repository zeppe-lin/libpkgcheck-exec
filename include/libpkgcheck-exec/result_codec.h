// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file result_codec.h
 *  \brief Versioned durable encoding for retained check-execution evidence.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <libpkgcheck-exec/model.h>

namespace pkgcheck_exec {

/*! \brief Current canonical check-execution-result encoding. */
inline constexpr std::uint16_t check_execution_result_encoding_version = 1;
/*! \brief Hard refusal bound for one durable check-execution record. */
inline constexpr std::size_t maximum_check_execution_result_encoding_size =
    128U * 1024U * 1024U;

using check_execution_result_encoding = std::vector<std::uint8_t>;

/*! \brief Encode exact adapter-owned check evidence into canonical bytes. */
[[nodiscard]] check_execution_result_encoding encode_check_execution_result(
    const check_execution_result& result);

/*! \brief Decode evidence under exact caller-supplied semantic authorities.
 *
 * The check request, execution request, and backend profile bodies are not
 * reconstructed from identities. The decoder requires those exact authorities,
 * delegates execution evidence to libpkgexec's durable codec, rebuilds the
 * check result through public invariant-enforcing factories, and verifies every
 * retained identity and canonical byte.
 */
[[nodiscard]] check_execution_result decode_check_execution_result(
    const check_execution_result_encoding& encoding,
    pkgcheck::check_request check_request,
    pkgexec::execution_request execution_request,
    pkgexec::backend_capability_profile backend);

} // namespace pkgcheck_exec
