#--------------------------------------------------------------------------
# game
#--------------------------------------------------------------------------

SET(files_game
	console.h
	console.cc
	laser.h
	laser.cc
	network.h
	network.cc
	spaceship.h
	spaceship.cc
	dead_rec.h
	dead_rec.cc
	)
SOURCE_GROUP("game" FILES ${files_game})
	
SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(game STATIC ${files_game} ${files_pch})
TARGET_PCH(game ../)
ADD_DEPENDENCIES(game glew enet)
TARGET_LINK_LIBRARIES(game PUBLIC engine exts glew enet soloud)
