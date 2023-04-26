#include "config.h"
#include "server_app.h"

int
main(int argc, const char** argv)
{
	ServerApp app;
	if (app.Open())
	{
		app.Run();
		app.Close();
	}
	app.Exit();
}