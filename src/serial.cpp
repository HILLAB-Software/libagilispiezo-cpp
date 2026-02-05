/*
 * libagilispiezo - A C++ library for controlling Newport Agilis Piezo Controllers
 * Copyright (C) 2025 HIL Lab. Inc
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

#include "serial.h"
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

namespace agilispiezo {

Serial::Serial() {
  StartIOThread();
}

Serial::~Serial() {
  Disconnect();
  StopIOThread();
}

void Serial::StartIOThread() {
  // Create work object to keep io_service running
  work_ = std::make_unique<asio::io_service::work>(io_);
  
  // Start io_service in background thread
  io_thread_ = std::thread([this]() {
    io_.run();
  });
}

void Serial::StopIOThread() {
  if (work_) {
    work_.reset(); // Allow io_service to finish
  }
  
  if (!io_.stopped()) {
    io_.stop();
  }
  
  if (io_thread_.joinable()) {
    io_thread_.join();
  }
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
    port_->set_option(
      asio::serial_port_base::flow_control(
        asio::serial_port_base::flow_control::none));
    
    Log("Connected to serial port: " + device_port_name);
    Log("Port settings - Baud: " + std::to_string(baud_rate) + 
        ", ByteSize: " + std::to_string(byte_size) + 
        ", StopBits: " + std::to_string(stop_bits.value()) +
        ", Parity: " + std::to_string(parity.value()));
  }
  catch (const std::system_error& err) {
    Log("Error connecting to serial port: " + std::string(err.what()));
    return false;
  }
  
  if (handshake_expect.empty()) return true;
  
  Log("Waiting 100ms before handshake...");
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Flush any existing data in buffer
  FlushListen();
  
  Log("Sending handshake: [" + EscapeString(handshake_send) + "] (hex: " + BytesToHex(handshake_send) + ")");
  Send(handshake_send);
  
  Log("Expecting handshake response ending with: [" + EscapeString(handshake_expect) + 
      "] (hex: " + BytesToHex(handshake_expect) + ")");
  
  std::string rx;
  if (ListenUntil(&rx, handshake_expect, handshake_timeout_ms)) {
    Log("Handshake successful");
    return true;
  }
  
  Log("Handshake failed - trying to read any available data...");
  
  // Try to read any data without delimiter
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
  int fd = port_->lowest_layer().native_handle();
  int bytes_available = 0;
  if (fd >= 0) {
    if (ioctl(fd, FIONREAD, &bytes_available) == 0) {
      Log("Checking for remaining data in buffer...");
      if (bytes_available > 0) {
        Log("Found " + std::to_string(bytes_available) + " bytes in buffer after handshake timeout");
        try {
          std::vector<char> buffer(bytes_available);
          size_t read_count = port_->read_some(asio::buffer(buffer));
          if (read_count > 0) {
            std::string data(buffer.begin(), buffer.begin() + read_count);
            Log("Read raw data (" + std::to_string(read_count) + " bytes): [" + 
                EscapeString(data) + "] (hex: " + BytesToHex(data) + ")");
          } else {
            Log("read_some returned 0 bytes");
          }
        } catch (const std::exception& err) {
          Log("Error reading raw data: " + std::string(err.what()));
        }
      } else {
        Log("No data in buffer after timeout");
      }
    } else {
      Log("ioctl FIONREAD failed");
    }
  }
#endif
  
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
      Log("Sent " + std::to_string(write_size) + " bytes: [" + EscapeString(write) + 
          "] (hex: " + BytesToHex(write) + ")");
    }
  }
  catch (const std::exception& err) {
    Log("Send error: " + std::string(err.what()));
    return 0;
  }
  
  // Give device time to process
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  return write_size;
}

bool Serial::ListenUntil(std::string* read,
  const std::string& delimiter,
  const int timeout_ms) {
  if (port_ == nullptr) {
    Log("ListenUntil failed: Port not open");
    return false;
  }
  
  // First check if any data is available (non-blocking)
  try {
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    int fd = port_->lowest_layer().native_handle();
    int bytes_available = 0;
    if (fd >= 0) {
      ioctl(fd, FIONREAD, &bytes_available);
      if (bytes_available > 0) {
        Log("Available bytes in buffer: " + std::to_string(bytes_available));
      } else {
        Log("No data available in buffer, waiting for response...");
      }
    }
#else
    Log("Waiting for response...");
#endif
  } catch (const std::exception& err) {
    Log("Error checking available data: " + std::string(err.what()));
  }
  
  auto start_time = std::chrono::steady_clock::now();
  
  // Reset io_service for this operation
  io_.reset();
  
  asio::streambuf received;
  std::error_code read_ec;
  std::error_code timer_ec;
  bool timeout_occurred = false;
  bool read_completed = false;
  size_t bytes_read = 0;
  
  // Use promise/future for synchronization
  std::promise<void> completion_promise;
  std::future<void> completion_future = completion_promise.get_future();
  
  // Create deadline timer using the same io_service as port
  auto timer = std::make_shared<asio::steady_timer>(io_, std::chrono::milliseconds(timeout_ms));
  
  // Start async read
  asio::async_read_until(*port_, received, delimiter,
    [&, timer](const std::error_code& ec, size_t bytes) {
      read_ec = ec;
      bytes_read = bytes;
      read_completed = true;
      timer->cancel(); // Cancel timer if read completes first
      
      if (!ec) {
        Log("Async read completed successfully, bytes: " + std::to_string(bytes));
      } else if (ec == asio::error::operation_aborted) {
        // Read was cancelled due to timeout
        Log("Async read was cancelled");
        // Check if any data was read before cancellation
        size_t buffered = received.size();
        if (buffered > 0) {
          Log("But found " + std::to_string(buffered) + " bytes in buffer");
        }
      } else {
        Log("Async read completed with error: " + ec.message());
      }
      
      completion_promise.set_value();
    });
  
  // Start timeout timer
  timer->async_wait([&, timer](const std::error_code& ec) {
    timer_ec = ec;
    if (!ec) {
      // Timer expired naturally (timeout)
      timeout_occurred = true;
      Log("Timeout occurred, cancelling read operation");
      
      // Check buffer BEFORE cancelling
      size_t buffered = received.size();
      if (buffered > 0) {
        Log("Found " + std::to_string(buffered) + " bytes in async buffer before cancel");
      }
      
      port_->cancel(); // Cancel the read operation
    } else if (ec == asio::error::operation_aborted) {
      // Timer was cancelled (read completed first)
      Log("Timer cancelled - read completed before timeout");
    }
  });
  
  // Wait for completion (either read or timeout)
  completion_future.wait();
  
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now() - start_time).count();
  
  // Check results
  if (timeout_occurred) {
    Log("ListenUntil timeout after " + std::to_string(elapsed) + "ms");
    
    // Check if any partial data was received
    size_t available_data = received.size();
    if (available_data > 0) {
      std::string partial_data = {
        asio::buffers_begin(received.data()),
        asio::buffers_end(received.data())
      };
      Log("Received partial data (" + std::to_string(available_data) + 
          " bytes): [" + EscapeString(partial_data) + "] (hex: " + BytesToHex(partial_data) + ")");
    } else {
      Log("No partial data in async buffer");
    }
    return false;
  }
  
  if (read_ec) {
    if (read_ec == asio::error::operation_aborted) {
      Log("Read operation was cancelled (timeout)");
      
      // Check if we got partial data before cancellation
      size_t buffered = received.size();
      if (buffered > 0) {
        std::string partial_data = {
          asio::buffers_begin(received.data()),
          asio::buffers_end(received.data())
        };
        Log("Partial data from cancelled read (" + std::to_string(buffered) + 
            " bytes): [" + EscapeString(partial_data) + "] (hex: " + BytesToHex(partial_data) + ")");
      }
    } else {
      Log("Read error: " + read_ec.message());
    }
    return false;
  }
  
  if (!read_completed || bytes_read == 0) {
    Log("Read did not complete or no data received");
    return false;
  }
  
  // Extract data from buffer
  *read = {
    asio::buffers_begin(received.data()),
    asio::buffers_end(received.data())
  };
  
  Log("Received " + std::to_string(bytes_read) + " bytes in " + std::to_string(elapsed) + 
      "ms: [" + EscapeString(*read) + "] (hex: " + BytesToHex(*read) + ")");
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

std::string Serial::BytesToHex(const std::string& data) {
  std::ostringstream oss;
  for (unsigned char c : data) {
    oss << std::hex << std::setfill('0') << std::setw(2) << (int)c << " ";
  }
  return oss.str();
}

std::string Serial::EscapeString(const std::string& data) {
  std::string result;
  for (char c : data) {
    switch (c) {
      case '\r': result += "\\r"; break;
      case '\n': result += "\\n"; break;
      case '\t': result += "\\t"; break;
      case '\\': result += "\\\\"; break;
      default:
        if (c >= 32 && c < 127) {
          result += c;
        } else {
          std::ostringstream oss;
          oss << "\\x" << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)c;
          result += oss.str();
        }
    }
  }
  return result;
}

}