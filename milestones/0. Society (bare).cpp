//http://opengameart.org/content/isometric-tiles
//http://clintbellanger.net/articles/isometric_math/
#include <SFML/Graphics.hpp> //For SFML graphics
#include <SFML/Audio.hpp> //For SFML audio
#include <cstring> //For strings
#include <math.h> //For mathematical constants (pi)
#include <random> //For random generation
#include <fstream> //IO
#include <sstream> //IO to String
#include <iostream> //Terminal
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;

#define wWidth 1024 //
#define wHeight 512 // Window dimensions
#define tWidth 64   //
#define tHeight 32  // Texture dimensions
const uint tWidthH = tWidth / 2;   //
const uint tHeightH = tHeight / 2; // Texture half dimensions
#define mWidth 128  //
#define mHeight 128 // Map dimensions

sf::RenderWindow window(sf::VideoMode(wWidth, wHeight), "Society");

byte map[mWidth][mHeight]; //Map array, size of mWidth * mHeight

//===================================
//GENERATORS
//===================================
std::mt19937 gen(time(NULL));
inline bool rb(float dist = 0.5f) //Random boolean
{
    std::bernoulli_distribution b_dist(dist); //Random boolean https://msdn.microsoft.com/en-us/library/bb982365.aspx
    return b_dist(gen);
}

inline double rf(double min, double max)
{
    std::uniform_real_distribution<double> rd_dist(min, max); //http://www.cplusplus.com/reference/random/uniform_real_distribution/uniform_real_distribution/ & /operator()/
    return rd_dist(gen);
}

inline int ri(int min, int max)
{
    std::uniform_int_distribution<int> id_dist(min, max); //http://www.cplusplus.com/reference/random/uniform_int_distribution/ & /operator()/
    return id_dist(gen);
}
//===================================

void gameLoop()
{
    
}


sf::Texture groundTexture;
sf::RectangleShape groundTile(sf::Vector2f(tWidth, tHeight));
uint cameraX = 0, cameraY = 0; //Player camera position (doubled)
const uint wTilesX = wWidth / tWidth;   //
const uint wTilesY = wHeight / tHeight; // Amount of tiles per windowful

void display()
{
    window.clear(sf::Color(128, 128, 128));
  //Calculate part of map to show
    uint yStart = cameraY / 2;
    uint xStart = cameraX / 2;
    uint yEnd = cameraY / 2 + wTilesY;
    uint xEnd = cameraX / 2 + wTilesX;
  //Go through the map, and draw the ground, based on camera location
    int isoOffX = 0, isoOffY = 0;
    uint scrY = wTilesY;
    uint scrX = -1;
    for (uint y = yStart; y < yEnd; ++y) {
        isoOffX = scrX * tWidthH;
        isoOffY = scrY * tHeightH;
        for (uint x = xStart; x < xEnd; ++x) {
          //Calculate texture offset
            uint teX = 0, teY = 0;
            byte m = map[x][y];
            if (m > 31) { m -= 32; teY += tHeight; } //Switch to second layer
            teX = m * tWidth;
            groundTile.setTextureRect(sf::IntRect(teX, teY, tWidth, tHeight));
          //Calculate draw offset
            int pX, pY;
            isoOffX += tWidthH;
            isoOffY -= tHeightH;
            pX = isoOffX;
            pY = isoOffY;
          //Draw
            groundTile.setPosition(pX, pY);
            window.draw(groundTile);
        }
        ++scrY;
        ++scrX;
    }
  //Draw
    window.display();
}

void pollInput()
{
  //Check if we want to exit
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
    }
  //Check the keyboard
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) { //Move camera left (NW)
        if (cameraX > 2) { cameraX -= 2; } else { cameraX = 0; }        
        if (cameraY > 2) { cameraY -= 2; } else { cameraY = 0; }
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { //Move camera right (SE)
        if (cameraX / 2 < mWidth - wTilesX) { cameraX += 2; } else { cameraX = (mWidth - wTilesX) * 2; }
        if (cameraY / 2 < mHeight - wTilesY) { cameraY += 2; } else { cameraY = (mHeight - wTilesY) * 2; }
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) { //Move camera up (NE)
        if (cameraX / 2 < mWidth - wTilesX) { cameraX += 2; } else { cameraX = (mWidth - wTilesX) * 2; }
        if (cameraY > 2) { cameraY -= 2; } else { cameraY = 0; }
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) { //Move camera down (SW)
        if (cameraX > 2) { cameraX -= 2; } else { cameraX = 0; }
        if (cameraY / 2 < mHeight - wTilesY) { cameraY += 2; } else { cameraY = (mHeight - wTilesY) * 2; }
    }
}

int main()
{
  //Load window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.setPosition(sf::Vector2i((desktop.width / 2) - (wWidth / 2), (desktop.height / 2) - (wHeight / 2)));
  //Load textures & tiles
    if (!groundTexture.loadFromFile("textures/ground.png")) {
        std::cout << "Couldn't load textures/ground.png" << std::endl;
    }
    groundTile.setTexture(&groundTexture);
    groundTexture.setSmooth(true);
  //Load map
    memset(map, 0, sizeof(map));
    for (uint y = 0; y < mHeight; ++y) {
        for (uint x = 0; x < mWidth; ++x) {
            map[x][y] = ri(0, 48);
            if (x == 0 || y == 0 || x == mWidth - 1 || y == mHeight - 1) { map[x][y] = 33; }
        }
    }
  //Start loop
    while (window.isOpen()) {
        pollInput();
        gameLoop();
        display();
        sf::sleep(sf::milliseconds(32));
    }
}
