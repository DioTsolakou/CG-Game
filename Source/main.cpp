#include "SDL2\SDL.h"
#include <iostream>
#include <chrono>
#include "GLEW\glew.h"
#include "Renderer.h"
#include <thread>         // std::this_thread::sleep_for

#define FPS_INTERVAL 1.0 //seconds.

using namespace std;

//Screen attributes
SDL_Window* window;

//OpenGL context 
SDL_GLContext gContext;
const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
Uint32 fps_lasttime = SDL_GetTicks(); //the last recorded time.
Uint32 fps_current; //the current FPS.
Uint32 fps_frames = 0; //frames passed since the last recorded fps.

//Event handler
SDL_Event event;

Renderer* renderer = nullptr;

void clean_up()
{
	delete renderer;

	SDL_GL_DeleteContext(gContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

// initialize SDL and OpenGL
bool init()
{
	//Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		return false;
	}

	// use Double Buffering
	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0)
		cout << "Error: No double buffering" << endl;

	// set OpenGL Version (3.3)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	// set relative mouse movement mode
	SDL_SetRelativeMouseMode(SDL_TRUE);

	// Create Window
	window = SDL_CreateWindow("CG Game",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	// sets proper fullscreen
	//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	if (window == NULL)
	{
		printf("Could not create window: %s\n", SDL_GetError());
		return false;
	}

	//Create OpenGL context
	gContext = SDL_GL_CreateContext(window);
	if (gContext == NULL)
	{
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	// Disable Vsync
	if (SDL_GL_SetSwapInterval(0) == -1)
		printf("Warning: Unable to disable VSync! SDL Error: %s\n", SDL_GetError());

	// Initialize GLEW
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printf("Error loading GLEW\n");
		return false;
	}
	// some versions of glew may cause an opengl error in initialization
	glGetError();

	renderer = new Renderer();
	bool engine_initialized = renderer->Init(SCREEN_WIDTH, SCREEN_HEIGHT);

	return engine_initialized;
}

void keyboardInput(bool &quit)
{
	if (event.type == SDL_KEYDOWN)
	{
		if (event.key.keysym.sym == SDLK_ESCAPE) quit = true;
		if (event.key.keysym.sym == SDLK_w || event.key.keysym.sym == SDLK_UP)
		{
			renderer->CameraMoveForward(true);
		}
		else if (event.key.keysym.sym == SDLK_s || event.key.keysym.sym == SDLK_DOWN)
		{
			renderer->CameraMoveBackWard(true);
		}
		else if (event.key.keysym.sym == SDLK_a || event.key.keysym.sym == SDLK_LEFT)
		{
			renderer->CameraMoveLeft(true);
		}
		else if (event.key.keysym.sym == SDLK_d || event.key.keysym.sym == SDLK_RIGHT)
		{
			renderer->CameraMoveRight(true);
		}
		/*else if (event.key.keysym.sym == SDLK_q)
		{
			renderer->CameraRollLeft(true);
		}
		else if (event.key.keysym.sym == SDLK_e)
		{
			renderer->CameraRollRight(true);
		}*/
		else if (event.key.keysym.sym == SDLK_r) renderer->ReloadShaders();
	}
	else if (event.type == SDL_KEYUP)
	{
		if (event.key.keysym.sym == SDLK_w || event.key.keysym.sym == SDLK_UP)
		{
			renderer->CameraMoveForward(false);
		}
		else if (event.key.keysym.sym == SDLK_s || event.key.keysym.sym == SDLK_DOWN)
		{
			renderer->CameraMoveBackWard(false);
		}
		else if (event.key.keysym.sym == SDLK_a || event.key.keysym.sym == SDLK_LEFT)
		{
			renderer->CameraMoveLeft(false);
		}
		else if (event.key.keysym.sym == SDLK_d || event.key.keysym.sym == SDLK_RIGHT)
		{
			renderer->CameraMoveRight(false);
		}
		/*else if (event.key.keysym.sym == SDLK_q)
		{
			renderer->CameraRollLeft(false);
		}
		else if (event.key.keysym.sym == SDLK_e)
		{
			renderer->CameraRollRight(false);
		}*/
	}
}

void mouseInput(bool &mouse_button_pressed, bool &right_mouse_button_pressed, glm::vec2 prev_mouse_position)
{
	if (event.type == SDL_MOUSEMOTION)
	{
		int x = event.motion.xrel;
		int y = event.motion.yrel;

		renderer->CameraLook(glm::vec2(prev_mouse_position - glm::vec2(x, y)));
		prev_mouse_position = glm::vec2(x, y);
	}
	else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
	{
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			// to do : call function to shoot

			int x = event.button.x;
			int y = event.button.y;
			mouse_button_pressed = (event.type == SDL_MOUSEBUTTONDOWN);
			prev_mouse_position = glm::vec2(x, y);
		}
		else if (event.button.button == SDL_BUTTON_RIGHT)
		{
			int x = event.button.x;
			int y = event.button.y;
			right_mouse_button_pressed = (event.type == SDL_MOUSEBUTTONDOWN);
		}
	}
}

int main(int argc, char* argv[])
{
	//Initialize SDL, glew, engine
	if (init() == false)
	{
		system("pause");
		return EXIT_FAILURE;
	}

	//Quit flag
	bool quit = false;
	bool mouse_button_pressed = false;
	bool right_mouse_button_pressed = false;
	glm::vec2 prev_mouse_position(0);

	auto simulation_start = chrono::steady_clock::now();

	// Wait for user exit
	while (quit == false)
	{
		// While there are events to handle
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				quit = true;
			}
			else if (event.type == SDL_WINDOWEVENT)
			{
				if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					renderer->ResizeBuffers(event.window.data1, event.window.data2);
				}
			}
			keyboardInput(quit);
			mouseInput(mouse_button_pressed, right_mouse_button_pressed, prev_mouse_position);
		}

		fps_frames++;
		if (fps_lasttime < SDL_GetTicks() - FPS_INTERVAL * 1000)
		{
			fps_lasttime = SDL_GetTicks();
			fps_current = fps_frames;
			fps_frames = 0;
		}

		cout << "fps: " << fps_current << endl;

		//SDL_WarpMouseInWindow(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2); might be unnecessary

		// Compute the ellapsed time
		auto simulation_end = chrono::steady_clock::now();

		float dt = chrono::duration <float>(simulation_end - simulation_start).count(); // in seconds

		simulation_start = chrono::steady_clock::now();

		// Update
		renderer->Update(dt);

		// Draw
		renderer->Render();

		//Update screen (swap buffer for double buffering)
		SDL_GL_SwapWindow(window);
	}

	//Clean up
	clean_up();

	return 0;
}