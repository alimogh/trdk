/**************************************************************************
 *   Created: 2014/09/02 23:31:12
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

#include "EquationPosition.hpp"
#include "Core/Strategy.hpp"

namespace trdk { namespace Strategies { namespace FxMb {

	//! Base class for all FxArb1-strategies.
	class FxArb1 : public Strategy {
		
	public:
		
		typedef Strategy Base;

	public:

		static const size_t nEquationsIndex;

	protected:

		enum {
			EQUATIONS_COUNT = 12,
			BROKERS_COUNT = 2,
			PAIRS_COUNT = 3
		};

		struct PositionConf {
			size_t index;
			Security *security;
			Qty qty;
			double requiredVol;
		};
		struct SecurityPositionConf : public PositionConf {
			Security *security;
			TradeSystem *tradeSystem;
		};

		struct BrokerConf {

			std::string name;

			std::map<std::string /* symbol */, PositionConf> pos;

			//! Securities order coincides with order in config
			//! and it is important.
			std::vector<SecurityPositionConf> sendList;

			TradeSystem *tradeSystem;

		};

		struct Broker {

			struct CheckedSecurity {
				
				SecurityPositionConf *conf;
				OrderSide side;

				CheckedSecurity()
						: conf(nullptr),
						side(numberOfOrderSides) {
					//...//
				}

			};

			typedef boost::array<CheckedSecurity, PAIRS_COUNT>
				CheckedSecurities[EQUATIONS_COUNT];

			struct Pair {

				struct Val {
					
					explicit Val(
								double val,
								SecurityPositionConf &security,
								const OrderSide &side,
								size_t &equationIndex,
								CheckedSecurities &checkedSecurities)
							: val(val),
							security(security),
							side(side),
							equationIndex(equationIndex),
							checkedSecurities(checkedSecurities) {
						//...//
					}

					operator bool() const {
						return !Lib::IsZero(val);
					}

					double Get() const {
						OnGet();
						return val;
					}

					double GetConst() const {
						return val;
					}

				private:

					Val & operator =(const Val &);

				private:

					void OnGet() const {
						foreach (
								auto &checkSecurity,
								checkedSecurities[equationIndex]) {
							if (!checkSecurity.conf) {
								checkSecurity.conf = &security;
								AssertEq(
									numberOfOrderSides,
									checkSecurity.side);
								checkSecurity.side = side;
								return;
							}
						}
						AssertFail("Too many pairs.");		
					}

				private:

					double val;
					SecurityPositionConf &security;
					const OrderSide side;

					size_t &equationIndex;
					CheckedSecurities &checkedSecurities;


				};
				
				CheckedSecurities checkedSecurities;

				Val bid;
				Val ask;
			
				
				explicit Pair(
							SecurityPositionConf &security,
							size_t &equationIndex,
							CheckedSecurities &checkedSecurities)
						: bid(
							security.security->GetBidPrice(),
							security,
							ORDER_SIDE_BID,
							equationIndex,
							checkedSecurities),
						ask(
							security.security->GetAskPrice(),
							security,
							ORDER_SIDE_ASK,
							equationIndex,
							checkedSecurities) {
					//...//
				}

				operator bool() const {
					return bid && ask;
				}

			};

			size_t equationIndex;
			CheckedSecurities checkedSecurities;

			Pair p1;
			Pair p2;
			Pair p3;

			explicit Broker(BrokerConf &conf)
 					: equationIndex(0),
					p1(conf.sendList[0], equationIndex, checkedSecurities),
 					p2(conf.sendList[1], equationIndex, checkedSecurities),
 					p3(conf.sendList[2], equationIndex, checkedSecurities) {
				const CheckedSecurity checkedSecurityInit;
				foreach (auto &e, checkedSecurities) {
					e.assign(checkedSecurityInit);
				}
			}

			operator bool() const {
				return p1 && p2 && p3;
			}

			void ResetCheckedSecurities() {
				const CheckedSecurity checkedSecurityInit;
				checkedSecurities[equationIndex].assign(checkedSecurityInit);
			}

		};

		typedef boost::function<
				bool (
					const Broker &b1,
					const Broker &b2,
					double &result)>
			Equation;
		typedef boost::function<
				void (
					const Broker &b1,
					const Broker &b2,
					trdk::Module::Log &)>
			EquationPrint;
		typedef std::vector<
				std::pair<Equation, EquationPrint>>
			Equations;

		struct EquationOpenedPositions {
			
			size_t activeCount;
			size_t waitsForReplyCount;
			std::vector<boost::shared_ptr<EquationPosition>> positions;

			boost::posix_time::ptime lastStartTime;

			bool isCanceled;

			EquationOpenedPositions()
					: activeCount(0),
					waitsForReplyCount(0),
					isCanceled(false) {
				//...//
			}

		};

	public:

		explicit FxArb1(
					Context &,
					const std::string &name,
					const std::string &tag,
					const Lib::IniSectionRef &);
		virtual ~FxArb1();

	public:

		virtual boost::posix_time::ptime OnSecurityStart(Security &);

		virtual void OnLevel1Update(
					Security &,
					Lib::TimeMeasurement::Milestones &);
		virtual void OnPositionUpdate(Position &);

		virtual void ReportDecision(const Position &) const;
		virtual std::auto_ptr<PositionReporter> CreatePositionReporter() const;

	protected:
		
		virtual void UpdateAlogImplSettings(const Lib::IniSectionRef &);

		Equations CreateEquations(const Lib::Ini &) const;

	protected:

		virtual void CheckOpportunity(
					Broker &b1,
					Broker &b2,
					Lib::TimeMeasurement::Milestones &)
				= 0;

		//! Checks conf. Must be called at each data update.
		void CheckConf();

		template<size_t id>
		Broker GetBroker() {
			return Broker(GetBrokerConf<id>());
		}

		const BrokerConf & GetBrokerConf(size_t id) const {
			return const_cast<FxArb1 *>(this)->GetBrokerConf(id);
		}
		template<size_t id>
		const BrokerConf & GetBrokerConf() const {
			return const_cast<FxArb1 *>(this)->GetBrokerConf<id>();
		}
		BrokerConf & GetBrokerConf(size_t id) {
			AssertLt(0, id);
			AssertGe(m_brokersConf.size(), id);
			return m_brokersConf[id - 1];
		}
		template<size_t id>
		BrokerConf & GetBrokerConf() {
			static_assert(0 < id, "Broker Index starts from 1.");
			AssertGe(m_brokersConf.size(), id);
			return m_brokersConf[id - 1];
		}

		const Equations & GetEquations() const {
			return m_equations;
		}

		size_t GetOppositeEquationIndex(size_t equationIndex) const {
			return equationIndex >= (EQUATIONS_COUNT / 2)
				?	equationIndex - (EQUATIONS_COUNT / 2)
				:	equationIndex + (EQUATIONS_COUNT / 2);
		}

		EquationOpenedPositions & GetEquationPositions(size_t equationIndex) {
			AssertLe(0, equationIndex);
			AssertGt(m_positionsByEquation.size(), equationIndex);
			return m_positionsByEquation[equationIndex];
		}
		const EquationOpenedPositions & GetEquationPositions(
					size_t equationIndex)
				const {
			return const_cast<FxArb1 *>(this)
				->GetEquationPositions(equationIndex);
		}

		void CloseEquation(size_t equationIndex, const Position::CloseType &);

		//! Cancels all opened for equation orders and close positions for it.
		size_t CancelEquation(
					size_t equationIndex,
					const Position::CloseType &closeType)
				throw();
	
		//! Sends open-orders for each configured security.
		void StartPositionsOpening(
					size_t equationIndex,
					size_t opposideEquationIndex,
					const Broker &b1,
					const Broker &b2,
					Lib::TimeMeasurement::Milestones &);

		bool CheckRestoreState();

		virtual void CloseDelayed(
					size_t,
					Lib::TimeMeasurement::Milestones &)
				= 0;

		//! Logging current bid/ask values for all pairs (if logging enabled).
		void LogBrokersState(
					size_t equationIndex,
					const Broker &,
					const Broker &)
				const;

		bool IsInGracePeriod(const EquationOpenedPositions &) const;
		void ResetGracePeriod();

	private:

		void OnOpportunityUpdate(Lib::TimeMeasurement::Milestones &);

		void OnOpportunityReturn();

		void DelayClose(EquationPosition &);
		void CloseDelayed(Lib::TimeMeasurement::Milestones &);

	private:

		const Equations m_equations;

		boost::array<BrokerConf, BROKERS_COUNT> m_brokersConf;

		boost::array<EquationOpenedPositions, EQUATIONS_COUNT>
			m_positionsByEquation;

		bool m_isPairsByBrokerChecked;

		std::bitset<EQUATIONS_COUNT> m_equationsForDelayedClosing;

		boost::posix_time::time_duration m_positionGracePeriod;

	};

} } }
