#include <SDL2/SDL.h>
#include "Game.cpp"

// Top part is heaven, bot part is hell. arrows control both.
// add function to switch places at certain points. AND OR invert colors to avoid obsticals / fly over or 
// add obsticles that GAME OVER when touch.
// add obsticles that can be touched in order to move one rect but not the other.
// make key for one of the players that is needed to unlock other player and make it movable

// Alternative idea for game. RPG like with different weapons and upgrades. zelda/pokemon still with quests and side-quests! 
// Fundemental idea is the same, during nighttime player sleepwalks in a dreamworld with different world and enemies. Either both day and night world are visible at the same time, 
// or night world only at night, or day and night world overlapped during night. some dreams are nightmares and some are plesant.
// 

int main( int argc, char *argv[] )
{   
    const int HEIGHT = 1200; 
    const int WIDTH = 1050; 
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("Heaven & Hell", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, HEIGHT, WIDTH, SDL_RENDERER_ACCELERATED);
    if ( NULL == window )
    {
        std::cout << "Could not create window: " << SDL_GetError( ) << std::endl;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer( window, -1 , 0);
    SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );

    Game g("config.txt", renderer, HEIGHT, WIDTH);
    g.run();
        
    SDL_DestroyWindow( window );
    SDL_Quit();

    return EXIT_SUCCESS;

}