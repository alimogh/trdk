/**************************************************************************
 *   Created: 2017/01/04 01:15:35
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "Core/Service.hpp"
#include "Api.h"

namespace trdk {
namespace Services {
namespace Indicators {

//! Average Directional Movement Index.
/** @sa https://en.wikipedia.org/wiki/Average_directional_movement_index
  * @sa https://en.wikipedia.org/wiki/Average_true_range
  * @sa
 * https://ru.wikipedia.org/wiki/%D0%A1%D0%B8%D1%81%D1%82%D0%B5%D0%BC%D0%B0_%D0%BD%D0%B0%D0%BF%D1%80%D0%B0%D0%B2%D0%BB%D0%B5%D0%BD%D0%BD%D0%BE%D0%B3%D0%BE_%D0%B4%D0%B2%D0%B8%D0%B6%D0%B5%D0%BD%D0%B8%D1%8F
  *
  * Some of the details are wrong in Wiki, so original source (J. Welles
  * Wilder, Jr. book "New concepts in technical trading systems" book) has
  * been used to clarify.
  */
class TRDK_SERVICES_API Adx : public trdk::Service {
 public:
  //! General service error.
  class Error : public trdk::Lib::Exception {
   public:
    explicit Error(const char *) noexcept;
  };

  //! Throws when client code requests value which does not exist.
  class ValueDoesNotExistError : public Error {
   public:
    explicit ValueDoesNotExistError(const char *) noexcept;
  };

  //! Value data point.
  struct Point {
    //! Source data.
    struct Source {
      boost::posix_time::ptime time;
      trdk::Price open;
      trdk::Price high;
      trdk::Price low;
      trdk::Price close;
    } source;

    //! +DI (positive directional indicator).
    trdk::Lib::Double pdi;
    //! -DI (negative directional indicator).
    trdk::Lib::Double ndi;

    //! Average directional movement index.
    trdk::Lib::Double adx;
  };

 public:
  explicit Adx(Context &,
               const std::string &instanceName,
               const Lib::IniSectionRef &);
  virtual ~Adx() noexcept;

 public:
  virtual boost::posix_time::ptime GetLastDataTime() const override;

  bool IsEmpty() const;

  //! Returns last value point.
  /** @throw ValueDoesNotExistError
    */
  const Point &GetLastPoint() const;

 protected:
  virtual bool OnServiceDataUpdate(
      const trdk::Service &,
      const trdk::Lib::TimeMeasurement::Milestones &) override;

 private:
  class Implementation;
  std::unique_ptr<Implementation> m_pimpl;
};
}
}
}
