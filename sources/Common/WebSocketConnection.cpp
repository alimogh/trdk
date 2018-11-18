//
//    Created: 2018/04/07 3:47 PM
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

#include "Prec.hpp"
#include "WebSocketConnection.hpp"
#include "Constants.h"
#include "Exception.hpp"

using namespace trdk;
using namespace Lib;
using namespace TimeMeasurement;
namespace io = boost::asio;
using tcp = io::ip::tcp;
namespace ssl = io::ssl;
namespace beast = boost::beast;
namespace ws = beast::websocket;
namespace ptr = boost::property_tree;
namespace pt = boost::posix_time;
namespace ios = boost::iostreams;
namespace is = boost::iostreams;

WebSocketConnection::Events::Events(
    boost::function<EventInfo()> read,
    boost::function<void(EventInfo, const boost::property_tree::ptree &)>
        message,
    boost::function<void()> disconnect,
    boost::function<void(const std::string &)> debug,
    boost::function<void(const std::string &)> info,
    boost::function<void(const std::string &)> warn,
    boost::function<void(const std::string &)> error)
    : read(std::move(read)),
      message(std::move(message)),
      disconnect(std::move(disconnect)),
      debug(std::move(debug)),
      info(std::move(info)),
      warn(std::move(warn)),
      error(std::move(error)) {}

class WebSocketConnection::Implementation {
 public:
  WebSocketConnection &m_self;
  const std::string m_host;
  io::io_context m_ioContext;
  ssl::context m_sslContext{ssl::context::sslv23};
  ws::stream<ssl::stream<io::ip::tcp::socket>> m_stream{m_ioContext,
                                                        m_sslContext};

  boost::optional<boost::thread> m_thread;

  explicit Implementation(WebSocketConnection &self, std::string host)
      : m_self(self), m_host(std::move(host)) {}
  Implementation(Implementation &&) = delete;
  Implementation(const Implementation &) = delete;
  Implementation &operator=(Implementation &&) = delete;
  Implementation &operator=(const Implementation &) = delete;
  ~Implementation() {
    try {
      m_self.Stop();
    } catch (...) {
      AssertFailNoException();
      terminate();
    }
  }

  void Run(const Events &events) {
    io::spawn(m_ioContext, [this, &events](const io::yield_context &yield) {
      for (io::streambuf buffer;;) {
        boost::system::error_code error;
        m_stream.async_read(buffer, yield[error]);

        auto info = events.read();

        if (error) {
          boost::format errorMessage("Failed to read: \"%1%\" (code: %2%).");
          errorMessage % error.message()  // 1
              % error.value();            // 2
          events.error(errorMessage.str());
          return;
        }

        if (buffer.size() == 0) {
          events.debug("Connection closed.");
          return;
        }

        std::string messageContent;
        std::istream(&buffer) >> messageContent;

        ptr::ptree message;
        {
          is::stream_buffer<is::array_source> messageBuffer(
              messageContent.c_str(),
              messageContent.c_str() + messageContent.size());
          std::istream messageStream(&messageBuffer);
          try {
            ptr::read_json(messageStream, message);
            AssertEq(0, buffer.size());
          } catch (const ptr::json_parser_error &ex) {
            boost::format errorMessage(
                R"(Failed to parse server response: "%1%". Message: %2%)");
            errorMessage % ex.what()  // 1
                % messageContent;     // 2
            events.debug(errorMessage.str());
            return;
          }
        }

        try {
          events.message(std::move(info), message);
        } catch (const Exception &ex) {
          boost::format errorMessage(
              "Application error occurred while reading server message: "
              "\"%1%\". Message: %2%");
          errorMessage % ex.what()  // 1
              % messageContent;     // 2
          events.error(errorMessage.str());
        } catch (const std::exception &ex) {
          boost::format errorMessage(
              "System error occurred while reading server message: \"%1%\". "
              "Message: %2%");
          errorMessage % ex.what()  // 1
              % messageContent;     // 2
          events.error(errorMessage.str());
          return;
        }
      }
    });

    m_ioContext.run();
    events.disconnect();
  }
};

WebSocketConnection::WebSocketConnection(std::string host)
    : m_pimpl(boost::make_unique<Implementation>(*this, std::move(host))) {}
WebSocketConnection::WebSocketConnection(WebSocketConnection &&) noexcept =
    default;
WebSocketConnection::~WebSocketConnection() = default;

void WebSocketConnection::Connect(const std::string &port) {
  auto &socket = m_pimpl->m_stream.next_layer().next_layer();
  m_pimpl->m_stream.next_layer().set_verify_mode(ssl::verify_none);

  const auto endpoints =
      tcp::resolver(m_pimpl->m_ioContext).resolve(m_pimpl->m_host, port);
  io::connect(socket, endpoints.begin(), endpoints.end());

  socket.set_option(tcp::no_delay(true));

  m_pimpl->m_stream.next_layer().handshake(ssl::stream_base::client);
}

void WebSocketConnection::Handshake(const std::string &target) {
  m_pimpl->m_stream.handshake_ex(m_pimpl->m_host, target, [](auto &request) {
    request.insert(beast::http::field::user_agent,
                   TRDK_NAME " " TRDK_BUILD_IDENTITY);
  });
}

void WebSocketConnection::Start(const Events &events) {
  Assert(!m_pimpl->m_thread);
  if (m_pimpl->m_thread) {
    throw std::runtime_error("Connection is already started");
  }
  m_pimpl->m_thread.emplace([this, events]() {
    try {
      events.debug("Starting WebSocket service task...");
      m_pimpl->Run(events);
      events.debug("WebSocket service task is completed.");
    } catch (...) {
      AssertFailNoException();
      throw;
    }
  });
}

void WebSocketConnection::Stop() {
  if (!m_pimpl->m_thread) {
    return;
  }
  m_pimpl->m_ioContext.stop();
  m_pimpl->m_thread->join();
  m_pimpl->m_thread = boost::none;
}

void WebSocketConnection::Write(const ptr::ptree &message) {
  std::ostringstream oss;
  ptr::write_json(oss, message, false);
  m_pimpl->m_stream.write(io::buffer(oss.str()));
}
