//http://opengameart.org/content/isometric-tiles
//http://opengameart.org/users/andrettin
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

#define T_GRASS             32
#define T_WATER             64
#define T_SAND              96
#define T_SAND_PATCH        49
#define T_GRASS_WATER_PATCH 81
#define T_SAND_WATER_PATCH  113

#define GEN_RIVERS_AMOUNT   16
#define GEN_RIVER_MIN_LEN   64
#define GEN_RIVER_MAX_LEN   128
#define GEN_RIVER_WIND_BIAS 6
#define GEN_RIVER_BENDINESS 10

#define wWidth 1024  //
#define wHeight 512  // Window dimensions
const uint wWidthH = wWidth / 2;
const uint wHeightH = wHeight / 2;
#define gTWidth 64   //
#define gTHeight 32  // Ground texture dimensions
#define sTWidth 64   //
#define sTHeight 64  // Sprite texture dimensions
#define eTWidth 32   //
#define eTHeight 32  // Entity texture dimensions
const uint gTWidthH = gTWidth / 2;   //
const uint gTHeightH = gTHeight / 2; // Ground texture half dimensions
const uint sTWidthH = sTWidth / 2;   //
const uint sTHeightH = sTHeight / 2; // Sprite texture half dimensions
const uint eTWidthH = eTWidth / 2;   //
const uint eTHeightH = eTHeight / 2; // Entity texture half dimensions
#define mWidth 512  //
#define mHeight 512 // biome dimensions

#define MAXENTITIES 1024

sf::RenderWindow window(sf::VideoMode(wWidth, wHeight), "Society");
ulong gtime;
double frameTime;
double FPS;

byte **biome = new byte*[mWidth]; //biome array
byte **sprites = new byte*[mWidth]; //sprite array

//===================================
//GENERATORS
//===================================
std::mt19937 gen(time(NULL));
bool rb(float dist = 0.5f) //Random boolean
{
    std::bernoulli_distribution b_dist(dist); //Random boolean https://msdn.microsoft.com/en-us/library/bb982365.aspx
    return b_dist(gen);
}

double rf(double min, double max)
{
    std::uniform_real_distribution<double> rd_dist(min, max); //http://www.cplusplus.com/reference/random/uniform_real_distribution/uniform_real_distribution/ & /operator()/
    return rd_dist(gen);
}

int ri(int min, int max)
{
    std::uniform_int_distribution<int> id_dist(min, max); //http://www.cplusplus.com/reference/random/uniform_int_distribution/ & /operator()/
    return id_dist(gen);
}

double rf_nd(double average, double spread)
{
    std::normal_distribution<double> nd_dist(average, spread);
    return nd_dist(gen);
}
//===================================

//===================================
//Mathematical functions
//===================================
void normaliseAng(double &ang)
{
    if (ang >= 360) { ang -= 360; }
     else if (ang < 0) { ang += 360; }
}

double vecToAng(double dirX, double dirY) //Takes a vector, and gives the angle where {0, -1} is North, {1, 0} is East
{
    double ang = 90 + atan2(dirY, dirX) * (180 / M_PI); //Converted from radians with 180 / pi
    normaliseAng(ang);
    return ang;
}

void angToVec(double rot, double &dirX, double &dirY) //Takes an angle, and gives the vector where 0' is {0, -1}, 90' is {1, 0}
{
    double rRot = rot * (M_PI / 180); //Convert to radians
    dirX = sin(rRot);
    dirY = -cos(rRot);
}

void targToVec(double tarX, double tarY, double ourX, double ourY, double &dirX, double &dirY)
{
  //Find the difference in coordinates
    double diffX = tarX - ourX;
    double diffY = tarY - ourY;
    //Finding the biggest direction, absolute
    bool xIsBiggest = fabs(diffX) > fabs(diffY);
    //The biggest direction becomes 1 (in the direction)
    double yDir = (diffY > 0 ? 1 : -1);
    double xDir = (diffX > 0 ? 1 : -1);
    if (xIsBiggest)
      { dirX = xDir; }
     else
      { dirY = yDir; }
    //The smallest direction becomes a fraction of 1 (in the direction, introduced from * diff)
    if (xIsBiggest)
      { dirY = (1.0 / fabs(diffX)) * diffY; }
     else
      { dirX = (1.0 / fabs(diffY)) * diffX; }
}
//===================================

//===================================
//Map functions
//===================================
bool inBounds(double x, double y)
{
    return (x >= 0 && y >= 0 && x < mWidth && y < mHeight);
}

void randMapPos(uint &x, uint &y)
{
    x = ri(0, mWidth);
    y = ri(0, mHeight);
}
//===================================


//===================================
//ENTITY
//===================================
class Entity
{
  public:
    bool inited = false;
    double X; //X-position on the map
    double Y; //Y-position on the map
    byte type; //Type of entity: 0 Worker
    Entity(double, double, byte, double, double); //Constructor
    Entity(); //Blank constructor
    ~Entity(); //Destructor
    void getTex(uint&, uint&); //Return the texture offset within its sheet
  private:
    double dX; //
    double dY; // Direction vector
    byte frame = 0; //Frame of animation
    void changeEMap(bool); //Set the entity on the entityMap
    void setp(double, double);
};

Entity::Entity(double x, double y, byte type, double dX, double dY)
{
    this->inited = true;
    this->type = type;
    this->setp(x, y);
    this->dX = dX;
    this->dY = dY;
}

Entity::Entity() {  }

Entity::~Entity() {  }

Entity* entity[MAXENTITIES]; //Array of all Entities - ensure you use Entities[c] != NULL to check an Entity actually exists in the array
uint entities = 0; //Amount of sprites
Entity* entityMap[mWidth][mHeight];

void addEntity(double pX, double pY, byte type, double dX = 0, double dY = 1)
{
    if (entities < MAXENTITIES) //Can we add any more Entities?
    {
        uint tryPos = 0;
        while (entity[tryPos] != NULL) //Search for a free position in the Entity array
        {
            ++tryPos;
        }
        entity[tryPos] = new Entity(pX, pY, type, dX, dY);
        ++entities;
    }
}

void Entity::changeEMap(bool set)
{
    if (set) {
        entityMap[(uint)X][(uint)Y] = this;
    } else {
        entityMap[(uint)X][(uint)Y] = new Entity();
    }
}

void Entity::setp(double x, double y)
{
    changeEMap(false);
    X = x;
    Y = y;
    changeEMap(true);
}

void Entity::getTex(uint &teX, uint &teY)
{
  //Calculate the angle to show the sprite
    double eRot = vecToAng(dX, dY); //Entity's rotation
    byte z = eRot / 45; //Every 45 degrees, the next texture in the sheet is used (eg. 90 becomes 2)
    //Select the texture in the sheet, where the X-axis is the rotation, and the Y-axis is the animation frame
    teX = z * eTWidth;
    teY = eTHeight * frame;
}
//===================================




void gameLoop()
{
    
}


sf::Image groundTexImg;
sf::Texture groundTexture;
sf::RectangleShape groundTile(sf::Vector2f(gTWidth, gTHeight));
sf::Image spriteTexImg;
sf::Texture spriteTexture;
sf::RectangleShape spriteTile(sf::Vector2f(sTWidth, sTHeight));
std::vector<sf::Texture*> entityTextures;
sf::RectangleShape entityTile(sf::Vector2f(eTWidth, eTHeight));

const uint wTilesX = wWidth / gTWidth;   //
const uint wTilesY = wHeight / gTHeight; // Amount of tiles per windowful
uint cameraX = mWidth / 2 - wTilesX / 2,
     cameraY = mHeight / 2 - wTilesY / 2; //Player camera position

const uint mmSize = wWidth / 8; //Size of minimap on the screen
const uint mmDiagWidth = sqrt(pow(mmSize, 2) + pow(mmSize, 2)); //Width of minimap rotated 45deg
const uint mmLen = mWidth * mHeight * 4;
sf::Uint8* mm = new sf::Uint8[mmLen]; //Pixel data of the minimap
sf::Texture mmTex;
sf::RectangleShape minimap(sf::Vector2f(mWidth, mHeight));

sf::Font arialFont; //For outputting text
std::string outText = ""; //Cache of text output
sf::Text displayText; //Display text

void getTex(byte m, uint &teX, uint &teY)
{
    teX = 0; teY = 0;
    if (m > 95) { m -= 96; teY += gTHeight * 3; }   //Switch to fourth layer
    if (m > 63) { m -= 64; teY += gTHeight * 2; }   //Switch to third layer
     else if (m > 31) { m -= 32; teY += gTHeight; } //Switch to second layer
    teX = m * gTWidth;
}

void display()
{
    window.clear(sf::Color(128, 128, 128));
  //Calculate part of biome to show
    uint yStart = cameraY;
    uint xStart = cameraX;
    uint yEnd = cameraY + wTilesY;
    uint xEnd = cameraX + wTilesX;
  //Go through the biome/sprites, and draw them based on camera location
    int isoOffX = 0, isoOffY = 0;
    uint scrY = wTilesY;
    uint scrX = -1;
    for (uint y = yStart; y < yEnd; ++y) {
        isoOffX = scrX * gTWidthH;
        isoOffY = scrY * gTHeightH;
        for (uint x = xStart; x < xEnd; ++x) {
          //Calculate texture offset
            uint teX = 0, teY = 0;
            byte m = biome[x][y];
            getTex(m, teX, teY);
            groundTile.setTextureRect(sf::IntRect(teX, teY, gTWidth, gTHeight));
          //Calculate draw offset
            int pX, pY;
            isoOffX += gTWidthH;
            isoOffY -= gTHeightH;
            pX = isoOffX;
            pY = isoOffY;
          //Draw biomes
            groundTile.setPosition(pX, pY);
            window.draw(groundTile);
          //Draw entities (at a y offset, in order to not overlap with ground rendering)
            if (y > 0) {
                --y;
                if (entityMap[x][y]->inited) { //If there's an entity here
                    entityTile.setTexture(entityTextures[entityMap[x][y]->type]);
                    uint teX, teY;
                    entityMap[x][y]->getTex(teX, teY);
                    entityTile.setTextureRect(sf::IntRect(teX, teY, eTWidth, eTHeight));
                    entityTile.setPosition(pX - eTWidthH, pY - eTHeight);
                    window.draw(entityTile);
                }
                ++y;
            }
          //Draw sprites (at a y offset, in order to not overlap with ground rendering)
            if (y > 0) {
                --y;
                if (sprites[x][y]) { //If there's a sprite here
                    pY -= sTHeight - (sTHeightH / 2); //Adjust the sprite's position on the screen, as we are at an offset
                    if (pY + sTHeightH >= abs(pX - wWidthH) / 2) { //Only allow the tops of the sprites to show off-map
                        pX -= sTWidthH;
                        spriteTile.setTextureRect(sf::IntRect(sprites[x][y], 0, sTWidth, sTHeight));
                        spriteTile.setPosition(pX, pY);
                        window.draw(spriteTile);
                    }
                }
                ++y;
            }
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
                getTex(biome[x][y], teX, teY); //Get the texture of this position
                sf::Color c = groundTexImg.getPixel(teX + gTWidthH, teY + gTHeightH); //Sample its centre colour
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
  //Render variables and other text
    outText = "";
    outText += "FPS: " + std::to_string((uint)FPS);
//    outText += "\nx: " + std::to_string(cameraX) + " y: " + std::to_string(cameraY);
    //Draw
    displayText.setString(outText);
    window.draw(displayText);
  //Display
    window.display();
}

ulong prevClick;
void pollInput()
{
  //Check if we want to exit
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
    }
  //Monitor mouse
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && prevClick < gtime - 10) { //LEFT CLICK
        prevClick = gtime;
      //Detect where the click has been placed on the map
        
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

//biome 'pastes'
const byte pSize = 4;
const byte pLen = pow(pSize, 2);
const byte biomePastes[16][pLen] = { //Pre-designed biome parts
    { 0,  35, 0,  0,    //3x3 symmetrical sandbank, no corners
      33, 96, 36, 0,
      0,  34, 0,  0,
      0,  0,  0,  0 },
    { 37, 35, 39, 0,    //3x3 symmetrical sandbank w/ corners
      33, 96, 36, 0,
      38, 34, 40, 0,
      0,  0,  0,  0 },
    { 37, 35, 39, 0,    //3x3 sand doughnut
      33, 54, 36, 0,
      38, 34, 40, 0,
      0,  0,  0,  0 },
    { 37, 35, 39, 0,    //3x3 sand doughnut puddle
      33, 113, 36, 0,
      38, 34, 40, 0,
      0,  0,  0,  0 },
    { 37, 35, 35, 39,   //4x4 symmetrical sandbank w/corners
      33, 96, 96, 36,
      33, 96, 96, 36,
      38, 34, 34, 40 },
    { 37, 35, 35, 39,   //4x4 sand doughnut
      33, 50, 51, 36,
      33, 52, 53, 36,
      38, 34, 34, 40 },
    { 37, 35, 35, 39,   //4x4 sand doughnut puddle
      33, 101, 103, 36,
      33, 102, 104, 36,
      38, 34, 34, 40 },
    { 69, 67, 71, 0,    //3x3 symmetrical pond
      65, 64, 68, 0,
      70, 66, 72, 0,
      0,  0,  0,  0 },
    { 69, 67, 71, 0,    //3x3 water doughnut
      65, 86, 68, 0,
      70, 66, 72, 0,
      0,  0,  0,  0 },
    { 69, 71, 0, 0,     //1x 2x2 puddle
      70, 72, 0, 0,
      0,  0,  0, 0,
      0,  0,  0, 0 },
    { 69, 71, 0,  0,    //2x 2x2 puddle (top to bottom)
      70, 72, 0,  0,
      0,  0,  69, 71,
      0,  0,  70, 72 },
    { 0,  0,  69, 71,   //2x 2x2 puddle (bottom to top)
      0,  0,  70, 72,
      69, 71, 0,  0,
      70, 72, 0,  0 }
};

void setWatersEdge(byte &b, byte tile)
{
    b = T_WATER + tile + (b == T_SAND ? 32 : 0);
}

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
    //Sprites
    if (!spriteTexImg.loadFromFile("textures/sprites.png")) {
        std::cout << "Couldn't load textures/sprites.png" << std::endl;
    }
    spriteTexture.loadFromImage(spriteTexImg);
    spriteTile.setTexture(&spriteTexture);
    spriteTexture.setSmooth(true);
    //Entities
    //Load texture sheets
    //There are 1 textures to load: worker
    for (uint t = 0; t < 1; ++t) {
        entityTextures.push_back(new sf::Texture);
        if (!entityTextures[t]->loadFromFile("textures/e" + std::to_string(t) + ".png")) //Load texture sheet
        {
            std::cout << "ERR: Couldn't load texture sheet: textures/e" + std::to_string(t) << std::endl;
        }
    }
    //minimap
    mmTex.create(mWidth, mHeight);
    minimap.scale((double)mmSize / (double)mWidth, (double)mmSize / (double)mHeight);
    minimap.setOutlineThickness(1);
    minimap.setOutlineColor(sf::Color(0, 0, 0));
    minimap.setPosition(sf::Vector2f(0, mmDiagWidth / 2));
    minimap.setRotation(-45);
  //Generate biome & sprites
    //Load memories
    for (uint i = 0; i < mWidth; ++i) {
        biome[i] = new byte[mHeight];
    }
    for (uint i = 0; i < mWidth; ++i) {
        sprites[i] = new byte[mHeight];
    }
    //Completely random
    for (uint y = 0; y < mHeight; ++y) {
        for (uint x = 0; x < mWidth; ++x) {
            byte* m = &biome[x][y];
            *m = T_GRASS; //Start with grass
            if (rb(.03)) { *m = T_SAND_PATCH; } //Random sand patches
            if (rb(.02)) { *m = T_GRASS_WATER_PATCH; } //Random water patches
            //if (x == 0 || y == 0 || x == mWidth - 1 || y == mHeight - 1) { *m = 33; } //Border
        }
    }
    //Pastes
    for (uint y = 0; y < mHeight; ++y) {
        for (uint x = 0; x < mWidth; ++x) {
            if (rb(.05)) { //Should we put a random paste?
                const byte* p = &biomePastes[ri(0, 11)][0]; //Select a random paste
                byte py = -1;
              //First, check if the area is completely clear of any other biome
                bool clear = true;
                for (byte d = 0; d < pLen; ++d) { //For each paste data point
                    byte px = d % 4;
                    if (!px) { ++py; } //Next line
                    if (inBounds(x + px, y + py)) {
                        if (biome[x + px][y + py] != T_GRASS) {
                            clear = false;
                            break;
                        }
                    }
                }
              //If clear, paste
                if (clear) {
                    byte py = -1;
                    for (byte d = 0; d < pLen; ++d) { //For each paste data point
                        byte px = d % 4;
                        if (!px) { ++py; } //Next line
                        if (inBounds(x + px, y + py)) { //If not off the map
                            if (p[d]) { biome[x + px][y + py] = p[d]; }
                        }
                    }
                }
            }
        }
    }
    //Bodies of water
    
    //Rivers
    for (uint r = 0; r < GEN_RIVERS_AMOUNT; ++r) {
        const uint length = ri(GEN_RIVER_MIN_LEN, GEN_RIVER_MAX_LEN);
        const uint windBias = rf(-GEN_RIVER_WIND_BIAS, GEN_RIVER_WIND_BIAS);
        double x, y;
        double dir;
      //Set origin & direction
        byte org = ri(0, 3);
        switch (org) {
            case 0: //Go North
                x = ri(0, mWidth);
                y = mHeight;
                break;
            case 1: //Go East
                x = 0;
                y = ri(0, mHeight);
                break;
            case 2: //Go South
                x = ri(0, mWidth);
                y = 0;
                break;
            case 3: //Go West
                x = mWidth;
                y = ri(0, mHeight);
        }
        dir = org * 90 + rf(-45, 45);
        normaliseAng(dir);
        uint px = x, py = y; //Prev
      //For each intersection
        for (uint l = 0; l < length; ++l) {
          //Change direction
            dir += rf_nd(0.0, GEN_RIVER_BENDINESS);
            normaliseAng(dir);
          //Move in direction
            double dX, dY;
            angToVec(dir + windBias, dX, dY);
            x += dX;  y += dY;
          //Set water
            uint ix, iy;
            ix = (uint)x;  iy = (uint)y;
            if (inBounds(x, y)) {
                biome[ix][iy] = T_WATER;
            }
          //Generate waters' edges, by checking the square between the two, and setting accordingly
                   if (iy == py) { //We moved laterally
              //Set North
                if (inBounds(x, y - 1)) {
                    setWatersEdge(biome[ix][iy - 1], 15);
                }
              //Set South
                if (inBounds(x, y + 1)) {
                    setWatersEdge(biome[ix][iy + 1], 14);
                }
            } else if (ix == px) { //We moved longitudinally
              //Set East
                if (inBounds(x + 1, y)) {
                    setWatersEdge(biome[ix + 1][iy], 16);
                }
              //Set West
                if (inBounds(x - 1, y)) {
                    setWatersEdge(biome[ix - 1][iy], 13);
                }
            } else if (ix < px && iy < py) { //We moved NW
              //Set above
                if (inBounds(px, iy)) {
                    setWatersEdge(biome[px][iy], 20);
                }
              //Set below
                if (inBounds(ix, py)) {
                    setWatersEdge(biome[ix][py], 19);
                }
            } else if (ix > px && iy < py) { //We moved NE
              //Set above
                if (inBounds(px, iy)) {
                    setWatersEdge(biome[px][iy], 21);
                }
              //Set below
                if (inBounds(ix, py)) {
                    setWatersEdge(biome[ix][py], 18);
                }
            } else if (ix > px && iy > py) { //We moved SE
              //Set above
                if (inBounds(px, iy)) {
                    setWatersEdge(biome[px][iy], 19);
                }
              //Set below
                if (inBounds(ix, py)) {
                    setWatersEdge(biome[ix][py], 20);
                }
            } else if (ix < px && iy > py) { //We moved SW
              //Set above
                if (inBounds(px, iy)) {
                    setWatersEdge(biome[px][iy], 18);
                }
              //Set below
                if (inBounds(ix, py)) {
                    setWatersEdge(biome[ix][py], 21);
                }
            }
            px = ix;
            py = iy;
        }
    }
  //Sprites
    //Trees
    for (uint y = 0; y < mHeight; ++y) {
        for (uint x = 0; x < mWidth; ++x) {
            sprites[x][y] = 0;
            if (biome[x][y] == T_GRASS && rb(0.5)) { sprites[x][y] = 1; }
        }
    }
  //Entities
    //Init the entityMap
    for (uint y = 0; y < mHeight; ++y) {
        for (uint x = 0; x < mWidth; ++x) {
            entityMap[x][y] = new Entity();
        }
    }
  //First worker
    addEntity(cameraX + wTilesX / 2, cameraY + wTilesY / 2, 0);
    //biome[cameraX + wTilesX / 2][cameraY + wTilesY / 2] = 1;
    
  //Init display text
    if (!arialFont.loadFromFile("arial.ttf")) {
        std::cout << "ERR: Couldn't load font: arial.ttf" << std::endl;
    }
    displayText.setFont(arialFont);
    displayText.setColor(sf::Color::Black);
    displayText.setCharacterSize(24);
    displayText.setPosition(0, wHeight - 26);
  //Start the clock
    sf::Clock clock;
    sf::Time time = clock.getElapsedTime(); //time of current frame
    sf::Time oldTime = time; //time of previous frame
  //Start loop
    while (window.isOpen()) {
        pollInput();
        gameLoop();
        display();
      //Calculate frame time
        oldTime = time;
        time = clock.getElapsedTime();
        frameTime = (time - oldTime) / sf::seconds(1); //frameTime is the time this frame has taken, in seconds
        FPS = int(1.0 / frameTime);
        gtime = (ulong)time.asMilliseconds();
        
        sf::sleep(sf::milliseconds(16));
    }
}
