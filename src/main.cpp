#include "engine.h"
#include "uci.h"
#include "common.h"

int main()
{
	std::cout << eng_name << " " << version << " " << platform << std::endl;

	engine::init_rand();
	engine::init_movegen();
	engine::init_eval();

	uci::loop();

	return 0;
}
