#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h" // include netanim header

NS_LOG_COMPONENT_DEFINE("labExample");

using namespace ns3;

static int currentNbPacketReceived;

void ReceivePacket(Ptr<Socket> socket) {
  //NS_LOG_UNCOND("Received one packet!");
  NS_LOG_UNCOND("Current number of packet received: " << ++currentNbPacketReceived);
}

static void GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval) {
  if(pktCount > 0) {
      socket->Send(Create<Packet>(pktSize));
      Simulator::Schedule(pktInterval, &GenerateTraffic, socket, pktSize,pktCount-1, pktInterval);
    }
  else
      socket->Close();
}


int main(int argc, char *argv[]) {
  std::string phyMode("DsssRate1Mbps");
  uint32_t nodeNumber = 2;
  uint32_t packetNumber = 40;

  CommandLine cmd;
  cmd.AddValue("nodes", "# of nodes", nodeNumber);
  cmd.AddValue("packets", "# of packets", packetNumber);
  cmd.Parse(argc, argv);

  Time interPacketInterval = Seconds(1.0);

  NodeContainer c;
  c.Create(nodeNumber);
  NS_LOG_UNCOND("Nodes created!");

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default();
  wifiPhy.Set("RxGain", DoubleValue(0)); 
  //wifiPhy.Set("TxPowerLevels", UintegerValue(1));
  //wifiPhy.Set("TxGain", DoubleValue(-10));
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel","Rss", DoubleValue(-80));
  //wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel");
  wifiPhy.SetChannel(wifiChannel.Create());
  NS_LOG_UNCOND("Wifi PHY & channel configured!");

  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode",StringValue(phyMode), "ControlMode",StringValue(phyMode));  
  
  wifiMac.SetType("ns3::AdhocWifiMac");
  
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);
  NS_LOG_UNCOND("Wifi MAC configured!");

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));   
  positionAlloc->Add(Vector(5.0, 5.0, 0.0));
  positionAlloc->Add(Vector(10.0, 10.0, 0.0));
  positionAlloc->Add(Vector(15.0, 15.0, 0.0));
 
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);
  NS_LOG_UNCOND("Mobility configured!");

  InternetStackHelper internet;
  internet.Install(c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);
  NS_LOG_UNCOND("IP configured!");

  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");

  // transmission from (5, 5) to (0, 0)
  Ptr<Socket> recvSink1 = Socket::CreateSocket(c.Get(0), tid);
  InetSocketAddress local1 = InetSocketAddress(Ipv4Address::GetAny(), 80);
  recvSink1->Bind(local1);
  recvSink1->SetRecvCallback(MakeCallback(&ReceivePacket));

  Ptr<Socket> source1 = Socket::CreateSocket(c.Get(1), tid);
  InetSocketAddress remote1 = InetSocketAddress(Ipv4Address("255.255.255.255"), 80);
  source1->SetAllowBroadcast(true);
  source1->Connect(remote1);
  NS_LOG_UNCOND("Connection established!");

  // transmission from (15, 15) to (10, 10)  
  Ptr<Socket> recvSink2 = Socket::CreateSocket(c.Get(2), tid);
  InetSocketAddress local2 = InetSocketAddress(Ipv4Address::GetAny(), 81);
  recvSink2->Bind(local2);
  recvSink2->SetRecvCallback(MakeCallback(&ReceivePacket));

  Ptr<Socket> source2 = Socket::CreateSocket(c.Get(3), tid);
  InetSocketAddress remote2 = InetSocketAddress(Ipv4Address("255.255.255.255"), 81);
  source2->SetAllowBroadcast(true);
  source2->Connect(remote2);
  NS_LOG_UNCOND("Connection established!");  

  wifiPhy.EnablePcap("labExample", devices);

  Simulator::ScheduleWithContext(source1->GetNode()->GetId(), Seconds(0), &GenerateTraffic, 
                                 source1, 1000, packetNumber, interPacketInterval);

  
  Simulator::ScheduleWithContext(source2->GetNode()->GetId(), Seconds(1.0), &GenerateTraffic, 
                                 source2, 1000, packetNumber, interPacketInterval);
  
  AnimationInterface anim("wireless-animation.xml");

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
