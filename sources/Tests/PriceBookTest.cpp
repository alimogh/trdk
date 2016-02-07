/*******************************************************************************
 *   Created: 2016/02/03 00:13:04
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 ******************************************************************************/

#include "Prec.hpp"
#include "Core/PriceBook.hpp"

namespace pt = boost::posix_time;

namespace trdk { namespace Tests {

	template<typename BookSide>
	void TestCleanPriceBookSide(BookSide &side) {
		EXPECT_EQ(0, side.GetSize());
		EXPECT_TRUE(side.IsEmpty());
		EXPECT_THROW(side.GetTop(), trdk::Lib::LogicError);
		EXPECT_THROW(side.GetLevel(0), trdk::Lib::LogicError);
		EXPECT_THROW(side.GetLevel(1), trdk::Lib::LogicError);
		EXPECT_THROW(
			side.GetLevel(PriceBook::GetSideMaxSize() - 1),
			trdk::Lib::LogicError);
		EXPECT_THROW(
			side.GetLevel(PriceBook::GetSideMaxSize()),
			trdk::Lib::LogicError);
		ASSERT_THROW(side.PopTop(), trdk::Lib::LogicError);
	}

	void TestPriceBookClean(PriceBook &book) {
		{
			SCOPED_TRACE("Bid"); 
			TestCleanPriceBookSide(book.GetBid());
		}
		{
			SCOPED_TRACE("Ask"); 
			TestCleanPriceBookSide(book.GetAsk());
		}
	}

	TEST(BookPriceTest, NewBook) {
		{
			PriceBook book;
			EXPECT_EQ(book.GetTime(), pt::not_a_date_time);
			SCOPED_TRACE("Has no time"); 
			TestPriceBookClean(book);
		}
		{
			const pt::ptime &time = pt::microsec_clock::local_time();
			PriceBook book(time);
			EXPECT_EQ(time, book.GetTime());
			SCOPED_TRACE("Has time");
			TestPriceBookClean(book);
		}
	}

	TEST(BookPriceTest, Level) {

		PriceBook::Level level;

		EXPECT_EQ(level.GetTime(), pt::not_a_date_time);
		EXPECT_DOUBLE_EQ(0, level.GetPrice());
		EXPECT_DOUBLE_EQ(0, level.GetQty());

		auto time = pt::microsec_clock::local_time();
		level.UpdateTime(time);
		EXPECT_EQ(time, level.GetTime());
		time += pt::microseconds(1);
		level.UpdateTime(time);
		EXPECT_EQ(time, level.GetTime());
		level.UpdateTime(time - pt::microseconds(1));
		EXPECT_EQ(time, level.GetTime());

		const Qty qty = 123456.789;
		level += qty;
		EXPECT_DOUBLE_EQ(0, level.GetPrice());
		EXPECT_DOUBLE_EQ(qty, level.GetQty());
		level += qty;
		EXPECT_DOUBLE_EQ(0, level.GetPrice());
		EXPECT_DOUBLE_EQ(qty * 2, level.GetQty());

		const double price = 98765.431;
		PriceBook::Level level2(time, price, qty);
		EXPECT_EQ(time, level2.GetTime());
		EXPECT_DOUBLE_EQ(price, level2.GetPrice());
		EXPECT_DOUBLE_EQ(qty, level2.GetQty());

		level2 += level;
		EXPECT_DOUBLE_EQ(price, level2.GetPrice());
		EXPECT_DOUBLE_EQ(qty * 3, level2.GetQty());
	
	}

	TEST(BookPriceTest, SetTime) {
		PriceBook book;
		EXPECT_EQ(book.GetTime(), pt::not_a_date_time);
		const pt::ptime &time = pt::microsec_clock::local_time();
		book.SetTime(time);
		EXPECT_EQ(time, book.GetTime());
	}

	template<typename Side>
	class PriceBookSideGenerator {

	private:

		template<typename Side>
		struct Sort {
			//...//
		};
		template<>
		struct Sort<PriceBook::Ask> {
			typedef std::less<double> Algo;
		};
		template<>
		struct Sort<PriceBook::Bid> {
			typedef std::greater<double> Algo;
		};

		struct Level {
			pt::ptime time;
			Qty qty;
			bool isUsed;
		};

	public:

		explicit PriceBookSideGenerator(size_t size)
			: m_size(size)
			, m_randomRange(.00001, .00002)
			, m_generateRandom(m_random, m_randomRange) {
			//...//
		}

		void Validate(const Side &side) {
			
			ASSERT_FALSE(side.IsEmpty());
			const auto size
				= std::min(m_source.size(), PriceBook::GetSideMaxSize());
			ASSERT_EQ(size, side.GetSize());
			
			size_t  i = 0;
			foreach (const auto &source, m_source) {
			
				if (!source.second.isUsed) {
					continue;
				}
			
				const PriceBook::Level *level = nullptr;
				ASSERT_NO_THROW(level = &side.GetLevel(i));
				EXPECT_DOUBLE_EQ(source.first, level->GetPrice());
				EXPECT_EQ(source.second.time, level->GetTime());
				EXPECT_DOUBLE_EQ(source.second.qty, level->GetQty());
			
				if (++i >= size) {
					break;
				}
			
			}
			
			EXPECT_THROW(side.GetLevel(m_source.size()), trdk::Lib::LogicError);
			EXPECT_EQ(&side.GetLevel(0), &side.GetTop());
		
		}

		void GeneratePriceUpdates(Side &side) {

			while (m_source.size() < m_size) {
			
				double price = 0;
				do {
					price = m_generateRandom();
				} while (m_source.count(price));
			
				const Level level = {
					pt::microsec_clock::local_time()
						+ pt::microseconds(int64_t(m_generateRandom())),
					m_generateRandom() * 100000000
				};

				const auto &pos
					= m_source.emplace(std::make_pair(price, level)).first;
				const size_t index = std::distance(m_source.begin(), pos);

				pos->second.isUsed = side.Update(level.time, price, level.qty);
				ASSERT_EQ(
					std::min(m_source.size(), PriceBook::GetSideMaxSize()),
					side.GetSize());

				ASSERT_EQ(
					index < PriceBook::GetSideMaxSize(),
					pos->second.isUsed);

			}

		}

		void GeneratePriceAdd(Side &side) {

			size_t count = 0;

			while (m_source.size() < m_size) {
			
				double price = 0;
				do {
					price = m_generateRandom();
				} while (m_source.count(price));

				const Level level = {
					pt::microsec_clock::local_time()
						+ pt::microseconds(int64_t(m_generateRandom())),
					m_generateRandom() * 100000000
				};
			
				const auto &pos
					= m_source.emplace(std::make_pair(price, level)).first;

				if (count < PriceBook::GetSideMaxSize()) {
				
					ASSERT_NO_THROW(side.Add(level.time, price, level.qty));
					++count;
					pos->second.isUsed = true;

					foreach (const auto &source, m_source) {
						if (!source.second.isUsed) {
							continue;
						}
						ASSERT_THROW(
							side.Add(level.time, source.first, level.qty),
							trdk::Lib::Exception);
					}
				
				} else {
				
					ASSERT_THROW(
						side.Add(level.time, price, level.qty),
						trdk::Lib::Exception);
				
				}

				ASSERT_EQ(count, side.GetSize());

			}

		}

		void UpdateQty(Side &side, const Qty &qty) {
			size_t i = 0;
			foreach (auto &level, m_source) {
				level.second.qty += qty;
				EXPECT_EQ(
					++i <= PriceBook::GetSideMaxSize(),
					side.Update(level.second.time, level.first, qty));
			}
		}

	private:

		size_t m_size;
		size_t m_minValue;
		boost::mt19937 m_random;
		boost::uniform_real<double> m_randomRange;
		boost::variate_generator<decltype(m_random), decltype(m_randomRange)>
			m_generateRandom;

		std::map<double, Level, typename Sort<Side>::Algo> m_source;

	};

	void TestRandomPriceBookLevelsUpdate(PriceBook &book, size_t size) {

		PriceBookSideGenerator<PriceBook::Bid> bidLines(size);
		PriceBookSideGenerator<PriceBook::Ask> askLines(size);

		{
			SCOPED_TRACE("Insert bid"); 
			bidLines.GeneratePriceUpdates(book.GetBid());
			bidLines.Validate(book.GetBid());
		}
		{
			SCOPED_TRACE("Insert ask"); 
			askLines.GeneratePriceUpdates(book.GetAsk());
			askLines.Validate(book.GetAsk());
		}
		{
			SCOPED_TRACE("Insert bid after ask"); 
			bidLines.Validate(book.GetBid());
		}

		{
			SCOPED_TRACE("Copy ctor after"); 
			PriceBook bookX(book);
			askLines.Validate(bookX.GetAsk());
			bidLines.Validate(bookX.GetBid());
		}
		{
			SCOPED_TRACE("Copy operator"); 
			PriceBook bookX;
			bookX = book;
			askLines.Validate(bookX.GetAsk());
			bidLines.Validate(bookX.GetBid());
		}

		{
			SCOPED_TRACE("Plus bid"); 
			bidLines.UpdateQty(book.GetBid(), 123457890);
			bidLines.Validate(book.GetBid());
		}
		{
			SCOPED_TRACE("Plus ask"); 
			askLines.UpdateQty(book.GetAsk(), 123457890);
			askLines.Validate(book.GetAsk());
		}
		{
			SCOPED_TRACE("Plus bid after ask"); 
			bidLines.Validate(book.GetBid());
		}

		{
			bidLines.UpdateQty(book.GetBid(), -123457890);
			SCOPED_TRACE("Minus bid"); 
			bidLines.Validate(book.GetBid());
		}
		{
			SCOPED_TRACE("Minus ask"); 
			askLines.UpdateQty(book.GetAsk(), -123457890);
			askLines.Validate(book.GetAsk());
		}
		{
			SCOPED_TRACE("Minus bid after ask"); 
			bidLines.Validate(book.GetBid());
		}
	
	}

	TEST(BookPriceTest, UpdateRandomLevels) {
		PriceBook book;
		{
			SCOPED_TRACE("Full");
			TestRandomPriceBookLevelsUpdate(book, book.GetSideMaxSize() * 2);
		}
		{
			SCOPED_TRACE("Clear");
			book.Clear();
			TestPriceBookClean(book);
		}
		{
			SCOPED_TRACE("Half"); 
			TestRandomPriceBookLevelsUpdate(book, book.GetSideMaxSize() / 2);
		}
	}

	void TestRandomPriceBookLevelsAdd(PriceBook &book, size_t size) {

		PriceBookSideGenerator<PriceBook::Bid> bidLines(size);
		PriceBookSideGenerator<PriceBook::Ask> askLines(size);

		{
			SCOPED_TRACE("Insert bid"); 
			bidLines.GeneratePriceAdd(book.GetBid());
			bidLines.Validate(book.GetBid());
		}
		{
			SCOPED_TRACE("Insert ask"); 
			askLines.GeneratePriceAdd(book.GetAsk());
			askLines.Validate(book.GetAsk());
		}
		{
			SCOPED_TRACE("Insert bid after ask"); 
			bidLines.Validate(book.GetBid());
		}

		{
			SCOPED_TRACE("Copy ctor after"); 
			PriceBook bookX(book);
			askLines.Validate(bookX.GetAsk());
			bidLines.Validate(bookX.GetBid());
		}
		{
			SCOPED_TRACE("Copy operator"); 
			PriceBook bookX;
			bookX = book;
			askLines.Validate(bookX.GetAsk());
			bidLines.Validate(bookX.GetBid());
		}
	
	}

	TEST(BookPriceTest, AddRandomLevels) {
		PriceBook book;
		{
			SCOPED_TRACE("Full");
			TestRandomPriceBookLevelsAdd(book, book.GetSideMaxSize() * 2);
		}
		{
			SCOPED_TRACE("Clear");
			book.Clear();
			TestPriceBookClean(book);
		}
		{
			SCOPED_TRACE("Half"); 
			TestRandomPriceBookLevelsAdd(book, book.GetSideMaxSize() / 2);
		}
	}

	TEST(BookPriceTest, UpdatePredefinedLevels) {

		PriceBook book;
		PriceBook::Bid &bid = book.GetBid();
		PriceBook::Ask &ask = book.GetAsk();
			
		EXPECT_TRUE(bid.Update(pt::not_a_date_time, 1, 1));
		EXPECT_TRUE(bid.Update(pt::not_a_date_time, 2, 2));
		EXPECT_TRUE(bid.Update(pt::not_a_date_time, 3, 3));

		EXPECT_TRUE(ask.Update(pt::not_a_date_time, 1, 1));
		EXPECT_TRUE(ask.Update(pt::not_a_date_time, 2, 2));
		EXPECT_TRUE(ask.Update(pt::not_a_date_time, 3, 3));
		
		EXPECT_DOUBLE_EQ(3, bid.GetLevel(0).GetPrice());
		EXPECT_DOUBLE_EQ(3, bid.GetLevel(0).GetQty());
		EXPECT_DOUBLE_EQ(2, bid.GetLevel(1).GetPrice());
		EXPECT_DOUBLE_EQ(2, bid.GetLevel(1).GetQty());
		EXPECT_DOUBLE_EQ(1, bid.GetLevel(2).GetPrice());
		EXPECT_DOUBLE_EQ(1, bid.GetLevel(2).GetQty());

		EXPECT_DOUBLE_EQ(1, ask.GetLevel(0).GetPrice());
		EXPECT_DOUBLE_EQ(1, ask.GetLevel(0).GetQty());
		EXPECT_DOUBLE_EQ(2, ask.GetLevel(1).GetPrice());
		EXPECT_DOUBLE_EQ(2, ask.GetLevel(1).GetQty());
		EXPECT_DOUBLE_EQ(3, ask.GetLevel(2).GetPrice());
		EXPECT_DOUBLE_EQ(3, ask.GetLevel(2).GetQty());

		EXPECT_TRUE(bid.Update(pt::not_a_date_time, 1, 1));
		EXPECT_TRUE(bid.Update(pt::not_a_date_time, 2, 1));
		EXPECT_TRUE(bid.Update(pt::not_a_date_time, 3, 1));

		EXPECT_TRUE(ask.Update(pt::not_a_date_time, 1, 1));
		EXPECT_TRUE(ask.Update(pt::not_a_date_time, 2, 1));
		EXPECT_TRUE(ask.Update(pt::not_a_date_time, 3, 1));

		EXPECT_DOUBLE_EQ(3, bid.GetLevel(0).GetPrice());
		EXPECT_DOUBLE_EQ(4, bid.GetLevel(0).GetQty());
		EXPECT_DOUBLE_EQ(2, bid.GetLevel(1).GetPrice());
		EXPECT_DOUBLE_EQ(3, bid.GetLevel(1).GetQty());
		EXPECT_DOUBLE_EQ(1, bid.GetLevel(2).GetPrice());
		EXPECT_DOUBLE_EQ(2, bid.GetLevel(2).GetQty());

		EXPECT_DOUBLE_EQ(1, ask.GetLevel(0).GetPrice());
		EXPECT_DOUBLE_EQ(2, ask.GetLevel(0).GetQty());
		EXPECT_DOUBLE_EQ(2, ask.GetLevel(1).GetPrice());
		EXPECT_DOUBLE_EQ(3, ask.GetLevel(1).GetQty());
		EXPECT_DOUBLE_EQ(3, ask.GetLevel(2).GetPrice());
		EXPECT_DOUBLE_EQ(4, ask.GetLevel(2).GetQty());

	}

	TEST(BookPriceTest, AddPredefinedLevels) {

		PriceBook book;
		PriceBook::Bid &bid = book.GetBid();
		PriceBook::Ask &ask = book.GetAsk();
		
		bid.Add(pt::not_a_date_time, 3, 3);	
		bid.Add(pt::not_a_date_time, 2, 2);
		bid.Add(pt::not_a_date_time, 1, 1);

		ask.Add(pt::not_a_date_time, 3, 3);
		ask.Add(pt::not_a_date_time, 2, 2);
		ask.Add(pt::not_a_date_time, 1, 1);
		
		EXPECT_DOUBLE_EQ(3, bid.GetLevel(0).GetPrice());
		EXPECT_DOUBLE_EQ(3, bid.GetLevel(0).GetQty());
		EXPECT_DOUBLE_EQ(2, bid.GetLevel(1).GetPrice());
		EXPECT_DOUBLE_EQ(2, bid.GetLevel(1).GetQty());
		EXPECT_DOUBLE_EQ(1, bid.GetLevel(2).GetPrice());
		EXPECT_DOUBLE_EQ(1, bid.GetLevel(2).GetQty());

		EXPECT_DOUBLE_EQ(1, ask.GetLevel(0).GetPrice());
		EXPECT_DOUBLE_EQ(1, ask.GetLevel(0).GetQty());
		EXPECT_DOUBLE_EQ(2, ask.GetLevel(1).GetPrice());
		EXPECT_DOUBLE_EQ(2, ask.GetLevel(1).GetQty());
		EXPECT_DOUBLE_EQ(3, ask.GetLevel(2).GetPrice());
		EXPECT_DOUBLE_EQ(3, ask.GetLevel(2).GetQty());

	}

} }
