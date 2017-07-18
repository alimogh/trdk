/**************************************************************************
 *   Created: 2016/08/29 16:48:27
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "NetworkStreamClient.hpp"

namespace trdk {
namespace Lib {

class HttpStreamClient : public trdk::Lib::NetworkStreamClient {
 public:
  explicit HttpStreamClient(trdk::Lib::NetworkStreamClientService &,
                            const std::string &host,
                            size_t port);
  virtual ~HttpStreamClient();

 protected:
  virtual void HandleNewSubMessages(
      const boost::posix_time::ptime &time,
      const Buffer::const_iterator &begin,
      const Buffer::const_iterator &end,
      const TimeMeasurement::Milestones &timeMeasurement) = 0;

  virtual void OnRequestComplete() = 0;

 protected:
  //! Sends request.
  /** @param[in] request            Request.
    * @param[in] onRequestCallback  Will be called after request if request
    *                               has been sent.
    */
  void Request(const std::string &request,
               const boost::function<void(void)> &onRequestCallback);

 protected:
  virtual void OnStart();

  virtual Buffer::const_iterator FindLastMessageLastByte(
      const Buffer::const_iterator &bufferBegin,
      const Buffer::const_iterator &transferedBegin,
      const Buffer::const_iterator &bufferEnd) const;

  virtual void HandleNewMessages(
      const boost::posix_time::ptime &time,
      const Buffer::const_iterator &begin,
      const Buffer::const_iterator &end,
      const trdk::Lib::TimeMeasurement::Milestones &);

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}