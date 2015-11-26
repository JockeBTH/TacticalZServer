#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

const int maxAmountofConnections = 8;

enum class Messages
{
	Ping,
	Snapshot,
	Connect
};

struct ClientEndpointInfo
{
	boost::asio::ip::address address;
	int port = -1;
};

class Server
{
public:
	Server();
	Server(int port);
	~Server();
	//bool Connect(asio::ip::udp::endpoint& receiver_endpoint);
	bool Send(std::string message);
	bool Receive(char* data, size_t& len, boost::asio::ip::udp::endpoint& sender_endpoint);
	bool AsyncReceive(char * data, size_t & len, boost::asio::ip::udp::endpoint & sender_endpoint);
	bool Send(std::string message, boost::asio::ip::udp::endpoint receiver_endpoint);
	bool Send(char * data, size_t size, boost::asio::ip::udp::endpoint receiver_endpoint);
	void ParseSnapshot(char * data, size_t & len);
	void ParsePing(boost::asio::ip::udp::endpoint & sender_endpoint);
	void ParseConnect(boost::asio::ip::udp::endpoint & sender_endpoint);
	void ParseMessage(char * data, size_t & len, boost::asio::ip::udp::endpoint & sender_endpoint);



	template<typename ComponentType>
	bool Send(ComponentType comp)
	{
		char* buf = new char[comp.ByteSize()];
		comp.SerializeToArray(buf, comp.ByteSize());

		return Send(std::string(buf, comp.ByteSize()));
	}
private:
	int amountOfconnections;
	ClientEndpointInfo connectionArray[maxAmountofConnections];
	//void HandleReceive( , std::size_t bytes);
	boost::asio::io_service io_service;
	boost::asio::ip::udp::socket socket;
	//std::unique_ptr<asio::ip::udp::socket> socket;
	boost::asio::ip::udp::endpoint receiver_endpoint;
};