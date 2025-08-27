/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/coro/coro.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/protocol/base_protocol.hpp>

namespace boost::asio {
  class io_context;
}  // namespace boost::asio

namespace libp2p::connection {
  class CapableConnection;
}  // namespace libp2p::connection

namespace libp2p::host {
  class BasicHost;
}  // namespace libp2p::host

namespace libp2p::protocol {
  struct PingConfig {
    // Fixes default field values with boost::di.
    PingConfig() = default;

    std::chrono::seconds timeout{20};
    std::chrono::seconds interval{15};
  };

  class Ping : public std::enable_shared_from_this<Ping>, public BaseProtocol {
   public:
    Ping(std::shared_ptr<boost::asio::io_context> io_context,
         std::shared_ptr<host::BasicHost> host,
         PingConfig config);

    // Adaptor
    ProtocolName getProtocolId() const override;

    // BaseProtocol
    void handle(std::shared_ptr<connection::Stream> stream) override;

    void start();

   private:
    Coro<void> ping(std::shared_ptr<connection::CapableConnection> connection);

    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<host::BasicHost> host_;
    PingConfig config_;
    event::Handle on_peer_connected_sub_;
  };
}  // namespace libp2p::protocol
