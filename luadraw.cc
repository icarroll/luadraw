#include <iostream>
#include <fstream>
#include <string>

#include <glm/glm.hpp>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

extern "C" {
#include <SDL.h>
#include <gl/glew.h>
#include <SDL_opengl.h>
}

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

string window_name;
SDL_Window * gWindow = NULL;
SDL_GLContext gContext;

void die(string message) {
    cout << message << endl;
    exit(1);
}

void init_graphics() {
    // init SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) die("SDL");
    if (! SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) die("texture");

    // init SDL GL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);

    gWindow = SDL_CreateWindow(window_name.c_str(), SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH, SCREEN_HEIGHT,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (! gWindow) die("window");

    gContext = SDL_GL_CreateContext(gWindow);
    if (! gContext) die("gl context");

    // init GLEW
    glewExperimental = GL_TRUE; 
    if (glewInit() != GLEW_OK) die("glew");

    // GL viewport
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glEnable(GL_DEPTH_TEST);
}

void close_graphics() {
    //TODO close OpenGL

    SDL_DestroyWindow(gWindow);
    gWindow = NULL;

    SDL_Quit();
}

void luaD_setcolorfields(lua_State * L, glm::vec3 color) {
    lua_pushstring(L, "red");
    lua_pushnumber(L, color.r);
    lua_settable(L, -3);

    lua_pushstring(L, "green");
    lua_pushnumber(L, color.g);
    lua_settable(L, -3);

    lua_pushstring(L, "blue");
    lua_pushnumber(L, color.b);
    lua_settable(L, -3);
}

glm::vec3 luaD_getcolorfields(lua_State * L) {
    glm::vec3 color = {};

    lua_getfield(L, -1, "red");
    color.r = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "green");
    color.g = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "blue");
    color.b = lua_tonumber(L, -1);
    lua_pop(L, 1);

    return color;
}

glm::vec3 bgcolor;

lua_State * L;
void read_lua() {
    L = luaL_newstate();
    if (! L) die("can't lua");
    luaL_openlibs(L);

    // set default lua variables
    lua_pushstring(L, "Drawing with Lua!");
    lua_setglobal(L, "window_name");

    lua_newtable(L);
    luaD_setcolorfields(L, {1.0, 0.0, 1.0});
    lua_setglobal(L, "bgcolor");

    int status;

    // read in main file
    status = luaL_loadfile(L, "main.lua");
    if (status != LUA_OK) die(lua_tostring(L, -1));

    // execute main file
    status = lua_pcall(L, 0, 0, 0);
    if (status != LUA_OK) die(lua_tostring(L, -1));

    // read altered lua variables
    lua_getglobal(L, "window_name");
    window_name = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "bgcolor");
    bgcolor = luaD_getcolorfields(L);
    lua_pop(L, 1);
}

void draw_scene() {
    // fill with background color
    glClearColor(bgcolor.r, bgcolor.g, bgcolor.b, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

unsigned int FRAME_TICK;

uint32_t timer_callback(uint32_t interval, void * param) {
    SDL_Event e;
    e.type = FRAME_TICK;
    SDL_PushEvent(& e);

    return interval;
}

int main(int nargs, char * args[])
{
    read_lua();

    init_graphics();

    // timer tick every 20msec
    FRAME_TICK = SDL_RegisterEvents(1);
    SDL_AddTimer(20, timer_callback, NULL);

    bool done = false;
    while (! done)
    {
        SDL_Event e;
        SDL_WaitEvent(& e); //TODO check for error

        if (e.type == SDL_QUIT) done = true;
        else if (e.type == FRAME_TICK) {
	    //TODO clear all FRAME_TICK from queue so they don't pile up

            draw_scene();

            SDL_GL_SwapWindow(gWindow);
        }
    }

    close_graphics();

    lua_close(L);

    return 0;
}
