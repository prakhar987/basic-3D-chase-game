Game: game.cpp glad.c
	  g++ -o Game game.cpp glad.c -lGL -lglfw -ldl -lftgl -lSOIL -I/usr/local/include -I/usr/local/include/freetype2 -L/usr/lib

	



clean:
	rm Game
