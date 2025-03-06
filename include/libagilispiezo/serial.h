/*
 * libagilispiezo - A C++ library for controlling Newport Agilis Piezo Controllers
 * Copyright (C) 2025 Your Name
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef LIBAGILISPIEZO_SERIAL_H
#define LIBAGILISPIEZO_SERIAL_H

#include <asio.hpp>
#include <string>
#include <functional>

#define STOPBITS_TYPE asio::serial_port_base::stop_bits
#define ONESTOPBIT    STOPBITS_TYPE(STOPBITS_TYPE::one)
#define ONE5STOPBITS  STOPBITS_TYPE(STOPBITS_TYPE::onepointfive)
#define TWOSTOPBITS   STOPBITS_TYPE(STOPBITS_TYPE::two)

#define PARITY_TYPE asio::serial_port_base::parity
#define NOPARITY      PARITY_TYPE(PARITY_TYPE::none)
#define ODDPARITY     PARITY_TYPE(PARITY_TYPE::odd)
#define EVENPARITY    PARITY_TYPE(PARITY_TYPE::even)

namespace agilispiezo {

// Callback type for logging
using LogCallback = std::function<void(const std::string&)>;

class Serial {
public:
  Serial();
  ~Serial();

  bool Connect(const std::string& device_port_name,
    const unsigned int baud_rate, const unsigned int byte_size,
    const STOPBITS_TYPE stop_bits, const PARITY_TYPE parity,
    const int handshake_timeout_ms = 1000,
    const std::string& handshake_send = "",
    const std::string& handshake_expect = "");
  void Disconnect();
  bool IsConnected();
  size_t Send(const std::string& write);
  bool ListenUntil(std::string* read, const std::string& delimiter,
    const int timeout_ms);
  void FlushListen();
  void FlushSend();
  
  // Set callback for logging
  void SetLogCallback(LogCallback callback);

private:
  void Log(const std::string& message);

  std::unique_ptr<asio::serial_port> port_ = nullptr;
  asio::io_service io_;
  LogCallback log_callback_ = nullptr;
};

}

#endif // LIBAGILISPIEZO_SERIAL_H