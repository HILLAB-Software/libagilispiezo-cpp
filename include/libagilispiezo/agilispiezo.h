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

#ifndef LIBAGILISPIEZO_AGILISPIEZO_H
#define LIBAGILISPIEZO_AGILISPIEZO_H

#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <future>
#include <iostream>
#include "libagilispiezo/serial.h"

namespace agilispiezo {

typedef std::chrono::steady_clock sclock;

class Timer {
public:
  virtual ~Timer() { }
  inline void Start() const { start_time_ = sclock::now(); }
  inline uint64_t ElapsedMilli() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
      sclock::now() - start_time_
    ).count();
  }
  inline uint64_t ElapsedSec() const {
    return std::chrono::duration_cast<std::chrono::seconds>(
      sclock::now() - start_time_
    ).count();
  }

private:
  mutable sclock::time_point start_time_;
};
  

class AgilisPiezo {
public:
  enum JogSpeed {
    JOGSPEED_0 = 0, // Stop
    JOGSPEED_5 = 1, // 5 steps/s at defined step amplitude
    JOGSPEED_100 = 2, // 100 steps/s at maximum step amplitude
    JOGSPEED_1700 = 3, // 1700 steps/s at maximum step amplitude
    JOGSPEED_666 = 4 // 666 steps/s at defined step amplitude
  };

  enum AxisStatus {
    AXISSTATUS_READY = 0,
    AXISSTATUS_STEPPING = 1,
    AXISSTATUS_JOGGING = 2,
    AXISSTATUS_MOVINGTOLIMIT = 3
  };

  enum ErrorCode {
    ERRORCODE_NOERROR = 0,
    ERRORCODE_UNKNOWN_COMMAND = -1,
    ERRORCODE_AXIS_OUT_OF_RANGE = -2,
    ERRORCODE_WRONG_FORMAT_FOR_PARAMETER = -3,
    ERRORCODE_PARAMETER_OUT_OF_RANGE = -4,
    ERRORCODE_NOT_ALLOWED_IN_LOCAL_MODE = -5,
    ERRORCODE_NOT_ALLOWED_IN_CURRENT_STATE = -6
  };

  enum LogLevel {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3,
    LOG_NONE = 4
  };

public:
  AgilisPiezo();
  ~AgilisPiezo();
  bool ConnectDeviceUSB(const std::string& port_name);
  bool ConnectDeviceRS232(const std::string& port_name);
  void DisconnectDevice();
  bool IsConnected() const;
  std::string GetErrorText(const int e) const;
  std::string GetPortName() const;

  /**
   * @brief Command-"DL"
   * Sets the step delay of stepping mode.
   * The delay applies for both positive and negative directions.
   * The delay is programmed as multiple of 10µs.
   * For example, a delay of 40 is equivalent to 40 x 10 µs = 400 µs.
   * The maximum value of the parameter is equal to a delay of 2 seconds
   * between pulses. By default, after reset, the value is 0.
   *
   * @param axis
   * @param delay
  */
  bool SetStepDelay(const int axis, const int delay) const;

  /**
   * @brief Command-"DL"
   * Returns stpe delay.
   *
   * @param axis
   * @param out_delay
  */
  bool GetStepDelay(const int axis, int* out_delay) const;

  /**
   * @brief Command-"JA"
   * Starts a jog motion at a defined speed.
   * Defined steps are steps with step amplitude defined by the
   * SU command (default 16). Max. amplitude steps are
   * equivalent to step amplitude 50:
   *
   * @param axis
   * @param sign
   * @param jog_speed
  */
  bool StartJogMotion(
    const int axis, const bool sign, const int jog_speed) const;

  /**
   * @brief Command-"JA"
   * Returns jog mode.
   *
   * @param axis
   * @param out_sign
   * @param out_jog_speed
  */
  bool GetJogMode(const int axis, bool* out_sign, int* out_jog_speed) const;

  /**
   * @brief Command-"MA"
   * Starts a process to measure the current position.
   * During the execution of the command, the USB communication is interrupted.
   * After completion, the communication is opened again.
   * The execution of the command can last up to 2 minutes.
   *
   * @param axis
   *
   * @param out_value
   * The distance of the current position to the limit in 1/1000th of the total travel.
  */
  bool MeasureCurrentPosition(
    const int axis, std::future<int>* out_position) const;

  /**
   * @brief Command-"ML"
   * Sets the controller to local mode.
   * In local mode the pushbuttons on the controller are enabled
   * and all commands that configure or operate the controller are disabled.
   * To go to remote mode, use the MR command.
   * At power-up the controller is always in local mode.
  */
  bool SetToLocalMode() const;

  /**
   * @brief Command-"MR"
   * Sets the controller to remote mode.
   * In remote mode all commands are enabled and the
   * pushbuttons on the controller are disabled.
  */
  bool SetToRemoteMode() const;

  /**
   * @brief Command-"MV"
   * Starts a jog motion at a defined speed to the limit
   * and stops automatically when the limit is activated.
  */
  bool MoveToLimit(
    const int axis, const bool sign, const int jog_speed = JOGSPEED_1700) const;

  /**
   * @brief Command-"PA"
   * Starts a process to move to an absolute position.
   * The execution of the command can last up to 2 minutes.
  */
  bool AbsoluteMove(const int axis, const int position) const;

  /**
   * @brief Command-"PH"
   * Returns the limits axis status of the controller.
  */
  bool TellLimitStatus(bool* out_axis1, bool* out_axis2) const;

  /**
   * @brief Command-"PR"
   * Starts a relative move of nn steps with step amplitude
   * defined by the SU command (default 16).
  */
  bool RelativeMove(const int axis, const bool sign, const int steps) const;

  /**
   * @brief Command-"RS"
   * Resets the controller. All temporary settings are reset
   * to default and the controller is in local mode.
  */
  bool ResetController() const;

  /**
   * @brief Command-"ST"
   * Stops the motion on the defined axis. Sets the state to ready.
  */
  bool StopMotion(const int axis) const;

  /**
   * @brief Command-"SU"
   * Sets the step amplitude (step size) in positive or negative direction.
   * If the sign is positive, it will set the step amplitude
   * in the forward direction. If the sign is negative, it will
   * set the step amplitude in the backward direction.
   *
   * @param axis
   *
   * @param sign
   *
   * @param amplitude
   * Integer between -50 and 50 included, except zero.
  */
  bool SetStepAmplitude(
    const int axis, const bool sign, const int amplitude) const;

  /**
   * @brief Command-"SU"
   * If sign is positive, this returns the step amplitude in forward direction.
   * Otherwise, this returns the step amplitude setting in backward direction.
  */
  bool GetStepAmplitudeSetting(
    const int axis, const bool sign, int* out_amplitude) const;

  /**
   * @brief Command-"TE"
   * Returns the error code of the previous command.
   * Each command generates an error code including "0" for NO ERROR.
   * This error code can be queried with the TE command.
   * For a safe program flow it is recommended to always query the command
   * error after each command execution.
   * The following error codes are defined:
   * 0 No error
   * -1 Unknown command
   * -2 Axis out of range (must be 1 or 2, or must not be specified)
   * -3 Wrong format for parameter nn (or must not be specified)
   * -4 Parameter nn out of range
   * -5 Not allowed in local mode
   * -6 Not allowed in current state
   *
  */
  bool GetErrorOfPreviousCommand(int* out_error_code) const;

  /**
   * @brief Command-"TP"
   * Returns the number of accumulated steps in forward direction
   * minus the number of steps in backward direction since powering
   * the controller or since the last ZP (zero position) command,
   * whatever was last.
  */
  bool TellNumberOfSteps(const int axis, int* out_steps) const;

  /**
   * @brief Command-"TS"
   * Returns the status of the axis.
   * 0 Ready (not moving)
   * 1 Stepping (currently executing a PR command)
   * 2 Jogging (currently executing a JA command with command
   *            parameter different than 0).
   * 3 Moving to limit (currently executing MV, MA, PA commands)
  */
  bool GetAxisStatus(const int axis, int* out_axis_status) const;

  /**
   * @brief Command-"VE"
   * Returns the firmware version of the controller.
  */
  bool GetControllerFirmwareVersion(std::string* out_version) const;

  /**
   * @brief Command-"ZP"
   * Resets the step counter to zero.
   * @param axis 
  */
  bool ZeroPosition(const int axis) const;

  /**
   * @brief Command-"CC"
   * This command is specific to AG-UC8.
   * The piezo actuators are selected by pairs which
   * are grouped in four channels. This command changes the selected channel.

    * @param value
  */
  bool ChangeChannel(const int channel);


  bool GetChannel(int* out_channel);

  void SetCommandTerm(const int64_t ms);

  int64_t GetCommandTerm();

  /**
   * @brief Set the log level for the library
   * @param level Log level (LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_NONE)
   */
  void SetLogLevel(LogLevel level);

private:
  /// Send command. If error_code is specified, read return values from serial.
  bool __SendCommand(const std::string& command, int* error_code = nullptr) const;
  bool __GetReturnValue(std::string& buf, const int timeout_ms = 3000) const;
  bool __GetIntegerFromReturnValue(
    const std::string& buf, const std::string& command, int* out) const;
  void __Log(LogLevel level, const std::string& message) const;

  // Communications
private:
  std::string last_port_name_;
  std::unique_ptr<Serial> serial_;
  int64_t cmd_term_ = 50;
  Timer cmd_term_timer_;
  mutable std::mutex m_;
  LogLevel log_level_ = LOG_WARNING;

  std::string axis_table_[3] = { "0", "1", "2" };
  std::string channel_table_[5] = { "0", "1", "2", "3", "4" };
  std::string DL[3] = { "0DL", "1DL", "2DL" };
  std::string JA[3] = { "0JA", "1JA", "2JA" };
  std::string MA[3] = { "0MA", "1MA", "2MA" };
  std::string MV[3] = { "0MV", "1MV", "2MV" };
  std::string PA[3] = { "0PA", "1PA", "2PA" };
  std::string PH = "PH";
  std::string PR[3] = { "0PR", "1PR", "2PR" };
  std::string ST[3] = { "0ST", "1ST", "2ST" };
  std::string SU[3] = { "0SU", "1SU", "2SU" };
  std::string TP[3] = { "0TP", "1TP", "2TP" };
  std::string TS[3] = { "0TS", "1TS", "2TS" };
  std::string ZP[3] = { "0ZP", "1ZP", "2ZP" };
};

}

#endif