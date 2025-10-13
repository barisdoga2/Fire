#include "EasyPlayground.hpp"



int main()
{
	if (EasyDisplay display({1024,768}); display.Init())
	{
		if (EasyPlayground playground(display); playground.Init())
		{
			while (playground.OneLoop());
		}
	}
	return 0;
}

