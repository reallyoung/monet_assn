#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

NS_LOG_COMPONENT_DEFINE("lab8");

using namespace ns3;

void ReceivePacket(Ptr<Socket> socket) {
//  NS_LOG_UNCOND("Received one packet!");
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
  int nodeNumber = 4;
  uint32_t packetNumber = 400;

  CommandLine cmd;
  cmd.AddValue("nodes", "Number of nodes", nodeNumber);
//  cmd.AddValue("packetNumber", "Number of packet", packetNumber);
  cmd.Parse(argc,argv);  


  Time interPacketInterval = Seconds(1.0);

  NodeContainer c;
  c.Create(nodeNumber);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default();
  wifiPhy.Set("RxGain", DoubleValue(0)); 
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel","Rss", DoubleValue(-80));
  wifiPhy.SetChannel(wifiChannel.Create());

  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode",StringValue(phyMode), "ControlMode",StringValue(phyMode));
  
  wifiMac.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);

  MobilityHelper mobility;
  int gw = 2;
  if( nodeNumber> 4)
	  gw =4;
  if(nodeNumber >10)
	  gw =5;
  if(nodeNumber>20)
	  gw =6;

  mobility.SetPositionAllocator("ns3::GridPositionAllocator", 
		  "MinX",DoubleValue(0.0), "MinY", DoubleValue(0.0),
		  "DeltaX", DoubleValue(10.0), "DeltaY", DoubleValue(10.0),
		  "GridWidth", UintegerValue(gw));
/*
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
*/
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  InternetStackHelper internet;
    internet.Install(c);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(devices);

	TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
	Ptr<Socket> mysocket[nodeNumber];
	InetSocketAddress local=InetSocketAddress(i.GetAddress(0), 80);
	InetSocketAddress remote=InetSocketAddress(i.GetAddress(1), 80);
	for(int nd=0;nd<nodeNumber;nd+=2)
	{
		mysocket[nd] = Socket::CreateSocket(c.Get(nd), tid);
		local = InetSocketAddress(i.GetAddress(nd), 80);
		mysocket[nd]->Bind(local);
		mysocket[nd]->SetRecvCallback(MakeCallback(&ReceivePacket));
		if(nd+1>=nodeNumber)
			break;
		mysocket[nd+1] = Socket::CreateSocket(c.Get(nd+1),tid);
		remote = InetSocketAddress(i.GetAddress(nd+1),80);
		mysocket[nd+1]->Bind(remote);
		mysocket[nd+1]->SetAllowBroadcast(true);
		mysocket[nd+1]->Connect(local);
		NS_LOG_UNCOND("Make Connection with "<<nd<<"&"<<nd+1);
	}

	wifiPhy.EnablePcap("lab8", devices);
	for(int nd=0;nd<nodeNumber;nd+=2){
		if(nd+1>=nodeNumber)        break;
		Simulator::ScheduleWithContext(mysocket[nd+1]->GetNode()->GetId(), Seconds(1.0), &GenerateTraffic, mysocket[nd+1], 1000, packetNumber, interPacketInterval);
		NS_LOG_UNCOND("Setup Sending node"<<nd+1);
	}



  AnimationInterface anim("Wireless-animation.xml");
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

