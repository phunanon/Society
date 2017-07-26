//http://opengameart.org/content/isometric-tiles
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


sf::Image groundTexImg;
sf::Texture groundTexture;
sf::RectangleShape groundTile(sf::Vector2f(tWidth, tHeight));
const uint wTilesX = wWidth / tWidth;   //
const uint wTilesY = wHeight / tHeight; // Amount of tiles per windowful
uint cameraX = mWidth / 2 - wTilesX / 2, cameraY = mHeight / 2 - wTilesY / 2; //Player camera position

const uint mmSize = wWidth / 8; //Size of minimap on the screen
const uint mmLen = mWidth * mHeight * 4;
sf::Uint8* mm = new sf::Uint8[mmLen]; //Pixel data of the minimap
sf::Texture mmTex;
sf::RectangleShape minimap(sf::Vector2f(mWidth, mHeight));

void getTex(byte m, uint &teX, uint &teY)
{
    teX = 0; teY = 0;
    if (m > 63) { m -= 64; teY += tHeight * 2; }   //Switch to third layer
     else if (m > 31) { m -= 32; teY += tHeight; } //Switch to second layer
    teX = m * tWidth;
}

void display()
{
    window.clear(sf::Color(128, 128, 128));
  //Calculate part of map to show
    uint yStart = cameraY;
    uint xStart = cameraX;
    uint yEnd = cameraY + wTilesY;
    uint xEnd = cameraX + wTilesX;
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
            getTex(m, teX, teY);
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
  //Render minimap
    uint p = 0; //Pixel pointer
    for (uint y = 0; y < mHeight; ++y)
    {
        for (uint x = 0; x < mWidth; ++x)
        {
            if ((x == cameraX && y >= cameraY && y <= cameraY + wTilesX) || (y == cameraY && x >= cameraX && x <= cameraX + wTilesY) || (x == cameraX + wTilesY && y >= cameraY && y <= cameraY + wTilesY) || (y == cameraY + wTilesY && x >= cameraX && x <= cameraX + wTilesX)) {
                mm[p] = mm[p + 1] = mm[p + 2] = 0;
            } else {
                uint teX, teY;
                getTex(map[x][y], teX, teY); //Get the texture of this position
                sf::Color c = groundTexImg.getPixel(teX + tWidthH, teY + tHeightH); //Sample its centre colour
                mm[p]     = c.r;
                mm[p + 1] = c.g;
                mm[p + 2] = c.b;
                mm[p + 3] = 255;
            }
            p += 4;
        }
    }
    //Draw minimap
    mmTex.update(mm);
    minimap.setTexture(&mmTex);
    window.draw(minimap);
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
        if (cameraX > 1) { cameraX -= 1; } else { cameraX = 0; }        
        if (cameraY > 1) { cameraY -= 1; } else { cameraY = 0; }
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) { //Move camera right (SE)
        if (cameraX < mWidth - wTilesX) { cameraX += 1; } else { cameraX = mWidth - wTilesX; }
        if (cameraY < mHeight - wTilesY) { cameraY += 1; } else { cameraY = mHeight - wTilesY; }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) { //Move camera up (NE)
        if (cameraX < mWidth - wTilesX) { cameraX += 1; } else { cameraX = mWidth - wTilesX; }
        if (cameraY > 1) { cameraY -= 1; } else { cameraY = 0; }
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) { //Move camera down (SW)
        if (cameraX > 1) { cameraX -= 1; } else { cameraX = 0; }
        if (cameraY < mHeight - wTilesY) { cameraY += 1; } else { cameraY = mHeight - wTilesY; }
    }
}

//Map 'pastes'
const byte pSize = 4;
const byte pLen = pow(pSize, 2);
const byte mapPastes[16][pLen] = { //Pre-designed map parts
    { 0,  36, 0,  0,
      34, 33, 37, 0,
      0,  35, 0,  0,
      0,  0,  0,  0 },
    { 38, 36, 40, 0,
      34, 33, 37, 0,
      39, 35, 41, 0,
      0,  0,  0,  0 },
    { 69, 67, 71, 0,
      65, 64, 68, 0,
      70, 66, 72, 0,
      0,  0,  0,  0 }
};

int main()
{
  //Load window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  //Load textures & tiles
    //Ground
    if (!groundTexImg.loadFromFile("textures/ground.png")) {
        std::cout << "Couldn't load textures/ground.png" << std::endl;
    }
    groundTexture.loadFromImage(groundTexImg);
    groundTile.setTexture(&groundTexture);
    groundTexture.setSmooth(true);
    //Minimap
    mmTex.create(mWidth, mHeight);
    minimap.scale(mmSize / mWidth, mmSize/ mHeight);
    minimap.setPosition(sf::Vector2f(wWidth - mmSize * 1.4, mmSize / 1.4));
    minimap.setRotation(-45);
  //Load map
    memset(map, 0, sizeof(map)); //Load memory
    for (uint y = 0; y < mHeight; ++y) { //Completely random
        for (uint x = 0; x < mWidth; ++x) {
            byte* m = &map[x][y];
            *m = 32; //Start with grass
            if (rb(.05)) { *m = 50; } //Random sand patches
            //if (x == 0 || y == 0 || x == mWidth - 1 || y == mHeight - 1) { *m = 33; }
        }
    }
    for (uint y = 0; y < mHeight; ++y) { //Pastes
        for (uint x = 0; x < mWidth; ++x) {
            if (rb(.01)) { //Should we put a random paste?
                const byte* p = &mapPastes[ri(0, 2)][0]; //Select a random paste
                byte py = -1;
                for (byte d = 0; d < pLen; ++d) { //For each paste data point
                    byte px = d % 4;
                    if (!px) { ++py; } //Next line
                    if (x + px < mWidth && y + py < mHeight) {
                        if (p[d]) { map[x + px][y + py] = p[d]; }
                    }
                }
            }
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
