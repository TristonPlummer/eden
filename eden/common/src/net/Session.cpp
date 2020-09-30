#include <shaiya/common/net/Session.hpp>

#include <glog/logging.h>

using namespace shaiya::net;

/**
 * Creates a new session from an io context.
 * @param context   The io context.
 */
Session::Session(boost::asio::io_context& context): socket_(context)
{
}

/**
 * Reads incoming data from this session.
 */
void Session::read()
{
    auto handler = [session = shared_from_this()](const boost::system::error_code& error, size_t bytesTransferred) {
        session->handleRead(error, bytesTransferred);
    };
    socket_.async_read_some(boost::asio::buffer(buf_, buf_.size()), handler);
}

/**
 * Handles a completed read event.
 * @param error             The error code returned.
 * @param bytesTransferred  The number of bytes that were read from the socket.
 */
void Session::handleRead(const boost::system::error_code& error, size_t bytesTransferred)
{
    // Ensure that data could be properly read
    if (error || bytesTransferred <= 0)
    {
        return close();
    }

    // Read the header of the packet
    size_t length = *reinterpret_cast<uint16_t*>(&buf_);
    size_t opcode = *reinterpret_cast<uint16_t*>(&buf_[2]);
    char* payload = &buf_[2];

    // If the prefixed size doesn't match the number of bytes read, then either something went wrong in transport
    // or the packet was incorrectly forged by a user.
    if (length != bytesTransferred)
    {
        LOG(INFO) << "Expected " << length << " bytes for the packet (opcode " << opcode << ") but received "
                  << bytesTransferred << " bytes.";
        return close();
    }

    // Execute the payload
    onRead(opcode, length - 2, payload);

    // Start reading more data
    std::fill_n(buf_.begin(), bytesTransferred, 0);
    read();
}

/**
 * Handles the callback of a write event
 * @param error                 The return error code.
 * @param bytesTransferred      The number of bytes that were written
 */
void Session::handleWrite(const boost::system::error_code& error, size_t bytesTransferred)
{
    if (error || bytesTransferred <= 0)
    {
        close();
    }
}

/**
 * Gets executed when this session is disconnected.
 */
void Session::onDisconnect()
{
}

/**
 * Gracefully closes this session's connection.
 */
void Session::close()
{
    try
    {
        if (!socket_.is_open())
            return;

        onDisconnect();
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        socket_.close();
    }
    catch (const std::exception& e)
    {
    }
}

/**
 * Gets the socket for this session.
 * @return  The socket.
 */
boost::asio::ip::tcp::socket& Session::socket()
{
    return socket_;
}

/**
 * Gets the remote address of this session, in IPV4 format.
 * @return  The remote address.
 */
std::string_view Session::remoteAddress()
{
    // The remote address should be lazily initialised, as the session
    // can be initialised without an active endpoint.
    if (remoteAddress_.empty())
    {
        auto address   = socket_.remote_endpoint().address().to_v4();
        remoteAddress_ = address.to_string();
    }

    return remoteAddress_;
}