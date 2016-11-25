#include "ns3/core-module.h"

NS_LOG_COMPONENT_DEFINE("HeloNS3");

using namespace ns3;

int main (int argc, char *argv[])
{


	NS_LOG_UNCOND("Hello NS-3");
	Simulator::Run();
	Simulator::Destroy();
}
