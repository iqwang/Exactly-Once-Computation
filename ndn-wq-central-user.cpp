/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndn-wq-central-user.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include <time.h>
#include <stdio.h>
#include <fstream>
#include "helper/ndn-fib-helper.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.WqCentralUser");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(WqCentralUser);

TypeId
WqCentralUser::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::WqCentralUser")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<WqCentralUser>()

      .AddAttribute("Frequency", "Frequency of interest packets", StringValue("5.0"),
                    MakeDoubleAccessor(&WqCentralUser::m_frequency), MakeDoubleChecker<double>())

      .AddAttribute("Randomize",
                    "Type of send time randomization: none (default), uniform, exponential",
                    StringValue("none"),
                    MakeStringAccessor(&WqCentralUser::SetRandomize, &WqCentralUser::GetRandomize),
                    MakeStringChecker())

      .AddAttribute("MaxSeq", "Maximum sequence number to request",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&WqCentralUser::m_seqMax), MakeIntegerChecker<uint32_t>());

  return tid;
}

WqCentralUser::WqCentralUser()
  : m_frequency(1.0)
  , m_firstTime(true)

{
  NS_LOG_FUNCTION_NOARGS();
  m_seqMax = std::numeric_limits<uint32_t>::max();

  m_taskPrefix1 = "/";
  m_taskPrefix2 = "-";
}


void
WqCentralUser::SendPacket()
{
	if (!m_active)
	return;
  
	NS_LOG_FUNCTION_NOARGS();
    
  std::vector<std::string> sensorNames = {"m1","m2","m3","m4","m5"};  
    
  for(uint8_t i=0; i<sensorNames.size(); i++)
  {
    std::string taskString = m_taskPrefix1 + sensorNames[i] + m_taskPrefix2;
    std::cout << "Assign task: " << taskString << std::endl;
    shared_ptr<Name> taskName = make_shared<Name>(taskString);
    taskName->appendSequenceNumber(m_rand->GetValue(0, std::numeric_limits<uint16_t>::max()));
    shared_ptr<Interest> taskInterest = make_shared<Interest>();
    taskInterest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    taskInterest->setName(*taskName);
    taskInterest->setInterestLifetime(time::milliseconds(m_interestLifeTime.GetMilliSeconds()));
    m_transmittedInterests(taskInterest, this, m_face);
    m_appLink->onReceiveInterest(*taskInterest);
    ScheduleNextPacket();
  }
}

void
WqCentralUser::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &WqCentralUser::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &WqCentralUser::SendPacket, this);
  // printLater.join();
}

void
WqCentralUser::SetRandomize(const std::string& value)
{
  if (value == "uniform") {
    m_random = CreateObject<UniformRandomVariable>();
    m_random->SetAttribute("Min", DoubleValue(0.0));
    m_random->SetAttribute("Max", DoubleValue(2 * 1.0 / m_frequency));
  }
  else if (value == "exponential") {
    m_random = CreateObject<ExponentialRandomVariable>();
    m_random->SetAttribute("Mean", DoubleValue(1.0 / m_frequency));
    m_random->SetAttribute("Bound", DoubleValue(50 * 1.0 / m_frequency));
  }
  else
    m_random = 0;

  m_randomType = value;
}

std::string
WqCentralUser::GetRandomize() const
{
  return m_randomType;
}


void
WqCentralUser::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

  //parse data content
    auto *tmpContent = ((uint8_t*)data->getContent().value());
    std::string receivedData;
    for(uint8_t i=0; i < data->getContent().value_size(); i++) {
      receivedData.push_back((char)tmpContent[i]);
    };
    std::cout << "User Receive Data: " << receivedData << " from " << data->getName().toUri() << std::endl;
}

void
WqCentralUser::OnInterest(shared_ptr<const Interest> interest)
{

  App::OnInterest(interest); // tracing inside
  NS_LOG_FUNCTION(this << interest);
  if (!m_active) {return;}
}

} // namespace ndn
} // namespace ns3
