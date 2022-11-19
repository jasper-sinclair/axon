#include "timeman.h"

int timer::elapsed() const
{
	return std::chrono::duration_cast<std::chrono::duration<int, std::ratio<1, 1000>>>
		(std::chrono::system_clock::now() - start_point_).count();
}

int timemanager::get_movetime(const int turn)
{
	if (!movetime_is_set)
	{
		movetime = time[turn] / moves_to_go + incr[turn];
		movetime -= movetime / 20 + incr[turn] / moves_to_go;
	}
	return movetime;
}

void timemanager::set_movetime(const int new_time)
{
	movetime = new_time - new_time / 20;
	movetime_is_set = true;
}

void timer::start()
{
	start_point_ = std::chrono::system_clock::now();
}
