#include <iostream>
#include "engine.h"
#include "uci.h"

int main()
{
	std::cout << eng_name << " " << version << " " << platform << std::endl;
	engine::init_rand();
	engine::init_movegen();
	engine::init_eval();
	engine::init_hash(engine::hash_size);

	uci::loop();

	return 0;
}
