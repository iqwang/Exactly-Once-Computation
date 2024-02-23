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


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

namespace ns3 {

/**
 * This scenario simulates a grid topology (using topology reader module)
 *
 *   /------\                                                    /------\
 *   | Src1 |<--+                                            +-->| Dst1 |
 *   \------/    \                                          /    \------/
 *                \                                        /
 *                 +-->/------\   "bottleneck"  /------\<-+
 *                     | Rtr1 |<===============>| Rtr2 |
 *                 +-->\------/                 \------/<-+
 *                /                                        \
 *   /------\    /                                          \    /------\
 *   | Src2 |<--+                                            +-->| Dst2 |
 *   \------/                                                    \------/
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-congestion-topo-plugin
 */

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/wq-compute-once-topo4.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

	// Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndn::StrategyChoiceHelper::InstallAll("prefix", "/localhost/nfd/strategy/bestroute");

  // Ptr<Node> mappers[7] = {  
  //                           Names::Find<Node>("m1"), Names::Find<Node>("m2"), Names::Find<Node>("m3"), Names::Find<Node>("m4"),
  //                           Names::Find<Node>("m5"), Names::Find<Node>("m6"), Names::Find<Node>("m7"),
  //                         };

  // Ptr<Node> reducers[6] = {
  //                           Names::Find<Node>("1"), Names::Find<Node>("2"), Names::Find<Node>("3"),
  //                           Names::Find<Node>("4"), Names::Find<Node>("5"), Names::Find<Node>("6"),
  //                         };

  Ptr<Node> mappers[3] = {  
                            Names::Find<Node>("m1"), Names::Find<Node>("m2"), Names::Find<Node>("m3")
                          };
  Ptr<Node> reducers[2] = {
                          Names::Find<Node>("1"), Names::Find<Node>("2")
                        };
  Ptr<Node> consumer = Names::Find<Node>("0");

  for (int i=0; i<3; i++) 
  {
    std::string prefix = "/" + Names::FindName(mappers[i]) + "-";
    // std::cout << " producer prefix: " << prefix << std::endl;
    
    ndn::AppHelper producerHelper("ns3::ndn::WqMapper");
    // ndn::AppHelper producerHelper("ns3::ndn::WqSensor");
    
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.SetPrefix(prefix);
    ApplicationContainer mapperNodes = producerHelper.Install(mappers[i]);
    ndnGlobalRoutingHelper.AddOrigins(prefix, mappers[i]);
  }

  for (int j=0; j<2; j++) 
  {
    std::string prefix = "/" + Names::FindName(reducers[j]) + "-";
    // std::cout << " reducer prefix: " << prefix << std::endl;
    
    ndn::AppHelper computeNodeHelper("ns3::ndn::WqReducer");    
    computeNodeHelper.SetAttribute("PayloadSize", StringValue("1024"));
    computeNodeHelper.SetPrefix(prefix);
    ApplicationContainer reducerNodes = computeNodeHelper.Install(reducers[j]);
    ndnGlobalRoutingHelper.AddOrigins(prefix, reducers[j]);
  } 

  // ndn::AppHelper consumerHelper("ns3::ndn::WqCentralUser");
  ndn::AppHelper consumerHelper("ns3::ndn::WqMrUser");
  consumerHelper.SetAttribute("Frequency", StringValue("1")); // 5 interests a second
  // std::string prefix = "/" + Names::FindName(0) + "-";
  consumerHelper.SetPrefix("/0-");
  consumerHelper.Install(consumer);
  ndnGlobalRoutingHelper.AddOrigins("/0-", consumer);  

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  // Simulator::Schedule(Seconds(2), ndn::LinkControlHelper::FailLinkByName, "/2-", "/m3-");
  // Simulator::Schedule(Seconds(3), ndn::LinkControlHelper::UpLinkByName, "/2-", "/m3-");

  Simulator::Stop(Seconds(20.0));

  // ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
  ndn::L3RateTracer::InstallAll("wq-trace.txt", Seconds(1));
  // ndn::L3RateTracer::Install(Names::Find<Node>("0"), "user-trace.txt", Seconds(0.5));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  // NS_LOG_UNCOND ("Hello Simulator");
  return ns3::main(argc, argv);
}
