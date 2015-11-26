//#include "Server.h"
//#include "AsyncServer.h"
#include <boost/asio.hpp>
#include <ctime>

#include <string>
#include <queue>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::asio::ip;
using namespace std;

typedef boost::shared_ptr<udp::socket> socket_ptr;
typedef boost::shared_ptr<std::string> string_ptr;
typedef boost::shared_ptr<queue<char*> > messageQueue_ptr;



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

enum class MsgType
{
	Connect,
	Ping,
	Message,
	Snapshot,
	Disconnect

};


void DisplayLoop(); //Reads from the message queue and displays
void ReadFromClients(); //Read message from server and push to message queue
void WriteLoop(); //Reads input from user and writes to server
string* BuildPrompt();
int Receive(char* data, size_t len);
void HandleReceive(const boost::system::error_code&, const size_t);
void ParseConnect();
void ParsePing();
void ParseMessage(char* data, size_t len);
void ParseSnapshot(char* data, size_t len);
void ParseDisconnect();
void Broadcast(string msg);

const int inputSize = 128;
const int maxAmountofConnections = 8;

udp::endpoint receiver_endpoint;
udp::endpoint connections[8];
messageQueue_ptr messageQueue(new queue<char*>);

boost::mutex mtx;

boost::asio::io_service ioService;
boost::asio::ip::udp::socket serverSocket(ioService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 13));


int main(int argc, char argv[])
{

	
	boost::thread_group threads;

	cout << "I am Server. BIP BOP\n";

	threads.create_thread(boost::bind(DisplayLoop));
	boost::this_thread::sleep(boost::posix_time::millisec(100));
	threads.create_thread(boost::bind(ReadFromClients));
	boost::this_thread::sleep(boost::posix_time::millisec(100));
	threads.create_thread(boost::bind(WriteLoop));
	boost::this_thread::sleep(boost::posix_time::millisec(100));

	threads.join_all();
	
	return 0;
}



int Receive(char* data, size_t len)
{

	len = serverSocket.receive_from(
		boost::asio::buffer((void*)data
			,len)
		,receiver_endpoint, 0);

	return len;
}

void HandleReceive(const boost::system::error_code&, size_t)
{
	//sock->send_to(boost::asio::buffer("Hejhej"), receiver_endpoint, 0);
}

void ParsePing()
{
	cout << "Parsing ping." << endl;
	serverSocket.send_to(
		boost::asio::buffer("Ping received"),
		receiver_endpoint,
		0);
}

void ParseSnapshot(char* data, size_t len)
{
	for (int i = 0; i < maxAmountofConnections; i++)
	{
		if (connections[i].address() != boost::asio::ip::address())
		{
			serverSocket.send_to(
				boost::asio::buffer("I'm sending a snapshot to you guys!"),
				connections[i],
				0);
		}
	}

}

void ParseConnect()
{
	cout << "Parsing connection." << endl;

	for (int i = 0; i < maxAmountofConnections; i++) {
		if (connections[i].address() == receiver_endpoint.address()) {
			return;
		}
	}

	for (int i = 0; i < maxAmountofConnections; i++) {
		if (connections[i].address() == boost::asio::ip::address()) {
			connections[i] = receiver_endpoint;
			std::string str = "Player " + to_string(i) + " connected on: " + receiver_endpoint.address().to_string();
			serverSocket.send_to(
				boost::asio::buffer(str),
				receiver_endpoint,
				0);
			Broadcast("A new player connected");
			break;
		}
	}

}

void ParseDisconnect()
{
	cout << "Parsing disconnect. \n";

	for (int i = 0; i < maxAmountofConnections; i++)
	{
		if (connections[i].address() == receiver_endpoint.address())
		{
			Broadcast("A player disconnected");

			connections[i] = boost::asio::ip::udp::endpoint();
			
			break;
		}
	}
}

void Broadcast(string msg)
{
	for (int i = 0; i < maxAmountofConnections; i++) {
		if (connections[i].address() != boost::asio::ip::address()) {
			serverSocket.send_to(
				boost::asio::buffer(msg),
				connections[i],
				0);
		}
	}
}

void ParseMessage(char* data, size_t len)
{
	cout << "Parsed Message." << endl;
	//char* msg = new char[inputSize];
	//memcpy(&msg, data + sizeof(char), len - sizeof(char));
	messageQueue->push(data);
	
}

void ParseMsgType(char* data, size_t len)
{
	// Message type 1 = Ping
	int messageType = -1;
	// Parse component data
	//memcpy(&messageType, data, len);

	string s = string(data, len);
	int usedInt = s[0] - '0'; //Hax

	std::cout << "Message type nr: " << usedInt << endl;
	std::cout << "String message: " << s << endl;

	switch (static_cast<MsgType>(usedInt))
	{
	case MsgType::Ping:
		ParsePing();
		break;
	case MsgType::Snapshot:
		ParseSnapshot(data, len);
		break;
	case MsgType::Connect:
		ParseConnect();
		break;
	case MsgType::Message:
		ParseMessage(data, len);
		break;
	case MsgType::Disconnect:
		ParseDisconnect();
		break;
	default:
		break;
	}

}

void ReadFromClients()
{
	char readBuf[1024] = { 0 };
	int bytesRead = 0;

	for (;;)
	{
	
		if (serverSocket.available())
		{
			try
			{
				bytesRead = Receive(readBuf, inputSize);
				ParseMsgType(readBuf, bytesRead);
			}
			catch (const std::exception& err)
			{
				cout << "Read from client crashed: " << err.what();
			}

		}
	}
}

void WriteLoop()
{
	char inputBuf[inputSize] = { 0 };
	string inputMsg;

	for (;;)
	{
		cin.getline(inputBuf, inputSize);
		inputMsg = (string)inputBuf;

		if (!inputMsg.empty())
		{
			try
			{
				serverSocket.send_to(boost::asio::buffer(inputMsg), receiver_endpoint, 0);
			}
			catch (const std::exception& err)
			{
				cout << "Read from WriteLoop crashed: " << err.what();
			}

		}

		if (inputMsg.find("exit") != string::npos)
			exit(1);
		inputMsg.clear();
		memset(inputBuf, 0, inputSize);
	}
}

void DisplayLoop()
{
	for (;;)
	{
		if (!messageQueue->empty()) 
		{
			try
			{
				cout << "This is the result: " << string((messageQueue->front())) << endl;
				messageQueue->pop();
			}
			catch (const std::exception& err)
			{
				cout << "Read from DisplayLoop crashed: " << err.what();
			}

		}
	}
}


//int main(int argc, char argv[])
//{
//	//////////
//	// AsyncServer
//	////////////
//	boost::asio::io_service ioService;
//	AsyncServer server(ioService);
//	ioService.run();
//
//
//
//	///////////
//	// Server 
//	///////////
//
//	//std::cout << "I Be Survurer." << std::endl;
//	//Server server(13);
//	//
//
//	//std::string message;
//	//while (message != "q" && message != "Q")
//	//{
//	//	udp::endpoint remote_endpoint;	
//	//	size_t len = 128;
//	//	char recv_buf[128]; // size of len
//
//	//	std::cout << "Waiting for message \n";
//	//	// If message was received without error
//	//	if(server.Receive(recv_buf, len, remote_endpoint))
//	//	{ 
//	//		server.ParseMessage(recv_buf, len, remote_endpoint);
//	//		//server.Send("HejHej!!", remote_endpoint);
//	//		//std::cout << "Size of message: " << len << "\n";
//	//		//std::cout.write(recv_buf, len);
//	//		//std::cout << "\n";
//	//	}
//	//}
//	
//	return 0;
//}
