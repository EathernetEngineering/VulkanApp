#include "pch.h"
#include <iostream>

#include "Application.hpp"

int main(int argc, char** argv) {
	
	CEE::Application* pApp = new CEE::Application(argc, argv);
	
	int returnVal = pApp->Run();
	
	delete pApp;
	
	return returnVal;
}
