/**************************************************************************
 *   Created: 2012/07/17 21:03:54
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include <boost/filesystem/path.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <string>

namespace trdk {
namespace Lib {

class FileSystemChangeNotificator : private boost::noncopyable {
 public:
  typedef boost::function<void()> EventSlot;

 public:
  FileSystemChangeNotificator(const boost::filesystem::path &,
                              const EventSlot &eventSlot);
  ~FileSystemChangeNotificator();

 public:
  void Start();
  void Stop();

 private:
  class Implementation;
  Implementation *m_pimpl;
};
}
}
