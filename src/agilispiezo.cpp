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


#include "libagilispiezo/agilispiezo.h"
#include <chrono>
#include <iostream>
#include <sstream>

namespace agilispiezo {

AgilisPiezo::AgilisPiezo() {
  serial_ = std::make_unique<Serial>();
  serial_->SetLogCallback([this](const std::string& message) {
    __Log(LOG_DEBUG, "Serial: " + message);
  });
  cmd_term_timer_.Start();
}

AgilisPiezo::~AgilisPiezo() {
  __Log(LOG_INFO, "Destroying AgilisPiezo instance");
  DisconnectDevice();
}

bool AgilisPiezo::ConnectDeviceUSB(const std::string& port_name) {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Connecting to USB device on port: " + port_name);
  if (serial_->Connect(port_name, 921600, 8,
    ONESTOPBIT, NOPARITY, 1000, "VE\r\n", "\r\n"
  )) {
    last_port_name_ = port_name;
    __Log(LOG_INFO, "Successfully connected to USB device");
    return true;
  }
  __Log(LOG_ERROR, "Failed to connect to USB device");
  return false;
}

bool AgilisPiezo::ConnectDeviceRS232(const std::string& port_name) {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Connecting to RS232 device on port: " + port_name);
  if (serial_->Connect(port_name, 115200, 8,
    ONESTOPBIT, NOPARITY, 1000, "VE\r\n", "\r\n"
  )) {
    last_port_name_ = port_name;
    __Log(LOG_INFO, "Successfully connected to RS232 device");
    return true;
  }
  __Log(LOG_ERROR, "Failed to connect to RS232 device");
  return false;
}

void AgilisPiezo::DisconnectDevice() {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Disconnecting device");
  serial_->Disconnect();
  last_port_name_.clear();
}

bool AgilisPiezo::IsConnected() const {
  std::string buf;
  __Log(LOG_DEBUG, "Checking connection status");
  bool ret = GetControllerFirmwareVersion(&buf);
  __Log(LOG_DEBUG, "Connection status: " + std::string(ret ? "Connected" : "Disconnected"));
  return ret;
}

std::string AgilisPiezo::GetErrorText(const int e) const {
  switch (e) {
  case 0: return "0: No error.";
  case -1: return "-1: Unknown command.";
  case -2: return "-2: Axis out of range (must be 1 or 2, or must not be specified).";
  case -3: return "-3: Wrong format for parameter nn (or must not be specified).";
  case -4: return "-4: Parameter nn out of range.";
  case -5: return "-5: Not allowed in local mode.";
  case -6: return "-6: Not allowed in current state.";
  case 1: return "1: Communication sync failed so reconfigure the port.";
  case 8: return "8: TE command failed to sent.";
  case 9: return "9: Write serial failed.";
  default: return std::to_string(e) + ": Undefined error code.";
  }
}

std::string AgilisPiezo::GetPortName() const {
  return last_port_name_;
}

bool AgilisPiezo::SetStepDelay(const int axis, const int delay) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "SetStepDelay: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Setting step delay for axis " + std::to_string(axis) + " to " + std::to_string(delay));
  return __SendCommand(DL[axis] + std::to_string(delay));
}

bool AgilisPiezo::GetStepDelay(const int axis, int* out_delay) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "GetStepDelay: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  __Log(LOG_INFO, "Getting step delay for axis " + std::to_string(axis));
  const bool ret = __SendCommand(DL[axis] + "?");
  __GetReturnValue(buf);
  __GetIntegerFromReturnValue(buf, DL[axis], out_delay);
  __Log(LOG_INFO, "Step delay for axis " + std::to_string(axis) + ": " + std::to_string(*out_delay));
  return ret;
}

bool AgilisPiezo::StartJogMotion(
  const int axis, const bool sign, const int jog_speed) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "StartJogMotion: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  int speed = jog_speed;
  if (!sign) speed = -speed;
  __Log(LOG_INFO, "Starting jog motion for axis " + std::to_string(axis) + 
        " with speed " + std::to_string(speed));
  return __SendCommand(JA[axis] + std::to_string(speed));
}

bool AgilisPiezo::GetJogMode(
  const int axis, bool* out_sign, int* out_jog_speed) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "GetJogMode: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  __Log(LOG_INFO, "Getting jog mode for axis " + std::to_string(axis));
  const bool ret = __SendCommand(JA[axis] + "?");
  __GetReturnValue(buf);
  __GetIntegerFromReturnValue(buf, JA[axis], out_jog_speed);
  if (*out_jog_speed < 0) {
    *out_sign = false;
    *out_jog_speed = -(*out_jog_speed);
  }
  else {
    *out_sign = true;
  }
  __Log(LOG_INFO, "Jog mode for axis " + std::to_string(axis) + ": sign=" + 
        (*out_sign ? "positive" : "negative") + ", speed=" + std::to_string(*out_jog_speed));
  return ret; 
}

bool AgilisPiezo::MeasureCurrentPosition(
  const int axis, std::future<int>* out_position) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "MeasureCurrentPosition: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Measuring current position for axis " + std::to_string(axis));
  const bool ret = __SendCommand(MA[axis]);
  *out_position = std::async(std::launch::async, [=]() {
    std::string buf;
    __Log(LOG_INFO, "Waiting for position measurement result (up to 130 seconds)");
    __GetReturnValue(buf, 130000); // Up to 130 seconds 
    int v = 0;
    bool success = __GetIntegerFromReturnValue(buf, MA[axis], &v);
    if (success) {
      __Log(LOG_INFO, "Position measurement for axis " + std::to_string(axis) + ": " + std::to_string(v));
    } else {
      __Log(LOG_ERROR, "Failed to parse position measurement result");
    }
    return v;
  });
  return ret;
}

bool AgilisPiezo::SetToLocalMode() const {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Setting to local mode");
  return __SendCommand("ML");
}

bool AgilisPiezo::SetToRemoteMode() const {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Setting to remote mode");
  return __SendCommand("MR");
}

bool AgilisPiezo::MoveToLimit(
  const int axis, const bool sign,
  const int jog_speed) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "MoveToLimit: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string direction = sign ? "positive" : "negative";
  __Log(LOG_INFO, "Moving axis " + std::to_string(axis) + " to " + direction + 
        " limit with speed " + std::to_string(jog_speed));
  return __SendCommand(MV[axis] + (sign ? "" : "-") + std::to_string(jog_speed));
}

bool AgilisPiezo::AbsoluteMove(const int axis, const int position) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "AbsoluteMove: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Moving axis " + std::to_string(axis) + " to absolute position " + 
        std::to_string(position));
  return __SendCommand(PA[axis] + std::to_string(position));
}

bool AgilisPiezo::TellLimitStatus(bool* out_axis1, bool* out_axis2) const {
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  __Log(LOG_INFO, "Getting limit status");
  const bool ret = __SendCommand(PH);
  __GetReturnValue(buf);
  int v;
  __GetIntegerFromReturnValue(buf, PH, &v);
  switch (v) {
  case 0: *out_axis1 = false, *out_axis2 = false; break;
  case 1: *out_axis1 = true, *out_axis2 = false; break;
  case 2: *out_axis1 = false, *out_axis2 = true; break;
  case 3: *out_axis1 = true, *out_axis2 = true; break;
  }
  __Log(LOG_INFO, "Limit status: axis1=" + std::string(*out_axis1 ? "at limit" : "not at limit") + 
        ", axis2=" + std::string(*out_axis2 ? "at limit" : "not at limit"));
  return ret;
}

bool AgilisPiezo::RelativeMove(
  const int axis, const bool sign, const int steps) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "RelativeMove: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string direction = sign ? "positive" : "negative";
  __Log(LOG_INFO, "Moving axis " + std::to_string(axis) + " " + std::to_string(steps) + 
        " steps in " + direction + " direction");
  return __SendCommand(PR[axis] + (sign ? "" : "-") + std::to_string(steps));
}

bool AgilisPiezo::ResetController() const {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Resetting controller");
  return __SendCommand("RS");
}

bool AgilisPiezo::StopMotion(const int axis) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "StopMotion: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Stopping motion for axis " + std::to_string(axis));
  return __SendCommand(ST[axis]);
}

bool AgilisPiezo::SetStepAmplitude(
  const int axis, const bool sign, const int amplitude) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "SetStepAmplitude: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  if (amplitude == 0 || amplitude < -50 || amplitude > 50) {
    __Log(LOG_ERROR, "SetStepAmplitude: Invalid amplitude (must be between -50 and 50, excluding 0)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string direction = sign ? "positive" : "negative";
  __Log(LOG_INFO, "Setting step amplitude for axis " + std::to_string(axis) + 
        " to " + std::to_string(amplitude) + " in " + direction + " direction");
  return __SendCommand(SU[axis] + (sign ? "" : "-") + std::to_string(amplitude));
}

bool AgilisPiezo::GetStepAmplitudeSetting(
  const int axis, const bool sign, int* out_amplitude) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "GetStepAmplitudeSetting: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  std::string direction = sign ? "positive" : "negative";
  __Log(LOG_INFO, "Getting step amplitude for axis " + std::to_string(axis) + 
        " in " + direction + " direction");
  const bool ret = __SendCommand(SU[axis] + (sign ? "?" : "-?"));
  __GetReturnValue(buf);
  __GetIntegerFromReturnValue(buf, SU[axis], out_amplitude);
  if (*out_amplitude < 0) *out_amplitude = -(*out_amplitude);
  __Log(LOG_INFO, "Step amplitude for axis " + std::to_string(axis) + 
        " in " + direction + " direction: " + std::to_string(*out_amplitude));
  return ret;
}

bool AgilisPiezo::GetErrorOfPreviousCommand(int* out_error_code) const {
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  __Log(LOG_INFO, "Getting error of previous command");
  const bool ret = __SendCommand("TE");
  __GetReturnValue(buf);
  __GetIntegerFromReturnValue(buf, "TE", out_error_code);
  __Log(LOG_INFO, "Error of previous command: " + std::to_string(*out_error_code) + 
        " (" + GetErrorText(*out_error_code) + ")");
  return ret;
}

bool AgilisPiezo::TellNumberOfSteps(const int axis, int* out_steps) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "TellNumberOfSteps: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  __Log(LOG_INFO, "Getting number of steps for axis " + std::to_string(axis));
  const bool ret = __SendCommand(TP[axis]);
  __GetReturnValue(buf);
  __GetIntegerFromReturnValue(buf, TP[axis], out_steps);
  __Log(LOG_INFO, "Number of steps for axis " + std::to_string(axis) + ": " + 
        std::to_string(*out_steps));
  return ret; 
}

bool AgilisPiezo::GetAxisStatus(const int axis, int* out_axis_status) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "GetAxisStatus: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  __Log(LOG_INFO, "Getting status for axis " + std::to_string(axis));
  const bool ret = __SendCommand(TS[axis]);
  __GetReturnValue(buf);
  __GetIntegerFromReturnValue(buf, TS[axis], out_axis_status);
  
  std::string status_str;
  switch (*out_axis_status) {
    case AXISSTATUS_READY: status_str = "Ready"; break;
    case AXISSTATUS_STEPPING: status_str = "Stepping"; break;
    case AXISSTATUS_JOGGING: status_str = "Jogging"; break;
    case AXISSTATUS_MOVINGTOLIMIT: status_str = "Moving to limit"; break;
    default: status_str = "Unknown"; break;
  }
  
  __Log(LOG_INFO, "Status for axis " + std::to_string(axis) + ": " + 
        std::to_string(*out_axis_status) + " (" + status_str + ")");
  return ret;
}

bool AgilisPiezo::GetControllerFirmwareVersion(std::string* out_version) const {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Getting controller firmware version");
  const bool ret = __SendCommand("VE");
  out_version->clear();
  __GetReturnValue(*out_version);
  int end = out_version->find("\r\n");
  if (end != std::string::npos)
    *out_version = out_version->substr(0, end);
  __Log(LOG_INFO, "Controller firmware version: " + *out_version);
  return ret; 
}

bool AgilisPiezo::ZeroPosition(const int axis) const {
  if (axis != 1 && axis != 2) {
    __Log(LOG_ERROR, "ZeroPosition: Invalid axis (must be 1 or 2)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Zeroing position for axis " + std::to_string(axis));
  return __SendCommand(ZP[axis]);
}

bool AgilisPiezo::ChangeChannel(const int channel) {
  if (channel < 0 || channel > 4) {
    __Log(LOG_ERROR, "ChangeChannel: Invalid channel (must be between 0 and 4)");
    return false;
  }
  
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Changing to channel " + std::to_string(channel));
  return __SendCommand("CC" + channel_table_[channel]);
}

bool AgilisPiezo::GetChannel(int* out_channel) {
  std::lock_guard<std::mutex> l(m_);
  std::string buf;
  __Log(LOG_INFO, "Getting current channel");
  const bool ret = __SendCommand("CC?");
  __GetReturnValue(buf);
  __GetIntegerFromReturnValue(buf, "CC", out_channel);
  __Log(LOG_INFO, "Current channel: " + std::to_string(*out_channel));
  return ret;
}

void AgilisPiezo::SetCommandTerm(const int64_t ms) {
  std::lock_guard<std::mutex> l(m_);
  __Log(LOG_INFO, "Setting command term to " + std::to_string(ms) + " ms");
  cmd_term_ = ms;
}

int64_t AgilisPiezo::GetCommandTerm() {
  std::lock_guard<std::mutex> l(m_);
  return cmd_term_;
}

void AgilisPiezo::SetLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> l(m_);
  log_level_ = level;
  __Log(LOG_INFO, "Log level set to " + std::to_string(level));
}

bool AgilisPiezo::__SendCommand(const std::string& command, int* error_code) const {
  const int remaintime = cmd_term_ - cmd_term_timer_.ElapsedMilli();
  if (remaintime > 0) {
    __Log(LOG_DEBUG, "Waiting " + std::to_string(remaintime) + " ms before sending command");
    std::this_thread::sleep_for(std::chrono::milliseconds(remaintime));
  }
  
  serial_->FlushSend();
  __Log(LOG_DEBUG, "Sending command: " + command);
  const size_t written_size = serial_->Send(command + "\r\n");
  cmd_term_timer_.Start();
  
  if (written_size != command.size() + 2) {
    __Log(LOG_ERROR, "Failed to send command: wrote " + 
          std::to_string(written_size) + " bytes, expected " + 
          std::to_string(command.size() + 2));
    return false;
  }
  
  return true;
}

inline bool AgilisPiezo::__GetReturnValue(std::string& buf, const int timeout_ms) const {
  __Log(LOG_DEBUG, "Waiting for response (timeout: " + std::to_string(timeout_ms) + " ms)");
  const bool ret = serial_->ListenUntil(&buf, "\r\n", timeout_ms);
  serial_->FlushListen();
  
  if (!ret) {
    __Log(LOG_ERROR, "Failed to get response (timeout)");
  } else {
    __Log(LOG_DEBUG, "Got response: " + buf);
  }
  
  return ret;
}

inline bool AgilisPiezo::__GetIntegerFromReturnValue(
  const std::string& buf, const std::string& command, int* out) const {
  const size_t begin = buf.find(command);
  if (begin == std::string::npos) {
    __Log(LOG_ERROR, "Failed to find command '" + command + "' in response");
    return false;
  }
  
  const size_t end = buf.find("\r\n");
  if (end == std::string::npos) {
    __Log(LOG_ERROR, "Failed to find end marker in response");
    return false;
  }
  
  std::string int_str = buf.substr(begin + command.size(), end - (begin + command.size()));
  try {
    *out = std::stoi(int_str);
    return true;
  } catch (const std::exception& err) {
    __Log(LOG_ERROR, "Failed to convert '" + int_str + "' to integer: " + err.what());
    return false;
  }
}

void AgilisPiezo::__Log(LogLevel level, const std::string& message) const {
  if (level >= log_level_) {
    std::string level_str;
    switch (level) {
      case LOG_DEBUG: level_str = "DEBUG"; break;
      case LOG_INFO: level_str = "INFO"; break;
      case LOG_WARNING: level_str = "WARNING"; break;
      case LOG_ERROR: level_str = "ERROR"; break;
      default: level_str = "UNKNOWN"; break;
    }
    
    std::ostringstream ss;
    ss << "[" << level_str << "] " << message;
    std::cout << ss.str() << std::endl;
  }
}

}