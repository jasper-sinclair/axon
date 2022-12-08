#pragma once
#include <thread>
#include "timeman.h"

namespace uci
{
	void isready();
	void loop();
	void search(board* pos, timemanager* chrono);
	void stop(std::thread& searching);
	void uci();
}
