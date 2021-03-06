/*
 * SerialInterface.cpp
 *
 *  Created on: Apr 13, 2013
 *      Author: bholdaway
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <cstring>
#include "SerialInterface.hpp"

SerialInterface::SerialInterface(std::string port, UInt32 baudRate)
  : m_fd(0),
    m_port(port),
    m_baudRate(baudRate),
    m_isPortGood(false)
{
  //empty
}

SerialInterface::~SerialInterface()
{
  close(m_fd);
}


Int32 SerialInterface::recv(UInt8* pBuf, UInt32 size)
{
  size_t ret = -1;

  if (!m_isPortGood)
  {
    sleep(1);
    openInterface();
  }

  if (m_fd > 0)
  {
    fd_set rdfs;
    struct timeval timeout;

    FD_ZERO(&rdfs);
    FD_SET(m_fd, &rdfs);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int sel = 0;
    if ((sel = select(m_fd+1, &rdfs, NULL, NULL, &timeout)) > 0)
    {
      ret = read(m_fd, pBuf, 1);
    }

    if (ret < 0 || sel < 0)
    {
      m_isPortGood = false;
      close(m_fd);
      ret = -1;
      printf("Closing port\n");
    }
    else if (sel == 0)
    {
      printf("timeout\n");
    }
  }

  return ret;
}

Int32 SerialInterface::send(UInt8* pBuf, UInt32 size)
{
  size_t ret = -1;

  if (!m_isPortGood)
  {
    sleep(1);
    openInterface();
  }

  if (m_fd > 0)
  {
    fd_set rdfs;
    struct timeval timeout;

    FD_ZERO(&rdfs);
    FD_SET(m_fd, &rdfs);

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (select(m_fd+1, NULL, &rdfs, NULL, &timeout) > 0)
    {
      ret = write(m_fd, pBuf, size);

      if (ret < 0)
      {
        m_isPortGood = false;
        close(m_fd);
        printf("Closing port\n");
        ret = -1;
      }
    }
  }

  return ret;
}

void SerialInterface::configure()
{
  fcntl(m_fd, F_SETFL, FNDELAY);
  struct termios port_settings;      // structure to store the port settings in
  bzero(&port_settings, sizeof(port_settings));

  speed_t speed = convertSpeed(m_baudRate);

  cfsetispeed(&port_settings, speed);    // set baud rates
  cfsetospeed(&port_settings, speed);

  port_settings.c_cflag |= ( CLOCAL | CREAD |  CS8);                        // Configure the device : 8 bits, no parity, no control
  port_settings.c_iflag |= ( IGNPAR | IGNBRK );

  // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
  port_settings.c_cc[VMIN]  = 0;
  port_settings.c_cc[VTIME] = 0;

  if(tcsetattr(m_fd, TCSANOW, &port_settings) < 0)    // apply the settings to the port
  {
    printf("Error: Failed to set serial settings\n");
  }
}

bool SerialInterface::isGood()
{
  return m_isPortGood;
}

bool SerialInterface::openInterface()
{
  printf("trying to open Serial port\n");
  m_fd = open(m_port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

  if (m_fd != -1)
  {
    configure();
    m_isPortGood = true;
    printf("Serial port open\n");
  }
  else
  {
    m_fd = 0;
  }
}

speed_t SerialInterface::convertSpeed(UInt32 baudRate)
{
  speed_t speed;

  switch (baudRate)
  {
    case 57600:
    {
      speed = B57600;
    }
    break;

    case 230400:
    {
      speed = B230400;
    }
    break;

    case 460800:
    {
      speed = B460800;
    }
    break;

    case 500000:
    {
      speed = B500000;
    }
    break;

    case 576000:
    {
      speed = B576000;
    }
    break;

    case 921600:
    {
      speed = B921600;
    }
    break;

    case 1000000:
    {
      speed = B1000000;
    }
    break;

    case 1152000:
    {
      speed = B1152000;
    }
    break;

    case 1500000:
    {
      speed = B1500000;
    }
    break;

    case 2000000:
    {
      speed = B2000000;
    }
    break;

    case 2500000:
    {
      speed = B2500000;
    }
    break;

    case 3000000:
    {
      speed = B3000000;
    }
    break;

    case 3500000:
    {
      speed = B3500000;
    }
    break;

    case 4000000:
    {
      speed = B4000000;
    }
    break;

    case 115200:
    default:
    {
      speed = B115200;
    }
  }

  return speed;
}
