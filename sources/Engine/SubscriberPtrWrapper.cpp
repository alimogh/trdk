/**************************************************************************
 *   Created: 2012/11/22 11:46:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#include "Prec.hpp"
#include "SubscriberPtrWrapper.hpp"
#include "Core/Position.hpp"
#include "Core/Strategy.hpp"

namespace pt = boost::posix_time;

using namespace trdk;
using namespace trdk::Lib;
using namespace trdk::Engine;

//////////////////////////////////////////////////////////////////////////

namespace {

//////////////////////////////////////////////////////////////////////////

class AvailabilityCheckVisitor : public boost::static_visitor<bool> {
 public:
  template <typename Module>
  bool operator()(const Module &) const {
    return true;
  }
};
template <>
bool AvailabilityCheckVisitor::operator()(const Strategy &strategy) const {
  return !strategy.IsBlocked();
}

class BlockVisitor : public boost::static_visitor<void> {
 public:
  template <typename Module>
  void operator()(Module &) const throw() {}
};
template <>
void BlockVisitor::operator()(Strategy &strategy) const throw() {
  strategy.Block();
}

//////////////////////////////////////////////////////////////////////////
}  // namespace

//////////////////////////////////////////////////////////////////////////

const Module &SubscriberPtrWrapper::operator*() const {
  return boost::apply_visitor(Visitors::GetConstModule(), m_subscriber);
}

const Module *SubscriberPtrWrapper::operator->() const { return &**this; }

bool SubscriberPtrWrapper::operator==(const SubscriberPtrWrapper &rhs) const {
  return &**this == &*rhs;
}

bool SubscriberPtrWrapper::IsBlocked() const {
  return !boost::apply_visitor(AvailabilityCheckVisitor(), m_subscriber);
}

void SubscriberPtrWrapper::Block() const throw() {
  boost::apply_visitor(BlockVisitor(), m_subscriber);
}

void SubscriberPtrWrapper::RaiseLevel1UpdateEvent(
    Security &security,
    const TimeMeasurement::Milestones &delayMeasurement) const {
  class Visitor : public boost::static_visitor<void>,
                  private boost::noncopyable {
   public:
    explicit Visitor(Security &security,
                     const TimeMeasurement::Milestones &delayMeasurement)
        : m_security(security), m_delayMeasurement(delayMeasurement) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaiseLevel1UpdateEvent(m_security, m_delayMeasurement);
    }

   private:
    Security &m_security;
    const TimeMeasurement::Milestones &m_delayMeasurement;
  };

  boost::apply_visitor(Visitor(security, delayMeasurement), m_subscriber);
}

void SubscriberPtrWrapper::RaiseLevel1TickEvent(
    const Level1Tick &tick,
    const TimeMeasurement::Milestones &delayMeasurement) const {
  class Visitor : public boost::static_visitor<void>,
                  private boost::noncopyable {
   public:
    explicit Visitor(const Level1Tick &tick,
                     const TimeMeasurement::Milestones &delayMeasurement)
        : m_tick(tick), m_delayMeasurement(delayMeasurement) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaiseLevel1TickEvent(*m_tick.security, m_tick.time, m_tick.value,
                                    m_delayMeasurement);
    }

   private:
    const Level1Tick &m_tick;
    const TimeMeasurement::Milestones &m_delayMeasurement;
  };

  boost::apply_visitor(Visitor(tick, delayMeasurement), m_subscriber);
}

void SubscriberPtrWrapper::RaiseNewTradeEvent(
    const Trade &trade,
    const TimeMeasurement::Milestones &delayMeasurement) const {
  class Visitor : public boost::static_visitor<void>,
                  private boost::noncopyable {
   public:
    explicit Visitor(const Trade &trade,
                     const TimeMeasurement::Milestones &delayMeasurement)
        : m_trade(trade), m_delayMeasurement(delayMeasurement) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaiseNewTradeEvent(*m_trade.security, m_trade.time,
                                  m_trade.price, m_trade.qty);
    }

   private:
    const Trade &m_trade;
    const TimeMeasurement::Milestones &m_delayMeasurement;
  };

  boost::apply_visitor(Visitor(trade, delayMeasurement), m_subscriber);
}

void SubscriberPtrWrapper::RaisePositionUpdateEvent(Position &position) const {
  class Visitor : public boost::static_visitor<void>,
                  private boost::noncopyable {
   public:
    explicit Visitor(Position &position) : m_position(position) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaisePositionUpdateEvent(m_position);
    }

   private:
    Position &m_position;
  };

  boost::apply_visitor(Visitor(position), m_subscriber);
}

void SubscriberPtrWrapper::RaiseBrokerPositionUpdateEvent(
    const BrokerPosition &position,
    const TimeMeasurement::Milestones &delayMeasurement) const {
  class Visitor : public boost::static_visitor<void>,
                  private boost::noncopyable {
   public:
    explicit Visitor(const BrokerPosition &position,
                     const TimeMeasurement::Milestones &delayMeasurement)
        : m_position(position), m_delayMeasurement(delayMeasurement) {}

   public:
    void operator()(Strategy &strategy) const {
      return strategy.RaiseBrokerPositionUpdateEvent(
          *m_position.security, m_position.isLong, m_position.qty,
          m_position.volume, m_position.isInitial);
    }

   private:
    const BrokerPosition &m_position;
    const TimeMeasurement::Milestones &m_delayMeasurement;
  };

  boost::apply_visitor(Visitor(position, delayMeasurement), m_subscriber);
}

void SubscriberPtrWrapper::RaiseNewBarEvent(
    Security &security,
    const Security::Bar &bar,
    const TimeMeasurement::Milestones &delayMeasurement) const {
  class Visitor : public boost::static_visitor<void>,
                  private boost::noncopyable {
   public:
    explicit Visitor(Security &security,
                     const Security::Bar &bar,
                     const TimeMeasurement::Milestones &delayMeasurement)
        : m_source(security),
          m_bar(bar),
          m_delayMeasurement(delayMeasurement) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaiseNewBarEvent(m_source, m_bar);
    }

   private:
    Security &m_source;
    const Security::Bar &m_bar;
    const TimeMeasurement::Milestones &m_delayMeasurement;
  };

  boost::apply_visitor(Visitor(security, bar, delayMeasurement), m_subscriber);
}

void SubscriberPtrWrapper::RaiseBookUpdateTickEvent(
    Security &security,
    const PriceBook &book,
    const TimeMeasurement::Milestones &delayMeasurement) const {
  class Visitor : public boost::static_visitor<void>,
                  private boost::noncopyable {
   public:
    explicit Visitor(Security &security,
                     const PriceBook &book,
                     const TimeMeasurement::Milestones &delayMeasurement)
        : m_source(security),
          m_book(book),
          m_delayMeasurement(delayMeasurement) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaiseBookUpdateTickEvent(m_source, m_book, m_delayMeasurement);
    }

   private:
    Security &m_source;
    const PriceBook &m_book;
    const TimeMeasurement::Milestones &m_delayMeasurement;
  };

  boost::apply_visitor(Visitor(security, book, delayMeasurement), m_subscriber);
}

void SubscriberPtrWrapper::RaiseSecurityServiceEvent(
    const pt::ptime &time,
    Security &security,
    const Security::ServiceEvent &event) const {
  const class Visitor : public boost::static_visitor<void>,
                        private boost::noncopyable {
   public:
    explicit Visitor(const pt::ptime &time,
                     Security &security,
                     const Security::ServiceEvent &event)
        : m_time(time), m_source(security), m_event(event) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaiseSecurityServiceEvent(m_time, m_source, m_event);
    }

   private:
    const pt::ptime &m_time;
    Security &m_source;
    const Security::ServiceEvent &m_event;
  } visitor(time, security, event);

  boost::apply_visitor(visitor, m_subscriber);
}

void SubscriberPtrWrapper::RaiseSecurityContractSwitchedEvent(
    const pt::ptime &time,
    Security &security,
    Security::Request &request,
    bool &isSwitched) {
  const class Visitor : public boost::static_visitor<void>,
                        private boost::noncopyable {
   public:
    explicit Visitor(const pt::ptime &time,
                     Security &security,
                     Security::Request &request,
                     bool &isSwitched)
        : m_time(time),
          m_source(security),
          m_request(request),
          m_isSwitched(isSwitched) {}

   public:
    void operator()(Strategy &strategy) const {
      strategy.RaiseSecurityContractSwitchedEvent(m_time, m_source, m_request,
                                                  m_isSwitched);
    }

   private:
    const pt::ptime &m_time;
    Security &m_source;
    Security::Request &m_request;
    bool &m_isSwitched;
  } visitor(time, security, request, isSwitched);

  boost::apply_visitor(visitor, m_subscriber);
}

//////////////////////////////////////////////////////////////////////////
