#include "Server.h"
#include <functional>


Server::Server() :io_service(), socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 13))
{
	amountOfconnections = 0;
}

Server::Server(int port) : io_service(), socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
{
	amountOfconnections = 0;
}

Server::~Server()
{
}

bool Server::Send(std::string message)
{
	//boost::asio::error errCode;
	socket.send_to(boost::asio::buffer(message), receiver_endpoint, 0);
	/*if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;*/

	return true;//!errCode;
}

bool Server::Send(std::string message, boost::asio::ip::udp::endpoint receiver_endpoint)
{
	//boost::asio::error errCode;
	socket.send_to(boost::asio::buffer(message), receiver_endpoint, 0);
	//if (errCode.value() != 0)
	//	std::cerr << errCode.message() << std::endl;
	//return !errCode;
	return true;
}

bool Server::Send(char* data, size_t size, boost::asio::ip::udp::endpoint receiver_endpoint)
{
//	asio::error_code errCode;
	socket.send_to(boost::asio::buffer(data, size), receiver_endpoint, 0/*, errCode*/);
	//if (errCode.value() != 0)
	//	std::cerr << errCode.message() << std::endl;
	//return !errCode;

	return true;
}


bool Server::Receive(char* data, size_t& len, boost::asio::ip::udp::endpoint& sender_endpoint)
{
	/*asio::error_code errCode;*/
	len = socket.receive_from(boost::asio::buffer((void*)data, len), sender_endpoint, 0/*, errCode*/);
	/*if (errCode.value() != 0)
		std::cerr << errCode.message() << std::endl;
	return !errCode;*/
	return true;
}

void Server::ParseSnapshot(char* data, size_t& len)
{
	// Parse
	int nameLengthParsed = 0;
	int bananParsed = 0;
	float temperatureParsed = 0.0f;
	// used to read the data from the byte array
	// Starts on 4 cause first 4 bytes are the message type
	int dataOffset = 4; 

						// Get length of string
	memcpy(&nameLengthParsed, data + dataOffset, sizeof(int));
	dataOffset += sizeof(int);

	// String as char array
	char* temp = new char[nameLengthParsed + 1];
	temp[nameLengthParsed] = '\0';
	memcpy(temp, data + dataOffset, nameLengthParsed);
	dataOffset += nameLengthParsed;

	// Parse component data
	memcpy(&bananParsed, data + dataOffset, sizeof(int));
	dataOffset += sizeof(int);
	// Parse component data
	memcpy(&temperatureParsed, data + dataOffset, sizeof(float));
	dataOffset += sizeof(float);

	std::cout << "\n";
	std::cout << "Name length: " << nameLengthParsed << +"\n";
	std::cout << "Name: " << temp << +"\n";
	std::cout << "Int Nr: " << bananParsed << +"\n";
	std::cout << "Float Nr: " << temperatureParsed << +"\n";
}

void Server::ParsePing(boost::asio::ip::udp::endpoint& sender_endpoint)
{

}

void Server::ParseConnect(boost::asio::ip::udp::endpoint& sender_endpoint)
{
	if (amountOfconnections < maxAmountofConnections)
	{
		for (size_t i = 0; i < maxAmountofConnections; i++)
		{
			if (connectionArray[i].port == -1)
			{
				connectionArray[i].address = sender_endpoint.address();
				connectionArray[i].port = sender_endpoint.port();
				++amountOfconnections;
				boost::asio::io_service temp_io_service;
				boost::asio::ip::udp::resolver resolver(temp_io_service);
				boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), connectionArray[i].address.to_string(), std::to_string(connectionArray[i].port));
				receiver_endpoint = *resolver.resolve(query);
				// Do bit conversion of User id
				char* userIdBitData = new char[sizeof(int)];
				memcpy(userIdBitData, &i, sizeof(int));
				// Send user id
				Send(userIdBitData, sizeof(int), sender_endpoint);
				break;
			}
		}
	}
	
}

void Server::ParseMessage(char* data, size_t& len, boost::asio::ip::udp::endpoint& sender_endpoint)
{

	// Message type 1 = Ping
	int messageType = -1;
	// Parse component data
	memcpy(&messageType, data, sizeof(int));

	std::cout << messageType;

	switch (static_cast<Messages>(messageType))
	{
	case Messages::Ping:
		ParsePing(sender_endpoint);
		break;
	case Messages::Snapshot:
		ParseSnapshot(data, len);
		break;
	case Messages::Connect:
		ParseConnect(sender_endpoint);
		break;
	default:
		break;
	}
}

