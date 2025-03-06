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
#include <iostream>
#include <thread>
#include <chrono>

using namespace agilispiezo;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <device-port>" << std::endl;
    std::cout << "Example: " << argv[0] << " /dev/ttyUSB0" << std::endl;
    return 1;
  }
  std::string port_name = argv[1];
  AgilisPiezo piezo;
  piezo.SetLogLevel(AgilisPiezo::LOG_INFO);
  std::cout << "Connecting to device on " << port_name << "..." << std::endl;
  bool connected = piezo.ConnectDeviceUSB(port_name);
  
  if (!connected) {
    std::cout << "Failed to connect to device. Trying RS232 connection..." << std::endl;
    connected = piezo.ConnectDeviceRS232(port_name);
    
    if (!connected) {
      std::cout << "Failed to connect to device." << std::endl;
      return 1;
    }
  }
  
  std::cout << "Successfully connected to device." << std::endl;
  std::string firmware;
  if (piezo.GetControllerFirmwareVersion(&firmware)) {
    std::cout << "Firmware Version: " << firmware << std::endl;
  }
  if (!piezo.SetToRemoteMode()) {
    std::cout << "Failed to set remote mode." << std::endl;
    piezo.DisconnectDevice();
    return 1;
  }

  int axis1_status, axis2_status;
  piezo.GetAxisStatus(1, &axis1_status);
  piezo.GetAxisStatus(2, &axis2_status);
  std::cout << "Axis 1 Status: " << axis1_status << std::endl;
  std::cout << "Axis 2 Status: " << axis2_status << std::endl;

  int axis1_pos, axis2_pos;
  piezo.TellNumberOfSteps(1, &axis1_pos);
  piezo.TellNumberOfSteps(2, &axis2_pos);
  std::cout << "Axis 1 Position: " << axis1_pos << " steps" << std::endl;
  std::cout << "Axis 2 Position: " << axis2_pos << " steps" << std::endl;
  
  std::cout << "Moving axis 1 by 10 steps..." << std::endl;
  piezo.RelativeMove(1, true, 10);
  
  do {
    piezo.GetAxisStatus(1, &axis1_status);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while (axis1_status != AgilisPiezo::AXISSTATUS_READY);
  
  piezo.TellNumberOfSteps(1, &axis1_pos);
  std::cout << "Axis 1 new position: " << axis1_pos << " steps" << std::endl;
  
  std::cout << "Moving back to original position..." << std::endl;
  piezo.RelativeMove(1, false, 10);
  
  do {
    piezo.GetAxisStatus(1, &axis1_status);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while (axis1_status != AgilisPiezo::AXISSTATUS_READY);
  
  piezo.TellNumberOfSteps(1, &axis1_pos);
  std::cout << "Axis 1 final position: " << axis1_pos << " steps" << std::endl;
  
  piezo.DisconnectDevice();
  std::cout << "Device disconnected." << std::endl;
  
  return 0;
}