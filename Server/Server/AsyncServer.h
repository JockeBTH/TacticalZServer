#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#define MESSAGESIZE 128
const int maxAmountofConnections = 8;

struct ClientEndpointInfo
{
	boost::asio::ip::address address;
	int port = -1;
};

class AsyncServer
{
public:
	AsyncServer(boost::asio::io_service& io_service);
	AsyncServer(boost::asio::io_service& io_service, int port);
	~AsyncServer();

private:
	int amountOfconnections;
	ClientEndpointInfo connectionArray[maxAmountofConnections];
	void Start();
	void HandleReceive(const boost::system::error_code& error, std::size_t bytes);
	void HandleSend(boost::shared_ptr<std::string> message /*message*/,
		const boost::system::error_code& /*error*/,
		std::size_t /*bytes_transferred*/);

	//void HandleSend(char* data/*message*/,
	//	const boost::system::error_code& /*error*/,
	//	std::size_t /*bytes_transferred*/);

	//boost::asio::io_service io_service;
	boost::asio::ip::udp::socket socket;
	boost::asio::ip::udp::endpoint receiver_endpoint;
	boost::asio::ip::udp::endpoint connections[8];
	boost::array<char, MESSAGESIZE> receiveBuffer;
};

