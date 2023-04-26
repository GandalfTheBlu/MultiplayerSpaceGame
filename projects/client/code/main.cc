#include "config.h"
#include "client_app.h"

int
main(int argc, const char** argv)
{
	ClientApp app;
	if (app.Open())
	{
		app.Run();
		app.Close();
	}
	app.Exit();
}