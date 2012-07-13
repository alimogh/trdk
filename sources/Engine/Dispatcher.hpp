/**************************************************************************
 *   Created: 2012/07/09 16:09:53
 *    Author: Eugene V. Palchukovsky
 *    E-mail: eugene@palchukovsky.com
 * -------------------------------------------------------------------
 *   Project: Trading Robot
 **************************************************************************/

#pragma once

class Algo;
class Options;

class Dispatcher : private boost::noncopyable {

private:

	class AlgoState;
	class Notifier;
	class Slots;

public:

	Dispatcher(boost::shared_ptr<const Options>);
	~Dispatcher();

public:

	void Register(boost::shared_ptr<Algo>);
	void CloseAll();

public:

	void Start();
	void Stop();

private:

	
	boost::shared_ptr<Notifier> m_notifier;
	std::unique_ptr<Slots> m_slots;
	
};
