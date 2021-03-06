#include "ITransport.h"

void ITransport::enableLogging(
    bool enable)
{
  m_enableLogging = enable;
  if (m_enableLogging) {
    m_file.open("transport.log");
  } else {
    m_file.close();
  }
}

void ITransport::logPacket(
  const uint8_t* data,
  uint32_t length)
{
    m_file << "sendPacket: ";
    for (size_t i = 0; i < length; ++i) {
        m_file << std::hex << (int)data[i] << " ";
    }
    m_file << std::endl;
}

void ITransport::logAck(
  const Ack& ack)
{
    m_file << "received: ";
    for (size_t i = 0; i < ack.size; ++i) {
        m_file << std::hex << (int)ack.data[i] << " ";
    }
    m_file << std::endl;
}
