#include "application.h"

#include <iostream>
#include <stdexcept>
#include <functional>
#include <conio.h>



int main()
{
	Application app;
	
	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		_getch();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}