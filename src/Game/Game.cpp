#include "Game.h"
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

glm::vec2 playerPosition;
glm::vec2 playerVelocity;

void Game::Setup(){
  playerPosition = glm::vec2(10.0, 20.0);
  playerVelocity = glm::vec2(100.0, 20.0);
}

void Game::Update(){
  if(FPS_LIMIT > 0){
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecsPreviousFrame);
    if(timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME){
      SDL_Delay(timeToWait);
    }
  }

  double deltaTime = (SDL_GetTicks() - millisecsPreviousFrame) / 1000.0;

  millisecsPreviousFrame = SDL_GetTicks();

  playerPosition.x += playerVelocity.x * deltaTime;
  playerPosition.y += playerVelocity.y * deltaTime;
}


void Game::Render(){
  SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
  SDL_RenderClear(renderer);

  SDL_Surface* surface = IMG_Load("./assets/images/tank-tiger-right.png");
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);

  SDL_Rect dstRect = { 
    static_cast<int>(playerPosition.x),
    static_cast<int>(playerPosition.y),
    32,
    32
  };

  SDL_RenderCopy(renderer, texture, NULL, &dstRect);
  SDL_DestroyTexture(texture);

  SDL_RenderPresent(renderer);
}

Game::Game(){
  spdlog::info("Game constructor called!");
}

Game::~Game(){
  spdlog::info("Game destractor called!");
}

void Game::Initialize(){
  isRunning = false;

  if(SDL_Init(SDL_INIT_EVERYTHING) != 0){
    spdlog::critical("Error initializing SDL.");
    return;
  }

  SDL_DisplayMode displayMode;
  SDL_GetCurrentDisplayMode(0, &displayMode);
  windowWidth = 800;//displayMode.w;
  windowHeight = 600;//displayMode.h;
  
  window = SDL_CreateWindow(
    NULL,
    SDL_WINDOWPOS_CENTERED, 
    SDL_WINDOWPOS_CENTERED,
    windowWidth,
    windowHeight,
    SDL_WINDOW_BORDERLESS
  );

  if(!window){
    spdlog::critical("Error creating SDL Window.");
    return;
  }

  renderer = SDL_CreateRenderer(
    window,
    -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
  );

  if(!renderer){
    spdlog::critical("Error creating SDL renderer.");
    return;
  }

  if(FULLSCREEN == true){
    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
  }

  isRunning = true;
}

void Game::Run(){
  Setup();
  while (isRunning)
  {
    ProcessInput();
    Update();
    Render();
  }
}

void Game::ProcessInput(){
  SDL_Event sdlEvent;
  while(SDL_PollEvent(&sdlEvent)){
    switch (sdlEvent.type)
    {
    case SDL_QUIT:
      isRunning = false;
      break;
    case SDL_KEYDOWN:
      if(sdlEvent.key.keysym.sym == SDLK_ESCAPE){
        isRunning = false;
      }
      break;
    }
  }
}

void Game::Destroy(){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}