#include "main.h"
#include "tuto.h"

int main(int argc, char* argv[])
{
	if (argc < 4)
		serveur();
	else
		client(argc, argv);

	return EXIT_SUCCESS;
}