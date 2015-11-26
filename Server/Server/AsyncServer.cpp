#include "AsyncServer.h"



AsyncServer::AsyncServer(boost::asio::io_service& io_service) : socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 13))
{
	amountOfconnections = 0;
	Start();
}

AsyncServer::AsyncServer(boost::asio::io_service& io_service, int port) : socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
{
	amountOfconnections = 0;
	Start();
}


AsyncServer::~AsyncServer()
{
}

void AsyncServer::Start()
{

	socket.async_receive_from(
		boost::asio::buffer(receiveBuffer), receiver_endpoint,
		boost::bind(&AsyncServer::HandleReceive, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
}

void AsyncServer::HandleReceive(const boost::system::error_code & error, std::size_t bytes)
{
	std::cout << "Received message: " <<  std::string(receiveBuffer.c_array(), bytes) << "\n";
	if (!error || error == boost::asio::error::message_size)
	{
		if ("connect" == std::string(receiveBuffer.c_array(), bytes))
		{

			bool isConnected = false;
			for (size_t i = 0; i < maxAmountofConnections; i++)
			{
				// Check if already connected
				if (receiver_endpoint.address() == connections[i].address())
				{
					std::cout << "Player is already connected on ip: "
						<< connections[i].address().to_string() << "\n";
					isConnected = true;
					break;
				}
			}

			if (!isConnected)
			{
				// Check for empty spot
				for (size_t i = 0; i < maxAmountofConnections; i++)
				{
					if (boost::asio::ip::address() == connections[i].address())
					{
						connections[i] = receiver_endpoint;

						std::cout << "A Player has connected on ip" <<
							connections[i].address().to_string() << "\n";
						break;
					}
				}
			}

		}
		else if (("broadcast" == std::string(receiveBuffer.c_array(), bytes)))
		{
			boost::shared_ptr<std::string> message(new std::string("Brodcasted message: Hej"));

			for (size_t i = 0; i < maxAmountofConnections; i++)
			{
				if (connections[i].address() != boost::asio::ip::address())
				{
					std::cout << "Message was sent to: " << connections[i].address().to_string() << "\n";

					socket.async_send_to(boost::asio::buffer(*message), connections[i],
						boost::bind(&AsyncServer::HandleSend, this, message,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred));

				}

			}
		}
		else
		{
			boost::shared_ptr<std::string> message(new std::string("hej"));

			socket.async_send_to(boost::asio::buffer(*message), receiver_endpoint,
				boost::bind(&AsyncServer::HandleSend, this, message,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
		}
		Start();
	}
}

//void AsyncServer::HandleSend(char * data, const boost::system::error_code &, std::size_t)
//{
//	
//}

void AsyncServer::HandleSend(boost::shared_ptr<std::string> message, const boost::system::error_code &, std::size_t)
{
	std::cout << "HandleSend. Message sent: " << message->data() << "\n";
}
