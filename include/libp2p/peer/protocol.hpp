/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace libp2p::peer {

  using ProtocolName = std::string;
}  // namespace libp2p::peer

namespace libp2p {
  using peer::ProtocolName;
}  // namespace libp2p
