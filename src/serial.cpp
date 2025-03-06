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

#include "libagilispiezo/serial.h"
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

namespace agilispiezo {

Serial::Serial() {}

Serial::~Serial() {
  Disconnect();
}

bool Serial::Connect(
  const std::string& device_port_name,
  const unsigned int baud_rate, const unsigned int byte_size,
  const STOPBITS_TYPE stop_bits, const PARITY_TYPE parity,
  const int handshake_timeout_ms,
  const std::string& handshake_send,
  const std::string& handshake_expect) {
  try {
    port_
      = std::make_unique<asio::serial_port>
      (asio::serial_port(io_, device_port_name));
    port_->set_option(
      asio::serial_port_base::baud_rate(baud_rate));
    port_->set_option(
      asio::serial_port_base::character_size(byte_size));
    port_->set_option(stop_bits);
    port_->set_option(parity);
    
    Log("Connected to serial port: " + device_port_name);
  }
  catch (const std::system_error& err) {
    Log("Error connecting to serial port: " + std::string(err.what()));
    return false;
  }
  
  if (handshake_expect.empty()) return true;
  
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Send(handshake_send);
  
  std::string rx;
  if (ListenUntil(&rx, handshake_expect, handshake_timeout_ms)) {
    Log("Handshake successful");
    return true;
  }
  
  Log("Handshake failed");
  Disconnect();
  return false;
}

void Serial::Disconnect() {
  if (port_ != nullptr) {
    try {
      port_->cancel();
      port_->close();
      port_.reset();
      io_.stop();
      io_.reset();
      port_ = nullptr;
      Log("Disconnected from serial port");
    }
    catch (const std::exception& err) {
      Log("Error during disconnect: " + std::string(err.what()));
    }
  }
}

bool Serial::IsConnected() {
  if (port_ == nullptr || !port_->is_open()) {
    return false;
  }
  
  // Try a non-blocking operation to see if the port is still valid
  try {
    size_t write_size = port_->write_some(asio::buffer(""));
    return true;
  }
  catch (const std::exception& err) {
    Log("Connection check failed: " + std::string(err.what()));
    return false;
  }
}

size_t Serial::Send(const std::string& write) {
  if (port_ == nullptr) {
    Log("Send failed: Port not open");
    return 0;
  }
  
  size_t write_size = 0;
  try {
    write_size = port_->write_some(asio::buffer(write));
    if (write_size > 0) {
      Log("Sent " + std::to_string(write_size) + " bytes: " + write);
    }
  }
  catch (const std::exception& err) {
    Log("Send error: " + std::string(err.what()));
    return 0;
  }
  return write_size;
}

bool Serial::ListenUntil(std::string* read,
  const std::string& delimiter,
  const int timeout_ms) {
  if (port_ == nullptr) {
    Log("ListenUntil failed: Port not open");
    return false;
  }
  
  int read_size;
  asio::streambuf received;
  
  auto future = std::async(std::launch::async, [this, delimiter, &read_size, &received]() {
    try {
      read_size = asio::read_until(*port_, received, delimiter);
      return true;
    }
    catch (const std::exception& err) {
      Log("Read error: " + std::string(err.what()));
      return false;
    }
  });
  
  if (future.wait_for(std::chrono::milliseconds(timeout_ms)) == std::future_status::timeout) {
    if (port_ != nullptr) {
      try {
        port_->cancel();
      }
      catch (const std::exception& err) {
        Log("Cancel error after timeout: " + std::string(err.what()));
      }
    }
    Log("ListenUntil timeout after " + std::to_string(timeout_ms) + "ms");
    return false;
  }
  
  if (!future.get()) {
    return false;
  }
  
  *read = {
    asio::buffers_begin(received.data()),
    asio::buffers_end(received.data())
  };
  
  Log("Received: " + *read);
  return true;
}

void Serial::FlushListen() {
  if (port_ == nullptr) return;
  
  try {
#if defined(_WIN64) || defined(_WIN32)
    PurgeComm(port_->lowest_layer().native_handle(), PURGE_TXCLEAR);
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    int fd = port_->lowest_layer().native_handle();
    if (fd >= 0) tcflush(fd, TCIFLUSH);
#else
    Log("Flushing receive buffer not supported on this platform");
#endif
    Log("Flushed receive buffer");
  }
  catch (const std::exception& err) {
    Log("FlushListen error: " + std::string(err.what()));
  }
}

void Serial::FlushSend() {
  if (port_ == nullptr) return;
  
  try {
#if defined(_WIN64) || defined(_WIN32)
    PurgeComm(port_->lowest_layer().native_handle(), PURGE_RXCLEAR);
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    int fd = port_->lowest_layer().native_handle();
    if (fd >= 0) tcflush(fd, TCOFLUSH);
#else
    Log("Flushing send buffer not supported on this platform");
#endif
    Log("Flushed send buffer");
  }
  catch (const std::exception& err) {
    Log("FlushSend error: " + std::string(err.what()));
  }
}

void Serial::SetLogCallback(LogCallback callback) {
  log_callback_ = callback;
}

void Serial::Log(const std::string& message) {
  if (log_callback_) {
    log_callback_(message);
  }
}

}