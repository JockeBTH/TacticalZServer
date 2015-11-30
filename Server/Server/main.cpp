//#include "Server.h"
//#include "AsyncServer.h"
#include <boost/asio.hpp>
#include <ctime>

#include <string>
#include <queue>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/timer/timer.hpp>


#define HEADERSIZE sizeof(int)

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

struct PlayerPositions
{
	int x;
	int y;
};

enum class MsgType
{
	Connect,
	ClientPing,
	ServerPing,
	Message,
	Snapshot,
	Disconnect,
	Event

};


void DisplayLoop(); //Reads from the message queue and displays
void ReadFromClients(); //Read message from server and push to message queue
void WriteLoop(); //Reads input from user and writes to server
string* BuildPrompt();
int Receive(char* data, size_t len);
void HandleReceive(const boost::system::error_code&, const size_t);
void ParseConnect(char* data, size_t len);
void ParsePing();
void ParseMessage(char* data, size_t len);
void ParseSnapshot(char* data, size_t len);
void ParseDisconnect();
void ParseEvent(char* data, size_t len);
void Broadcast(string msg);
void Broadcast(char* msg, size_t len);
void MoveMsgHead(char*& data, size_t& len, size_t stepSize);
int CreateHeader(MsgType type, char* data);
int CreatePing(char* data, string msg);
int CreateEventMessage(int msgType, string message, char* dataLocation);


const int inputSize = 128;
const int maxAmountofConnections = 8;

udp::endpoint receiver_endpoint;
udp::endpoint connections[8];
string playerNames[8];
std::clock_t startPingTime;
double durationOfPingTime[8];

const int boardSize = 16;
char gameBoard[boardSize][boardSize];
PlayerPositions playerPositions[8];

messageQueue_ptr messageQueue(new queue<char*>);



boost::asio::io_service ioService;
boost::asio::ip::udp::socket serverSocket(ioService, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 13));


int main(int argc, char argv[])
{
	for (size_t i = 0; i < maxAmountofConnections; i++)
	{
		durationOfPingTime[i] = -1;
	}
	playerPositions[0].x = 0;
	playerPositions[0].y = 0;
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

int CreateHeader(MsgType type, char* data)
{
	int msgType = static_cast<int>(type);
	int offset = 0;
	memcpy(data, &msgType, sizeof(int));
	offset += sizeof(int);
	// Check so that HEADERSIZE is correct
	if (offset != HEADERSIZE)
		cout << "NOOOOOOOOOO! HEADERSIZE is wrong!!";

	return offset;
}

int CreateEventMessage(int msgType, string message, char* dataLocation)
{
	int lengthOfMsg = 0;

	lengthOfMsg = message.size();

	int offset = 0;
	// Message type
	memcpy(dataLocation + offset, &msgType, sizeof(int));
	offset += sizeof(int);
	// Message, add one extra byte for null terminator
	memcpy(dataLocation + offset, message.data(), (lengthOfMsg + 1) * sizeof(char));
	offset += (lengthOfMsg + 1) * sizeof(char);

	return offset;
}

int CreatePing(char* data, string msg)
{
	int length = msg.size() * sizeof(char);
	data = new char[length + HEADERSIZE];
	int offset = CreateHeader(MsgType::ServerPing, data);
	
	memcpy(data + offset, msg.data(), length);
	offset += length;
	return offset;
}

void ParsePing()
{
	char* testMsg = new char[128];
	int testOffset = CreateEventMessage(1, "Ping recieved", testMsg);
	
	cout << "Parsing ping." << endl;

	serverSocket.send_to(
		boost::asio::buffer(
			testMsg,
			testOffset),
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

void ParseConnect(char* data, size_t len)
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

			playerNames[i] = string(data);
			cout << "This is the player connected: " << playerNames[i] << endl;
			
			int offset = 0;
			char* temp = new char[sizeof(int) * 2];
			int msgType = 0;
			memcpy(temp, &msgType, sizeof(int));
			offset += sizeof(int);
			memcpy(temp + offset, &i, sizeof(int));
			
			serverSocket.send_to(
				boost::asio::buffer(temp, sizeof(int) * 2),
				connections[i],
				0);
			std::string str = "Player " + playerNames[i] + " connected on: " + receiver_endpoint.address().to_string();
			// +1 is the null terminator
			MoveMsgHead(data, len, playerNames[i].size() + 1);

			Broadcast(str);
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

void ParseEvent(char* data, size_t len)
{
	if ("+Forward" == string(data))
		if (playerPositions[0].y > 0)
		playerPositions[0].y--;
	if ("-Forward" == string(data))
		if (playerPositions[0].y < boardSize - 1)
		playerPositions[0].y++;
	if ("+Right" == string(data))
		if (playerPositions[0].x < boardSize - 1)
			playerPositions[0].x++;
	if ("-Right" == string(data))		
		if (playerPositions[0].x > 0)
			playerPositions[0].x--;


}

void Broadcast(string msg)
{
	char* data = new char[128];

	int offset = CreateEventMessage(static_cast<int>(MsgType::Event), msg, data);
	for (int i = 0; i < maxAmountofConnections; i++) {
		if (connections[i].address() != boost::asio::ip::address()) {
			serverSocket.send_to(
				boost::asio::buffer(data, offset),
				connections[i],
				0);
		}
	}
}

void Broadcast(char* msg, size_t len)
{
	for (int i = 0; i < maxAmountofConnections; i++) {
		if (connections[i].address() != boost::asio::ip::address()) {
			serverSocket.send_to(
				boost::asio::buffer(msg, len),
				connections[i],
				0);
		}
	}
}

void ParseMessage(char* data, size_t len)
{
	cout << "Parsed Message." << endl;
	messageQueue->push(data);
	// Move message head if multipel messages were sent.
	// +1 is for null terminator
	MoveMsgHead(data, len, string(data).size() + 1);
}

void ParseServerPing()
{
	for (int i = 0; i < maxAmountofConnections; i++) {
		if (connections[i].address() == receiver_endpoint.address()) {
			durationOfPingTime[i] = 1000 * (std::clock() - startPingTime)
				/ static_cast<double>(CLOCKS_PER_SEC);
			break;
		}
	}
}

int messagesReceived = 0;

void ParseMsgType(char* data, size_t len)
{
	// Debug
	++messagesReceived;

	// Message type 1 = Ping
	int messageType = -1;

	// Parse component data
	//memcpy(&messageType, data, len);

	memcpy(&messageType, data, sizeof(int));
	MoveMsgHead(data, len, sizeof(int));

	//MoveMsgHead(data, len, sizeof(int));
	//std::cout << "Message length: " << lengthOfMsg << endl;
	//std::cout << "Message was: " << string(data + sizeof(int), lengthOfMsg) << endl;


	switch (static_cast<MsgType>(messageType))
	{
	case MsgType::ClientPing:
		ParsePing();
		break;
	case MsgType::ServerPing:
		ParseServerPing();
		break;
	case MsgType::Snapshot:
		ParseSnapshot(data, len);
		break;
	case MsgType::Connect:
		ParseConnect(data, len);
		break;
	case MsgType::Message:
		ParseMessage(data, len);
		break;
	case MsgType::Disconnect:
		ParseDisconnect();
		break;
	case MsgType::Event:
		ParseEvent(data, len);
		break;
	default:
		break;
	}

	//cout << "Messages received: " << messagesReceived << endl;

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
				//serverSocket.send_to(boost::asio::buffer(inputMsg), receiver_endpoint, 0);
				Broadcast(inputMsg);
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
	int lengthOfMsg = -1;
	std::clock_t previousePingMsg = 1000 * std::clock() / static_cast<double>(CLOCKS_PER_SEC);
	int intervallMs = 5000;
	char* data;
	for (;;)
	{
		if (!messageQueue->empty()) 
		{
			try
			{
				size_t len = 0; // is not used
				data = messageQueue->front();
				memcpy(&lengthOfMsg, data, sizeof(int));
				MoveMsgHead(data, len, sizeof(int));

				cout << "This is the result: " << string(data, lengthOfMsg) << endl;
				messageQueue->pop();
			}
			catch (const std::exception& err)
			{
				cout << "Read from DisplayLoop crashed: " << err.what();
			}

		} 

		system("cls");
		for (size_t i = 0; i < boardSize; i++)
		{
			cout << "_";
		}
		cout << endl;
		for (size_t i = 0; i < boardSize; i++)
		{
			for (size_t j = 0; j < boardSize; j++)
			{
				if (i == playerPositions[0].y && j == playerPositions[0].x)
					cout << 'X';
				else cout << gameBoard[j][i];
			}
				cout << endl;
		}
		for (size_t i = 0; i < boardSize; i++)
		{
			cout << "_";
		}
		boost::this_thread::sleep(boost::posix_time::millisec(100));
		std::clock_t currentTime = 1000 * std::clock() / static_cast<double>(CLOCKS_PER_SEC);
		if (intervallMs < currentTime - previousePingMsg)
		{
			startPingTime = std::clock();
			char* data = new char[128];
			int len = CreateEventMessage(static_cast<int>(MsgType::ServerPing), "Ping from server", data);
			Broadcast(data, len);

			for (size_t i = 0; i < maxAmountofConnections; i++)
			{
				cout << "Player " << i << "'s ping: " << durationOfPingTime[i] << endl;
			}
			cout << endl;
			previousePingMsg = currentTime;
		}
	}
}

void MoveMsgHead(char*& data, size_t& len, size_t stepSize)
{
	data += stepSize;
	len -= stepSize;
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
