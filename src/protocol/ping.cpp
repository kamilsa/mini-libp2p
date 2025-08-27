/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/ping.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <libp2p/basic/read.hpp>
#include <libp2p/basic/write.hpp>
#include <libp2p/common/weak_macro.hpp>
#include <libp2p/coro/spawn.hpp>
#include <libp2p/coro/yield.hpp>
#include <libp2p/host/basic_host.hpp>
#include <qtils/bytes.hpp>

namespace libp2p::protocol {
  constexpr size_t kPingSize = 32;
  using PingMessage = qtils::BytesN<kPingSize>;

  Ping::Ping(std::shared_ptr<boost::asio::io_context> io_context,
             std::shared_ptr<host::BasicHost> host,
             PingConfig config)
      : io_context_{std::move(io_context)},
        host_{std::move(host)},
        config_{std::move(config)} {}

  ProtocolName Ping::getProtocolId() const {
    return "/ipfs/ping/1.0.0";
  }

  void Ping::handle(std::shared_ptr<connection::Stream> stream) {
    coroSpawn(*io_context_, [stream]() -> Coro<void> {
      PingMessage message;
      while (true) {
        auto r = co_await read(stream, message);
        if (not r.has_value()) {
          break;
        }
        r = co_await write(stream, message);
        if (not r.has_value()) {
          break;
        }
      }
    });
  }

  void Ping::start() {
    host_->listenProtocol(getProtocolId(), shared_from_this()).value();
    auto on_peer_connected =
        [WEAK_SELF](
            std::weak_ptr<connection::CapableConnection> weak_connection) {
          WEAK_LOCK(connection);
          WEAK_LOCK(self);
          coroSpawn(*self->io_context_, [self, connection]() -> Coro<void> {
            co_await self->ping(connection);
          });
        };
    on_peer_connected_sub_ =
        host_->getBus()
            .getChannel<event::network::OnNewConnectionChannel>()
            .subscribe(on_peer_connected);
  }

  Coro<void> Ping::ping(
      std::shared_ptr<connection::CapableConnection> connection) {
    co_await coroYield();
    boost::asio::steady_timer timer{*io_context_};
    std::shared_ptr<connection::Stream> stream;
    while (true) {
      if (stream == nullptr) {
        auto stream_result =
            co_await host_->newStream(connection, {getProtocolId()});
        if (not stream_result.has_value()) {
          break;
        }
        stream = stream_result.value();
      }
      // TODO: random
      PingMessage message;
      timer.expires_after(config_.timeout);
      timer.async_wait([stream](boost::system::error_code ec) {
        if (not ec) {
          stream->reset();
        }
      });
      auto r = co_await write(stream, message);
      if (r.has_value()) {
        r = co_await read(stream, message);
        // TODO: equal
      }
      if (not r.has_value()) {
        stream->reset();
        stream.reset();
      }
      timer.cancel();
      timer.expires_after(config_.interval);
      co_await timer.async_wait(boost::asio::use_awaitable);
    }
  }
}  // namespace libp2p::protocol
