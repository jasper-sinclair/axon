#pragma once
#include <thread>
#include "timeman.h"

namespace uci
{
	void loop();
	void uci();
	void isready();
	void stop(std::thread& searching);
	void search(board* pos, timemanager* chrono);
}