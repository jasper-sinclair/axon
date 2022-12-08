#pragma once
#include <chrono>

class timemanager
{
public:
	timemanager() : time{0}, incr{0}, moves_to_go{50}
	{
	}

	bool movetime_is_set{};
	int get_movetime(int turn);
	int incr[2];
	int moves_to_go;
	int movetime{};
	int time[2];
	void set_movetime(int new_time);
};

class timer
{
	std::chrono::time_point<std::chrono::system_clock> start_point_;

public:
	void start();
	timer() { start(); }
	[[nodiscard]] int elapsed() const;
};
