//#include "Server.h"
#include "AsyncServer.h"
#include <boost/asio.hpp>
#include <ctime>

using boost::asio::ip::udp;


std::string make_daytime_string()
{
	using namespace std; // For time_t, time and ctime;
	time_t now = time(0);
	return ctime(&now);
}

struct header
{
	int componentType;
	int sizeInBytes;
	char* data;
};

struct test
{
	int banan;
	float temperatur;
};

int main(int argc, char argv[])
{
	//////////
	// AsyncServer
	////////////
	boost::asio::io_service ioService;
	AsyncServer server(ioService);
	ioService.run();





	///////////
	// Server 
	///////////

	//std::cout << "I Be Survurer." << std::endl;
	//Server server(13);
	//

	//std::string message;
	//while (message != "q" && message != "Q")
	//{
	//	udp::endpoint remote_endpoint;	
	//	size_t len = 128;
	//	char recv_buf[128]; // size of len

	//	std::cout << "Waiting for message \n";
	//	// If message was received without error
	//	if(server.Receive(recv_buf, len, remote_endpoint))
	//	{ 
	//		server.ParseMessage(recv_buf, len, remote_endpoint);
	//		//server.Send("HejHej!!", remote_endpoint);
	//		//std::cout << "Size of message: " << len << "\n";
	//		//std::cout.write(recv_buf, len);
	//		//std::cout << "\n";
	//	}
	//}
	
	return 0;
}
