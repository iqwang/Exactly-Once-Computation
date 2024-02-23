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
 *
 *   /------\ 0                                                 0 /------\
 *   |  c1  |<-----+                                       +----->|  p1  |
 *   \------/       \                                     /       \------/
 *                   \              /-----\              /
 *   /------\ 0       \         +==>| r12 |<==+         /       0 /------\
 *   |  c2  |<--+      \       /    \-----/    \       /      +-->|  p2  |
 *   \------/    \      \     |                 |     /      /    \------/
 *                \      |    |   1Mbps links   |    |      /
 *                 \  1  v0   v5               1v   2v  3  /
 *                  +->/------\                 /------\<-+
 *                    2|  r1  |<===============>|  r2  |4
 *                  +->\------/4               0\------/<-+
 *                 /    3^                           ^5    \
 *                /      |                           |      \
 *   /------\ 0  /      /                             \      \  0 /------\
 *   |  c3  |<--+      /                               \      +-->|  p3  |
 *   \------/         /                                 \         \------/
 *                   /     "All consumer-router and"     \
 *   /------\ 0     /      "router-producer links are"    \    0 /------\
 *   |  c4  |<-----+       "10Mbps"                        +---->|  p4  |
 *   \------/                                                    \------/
 *
 *   "Numbers near nodes denote face IDs. Face ID is assigned based on the order of link"
 *   "definitions in the topology file"
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-congestion-alt-topo-plugin
 */

int
main(int argc, char* argv[])
{
  CommandLine cmd;
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/wq-compute-once-britetopo.txt");
  topologyReader.Read();

   // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();
  
  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndn::StrategyChoiceHelper::InstallAll("prefix", "/localhost/nfd/strategy/bestroute");
  

  //100 nodes in total
  Ptr<Node> mappers[69]={ 
                          Names::Find<Node>("31"), Names::Find<Node>("32"), Names::Find<Node>("33"), Names::Find<Node>("34"), Names::Find<Node>("35"),
                          Names::Find<Node>("36"), Names::Find<Node>("37"), Names::Find<Node>("38"), Names::Find<Node>("39"), Names::Find<Node>("40"),
                          Names::Find<Node>("41"), Names::Find<Node>("42"), Names::Find<Node>("43"), Names::Find<Node>("44"), Names::Find<Node>("45"),
                          Names::Find<Node>("46"), Names::Find<Node>("47"), Names::Find<Node>("48"), Names::Find<Node>("49"),
                          Names::Find<Node>("50"), Names::Find<Node>("51"), Names::Find<Node>("52"), Names::Find<Node>("53"), Names::Find<Node>("54"),
                          Names::Find<Node>("55"), Names::Find<Node>("56"), Names::Find<Node>("57"), Names::Find<Node>("58"), Names::Find<Node>("59"),
                          Names::Find<Node>("60"), Names::Find<Node>("61"), Names::Find<Node>("62"), Names::Find<Node>("63"), Names::Find<Node>("64"),
                          Names::Find<Node>("65"), Names::Find<Node>("66"), Names::Find<Node>("67"), Names::Find<Node>("68"), Names::Find<Node>("69"), 
                          Names::Find<Node>("70"), Names::Find<Node>("71"), Names::Find<Node>("72"), Names::Find<Node>("73"), Names::Find<Node>("74"),
                          Names::Find<Node>("75"), Names::Find<Node>("76"), Names::Find<Node>("77"), Names::Find<Node>("78"), Names::Find<Node>("79"),
                          Names::Find<Node>("80"), Names::Find<Node>("81"), Names::Find<Node>("82"), Names::Find<Node>("83"), Names::Find<Node>("84"), 
                          Names::Find<Node>("85"), Names::Find<Node>("86"), Names::Find<Node>("87"), Names::Find<Node>("88"), Names::Find<Node>("89"),
                          Names::Find<Node>("90"), Names::Find<Node>("91"), Names::Find<Node>("92"), Names::Find<Node>("93"), Names::Find<Node>("94"),
                          Names::Find<Node>("95"), Names::Find<Node>("96"), Names::Find<Node>("97"), Names::Find<Node>("98"), Names::Find<Node>("99"),
                          };                        
                            
    Ptr<Node> reducers[30]={
                            Names::Find<Node>("1"), Names::Find<Node>("2"), Names::Find<Node>("3"), Names::Find<Node>("4"), Names::Find<Node>("5"),
                            Names::Find<Node>("6"), Names::Find<Node>("7"), Names::Find<Node>("8"), Names::Find<Node>("9"), Names::Find<Node>("10"),
                            Names::Find<Node>("11"), Names::Find<Node>("12"), Names::Find<Node>("13"), Names::Find<Node>("14"), Names::Find<Node>("15"), 
                            Names::Find<Node>("16"), Names::Find<Node>("17"), Names::Find<Node>("18"), Names::Find<Node>("19"), Names::Find<Node>("20"), 
                            Names::Find<Node>("21"), Names::Find<Node>("22"), Names::Find<Node>("23"), Names::Find<Node>("24"), Names::Find<Node>("25"),
                            Names::Find<Node>("26"), Names::Find<Node>("27"), Names::Find<Node>("28"), Names::Find<Node>("29"), Names::Find<Node>("30"),
                                                         
                            };
                            
   Ptr<Node> consumer = Names::Find<Node>("0");                       
    

  
  for (int i =0; i< 69; i++) 
  {
    std::string prefix = "/" + Names::FindName(mappers[i]) + "-";
    //std::cout << " producer prefix: " << prefix << std::endl;
    ndn::AppHelper producerHelper("ns3::ndn::WqMapper");
    // ndn::AppHelper producerHelper("ns3::ndn::WqSensor");    
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.SetPrefix(prefix);
    ApplicationContainer mapperNodes = producerHelper.Install(mappers[i]);
    ndnGlobalRoutingHelper.AddOrigins(prefix, mappers[i]);
  
  }
  
  for (int m =0; m< 30; m++) 
  {
    std::string prefix = "/" + Names::FindName(reducers[m]) + "-";
    //std::cout << " compute prefix: " << prefix << std::endl;
    ndn::AppHelper computeNodeHelper("ns3::ndn::WqReducer");    
    computeNodeHelper.SetAttribute("PayloadSize", StringValue("1024"));
    computeNodeHelper.SetPrefix(prefix);
    ApplicationContainer reducerNodes = computeNodeHelper.Install(reducers[m]);
    ndnGlobalRoutingHelper.AddOrigins(prefix, reducers[m]);   
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

  // Schedule simulation time and run the simulation
  Simulator::Stop(Seconds(20.0));
  ndn::L3RateTracer::InstallAll("wq-trace.txt", Seconds(1));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}

