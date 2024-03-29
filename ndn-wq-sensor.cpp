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

// #include "/usr/include/python2.7/Python.h"
#include "ndn-wq-sensor.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include <deque>
//////////////////

#include<unistd.h>
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>

/////////////////





NS_LOG_COMPONENT_DEFINE("ndn.WqSensor");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(WqSensor);

TypeId
WqSensor::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::WqSensor")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<WqSensor>()
      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&WqSensor::m_prefix), MakeNameChecker())
      .AddAttribute("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                    StringValue("/"), MakeNameAccessor(&WqSensor::m_postfix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&WqSensor::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&WqSensor::m_freshness),
                    MakeTimeChecker())
      .AddAttribute("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                    UintegerValue(0), MakeUintegerAccessor(&WqSensor::m_signature),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&WqSensor::m_keyLocator), MakeNameChecker());
  return tid;
}

WqSensor::WqSensor()
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
WqSensor::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
WqSensor::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
WqSensor::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());

  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  int ranNum = rand()%100 +1;
	std::string rawData = std::to_string(ranNum);  
	//std::cout<< "Raw data: "<< rawData << std::endl;
	uint8_t* p = (uint8_t*) &(*rawData.begin());
  auto buffer = make_shared< ::ndn::Buffer>(p, rawData.size());
  data->setContent(buffer);

  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }
  data->setSignatureInfo(signatureInfo);
  ::ndn::EncodingEstimator estimator;
  ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
  encoder.appendVarNumber(m_signature);
  data->setSignatureValue(encoder.getBuffer());

  // to create real wire encoding
  data->wireEncode();
  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
  std::cout << m_prefix.toUri() << " return: " << rawData << std::endl;
}

} // namespace ndn
} // namespace ns3
