/**************************************************************************
 *   Created: 2013/01/18 15:58:23
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot Development Kit
 *       URL: http://robotdk.com
 * Copyright: Eugene V. Palchukovsky
 **************************************************************************/

#pragma once

namespace trdk { namespace PyApi { namespace Detail {

	template<typename T>
	class ObjectCache : private boost::noncopyable {

	public:

		typedef T ValueType;

	private:

		typedef std::map<T *, boost::python::object> Cache;

		//! @todo: place for optimization - rw lock with spin
		typedef boost::shared_mutex CacheMutex;
		typedef boost::shared_lock<CacheMutex> CacheReadLock;
		typedef boost::unique_lock<CacheMutex> CacheWriteLock;

		template<typename ImplementationT, bool isConst> 
		struct ImplementationTrait {
			static_assert(!isConst, "Failed to find instantiation.");
			typedef ImplementationT * Ptr;
		};

		template<typename ImplementationT> 
		struct ImplementationTrait<ImplementationT, true> {
			typedef const ImplementationT * Ptr;
		};

	public:

		boost::python::object Find(ValueType &source) const {
			const CacheReadLock lock(m_cacheMutex);
			const Cache::const_iterator it = m_cache.find(&source);
			if (it == m_cache.end()) {
				return boost::python::object();
			}
			return it->second;
		}

		template<typename Export>
		boost::python::object Get(ValueType &source) {
			
			const CacheWriteLock lock(m_cacheMutex);
			
			{
				const Cache::const_iterator it = m_cache.find(&source);
				if (it != m_cache.end()) {
					return it->second;
				}
			}

			typedef ImplementationTrait<
						Export::Implementation,
						boost::is_const<ValueType>::value>
					::Ptr
				ImplementationPtr;
			auto &impl
				= *boost::polymorphic_downcast<ImplementationPtr>(&source);
			const Export implExport(impl);
			boost::python::object object(implExport);
			
			m_cache[&source] = object;
			return object;
		
		}

	private:

		Cache m_cache;
		mutable CacheMutex m_cacheMutex;

	};

} } }
