#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
//#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Lab9");
int direction;
void ChangePosition(Ptr<Node> node) {
	Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> (); 
	Vector pos = mobility->GetPosition();
	if(pos.x <=198.0&& direction)
		pos.x += 2.0;
	else
	{
		direction = 0;
		pos.x -= 2.0;
	}
	mobility->SetPosition(pos);
	Simulator::Schedule(Seconds(1.0),&ChangePosition, node);
}

int main (int argc, char *argv[])
{
//	LogComponentEnableAll(LOG_ALL);
	int isTCP = 0;
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",StringValue ("2200"));
	direction =1;
	NodeContainer nodes, apNode, staNode;
	apNode.Create(1);
	staNode.Create(2);
	nodes.Add(apNode);//nodes0 = apNode0
	nodes.Add(staNode);//nodes1 = staNode0
	
	//PHY layer
	WifiHelper wifi;// = WifiHelper::Default();
	wifi.SetRemoteStationManager("ns3::ArfWifiManager");//rate apaptation
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
	wifiPhy.SetChannel(wifiChannel.Create());
	
	//MAC layer for AP 
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
	Ssid ssid = Ssid("wifi");
	
	wifiMac.SetType("ns3::ApWifiMac","Ssid", SsidValue(ssid));
	NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, apNode.Get(0));
	NetDeviceContainer devices = apDevice;

	//MAC layer for mobile station
	wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(true));
	NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, staNode.Get(0));
	devices.Add(staDevice);

	//set initial position of nodes
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
	positionAlloc->Add(Vector(100.0,100.0,0.0));// AP 
	positionAlloc->Add(Vector(100.0,50.0,0.0)); // mobile station1
	positionAlloc->Add(Vector(100.0,100.0,0.0));// mobile station2
	mobility.SetPositionAllocator(positionAlloc);

	//mobility moder for wifi nodes
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(apNode.Get(0));
			
	mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel", 
			"Bounds", RectangleValue(Rectangle(0, 200, 0 ,200)),
			"Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]"), "Distance", DoubleValue(30));
	mobility.Install(staNode.Get(0));

	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(staNode.Get(1));
	//Set Internet stack
	InternetStackHelper internet;
	internet.Install(nodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0","255.255.255.0"); //class C is used here
	Ipv4InterfaceContainer i = ipv4.Assign(devices);

	wifiPhy.EnablePcap("lab09",devices);

	//Install On Off application with UDP protocol on AP
	uint16_t port = 6000;
	OnOffHelper onOff("ns3::UdpSocketFactory", 
			Address(InetSocketAddress(i.GetAddress(0),port)));
	onOff.SetAttribute("PacketSize", UintegerValue(1024));
	onOff.SetAttribute("DataRate", StringValue("3000000bps")); //3Mbps
	ApplicationContainer apps = onOff.Install(staNode.Get(0));

	//Install PacketSink app on the mobile staion
	PacketSinkHelper sink("ns3::UdpSocketFactory",
			InetSocketAddress(Ipv4Address::GetAny(), port));
	apps.Add(sink.Install(apNode.Get(0)));

	apps.Start(Seconds(1.0)); // onoff and packetsink start at 1.0
	apps.Stop(Seconds(100.0)); 
	Simulator::Schedule(Seconds(1.0), &ChangePosition, staNode.Get(1));	
	//for netanim
	AnimationInterface anim("test.xml");

	Simulator::Stop(Seconds(100.0));
	Simulator::Run();
	Simulator::Destroy();
			
}
