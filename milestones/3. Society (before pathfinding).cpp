/*TODO
- add pathfinding
- add paths
*/
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
#include "patFindLib.cpp" //Path finding
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long ulong;

#define T_GRASS             32
#define T_WATER             64
#define T_SAND              96
#define T_SAND_PATCH        49
#define T_GRASS_WATER_PATCH 81
#define T_SAND_WATER_PATCH  113

#define GEN_RIVERS_AMOUNT   32
#define GEN_RIVER_MIN_LEN   64
#define GEN_RIVER_MAX_LEN   256
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
#define mWidth 256  //
#define mHeight 256 // biome dimensions

#define MAXENTITIES 2

#define FRAMEDELAY 200

sf::RenderWindow window(sf::VideoMode(wWidth, wHeight), "Society");
ulong gtime;
double frameTime;
double FPS;

byte **biome = new byte*[mWidth]; //biome array
byte **sprites = new byte*[mWidth]; //sprite array

//===================================
//For optimised/approx code
//===================================
union
{
    int tmp;
    float f;
} u;
float sqrt_approx(float z)
{
    u.f = z;
    u.tmp -= 1 << 23; /* Subtract 2^m. */
    u.tmp >>= 1; /* Divide by 2. */
    u.tmp += 1 << 29; /* Add ((b + 1) / 2) * 2^m. */
    return u.f;
}

inline float eD_approx(float x1, float y1, float x2, float y2) //Calculate the distance between two points, as the crow flies
{
    return sqrt_approx(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}
//===================================

//===================================
//GENERATORS
//===================================
std::mt19937 gen(0);//time(NULL));
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

void targToVec(double ourX, double ourY, double tarX, double tarY, double &dirX, double &dirY)
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

const bool solids[] = {
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
bool isSolid(double x, double y)
{
    bool solid = solids[biome[(uint)x][(uint)y]];
    return solid;
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

    void moveTo(double, double); //Path-find to a target

    Entity(double, double, byte, double, double); //Constructor
    Entity(); //Blank constructor
    ~Entity(); //Destructor

    void getTex(uint&, uint&); //Return the texture offset within its sheet
    void nextThink(); //AI-trigger

  private:
    double dX = 0; //
    double dY = 0; // Direction vector
    double targX = 0; //
    double targY = 0; // Target coordinate
    double moveS; //Speed of movement
    double haveMoved; //Distance since last direction change

    byte frame = 0; //Frame of animation
    uint frameT = 0; //Frame timer
    uint frameDelay = FRAMEDELAY; //Delay between frames

    void changeEMap(bool); //Set the entity on the entityMap
    void setp(double, double);
    bool tryMove();
};

Entity::Entity(double x, double y, byte type, double dX = 0, double dY = 1)
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
        while (entity[tryPos]->inited) //Search for a free position in the Entity array
        {
            ++tryPos;
        }
        entity[tryPos] = new Entity(pX, pY, type, dX, dY);
        ++entities;
    }
}

void Entity::moveTo (double x, double y)
{
    targX = x;  targY = y; //Set target
    targToVec(X, Y, targX, targY, dX, dY);
    moveS = frameTime;
}

void Entity::changeEMap(bool set)
{
    if (set) {
        entityMap[(uint)X][(uint)Y] = this;
    } else {
        entityMap[(uint)X][(uint)Y] = new Entity();
    }
}

void Entity::setp (double x, double y)
{
    changeEMap(false);
    X = x;
    Y = y;
    changeEMap(true);
}

void Entity::getTex (uint &teX, uint &teY)
{
  //Change the frame, if need be
    if (frameT < gtime) {
        frameT = gtime + frameDelay;
        if (moveS > 0.01) { //Are we moving?
            if (frame > 3) {
                frame = 0;
            }
            ++frame;
        } else {
            frame = 0;
        }
    }
  //Calculate the angle to show the sprite
    double eRot = vecToAng(dX, dY) - 4; //Entity's rotation (rotated for the display)
    if (eRot < 0) { eRot += 360; }
    byte z = eRot / 45; //Every 45 degrees, the next texture in the sheet is used (eg. 90 becomes 2)
    //Select the texture in the sheet, where the X-axis is the rotation, and the Y-axis is the animation frame
    teX = z * eTWidth;
    teY = eTHeight * frame;
}

bool Entity::tryMove ()
{
    bool ret = false;
    if (fabs(targX - X) < .2 && fabs(targY - Y) < .2) { moveS = 0; return false; }
  //Calculate direction based off target
    if (fabs(X - targX) > .2) { dX = (X < targX ? 1 : -1); }
    if (fabs(Y - targY) > .2) { dY = (Y < targY ? 1 : -1); }
  //Start move
    changeEMap(false);
    double nX = X + (dX * moveS);
    double nY = Y + (dY * moveS);
    if (!isSolid(nX, Y)) {
        X = nX;
    } else { ret = true; }
    if (!isSolid(X, nY)) {
        Y = nY;
    } else { ret = true; }
    changeEMap(true);
    return ret;
}

void Entity::nextThink ()
{
  //Move
    if (tryMove()) { //If we hit a solid
        targX = X;
        targY = Y;
    }
}
//===================================





void gameLoop ()
{
  //Handle each entity
    for (uint e = 0; e < MAXENTITIES; ++e) {
        if (entity[e]->inited) { //For initialised entities
            Entity* ent = entity[e];
            ent->nextThink(); //Trigger AI
        }
    }
}


sf::Image groundTexImg;
sf::Texture groundTexture;
sf::Sprite groundTile;
sf::Image spriteTexImg;
sf::Texture spriteTexture;
sf::Sprite spriteTile;
std::vector<sf::Texture*> entityTextures;
sf::Sprite entityTile;

bool drawSprites = true;
int hovX = 0, hovY = 0;
int selX = 0, selY = 0;
Entity* selEntity;

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

void getTex (byte m, uint &teX, uint &teY)
{
    teX = 0; teY = 0;
    if (m > 95) { m -= 96; teY += gTHeight * 3; }   //Switch to fourth layer
    if (m > 63) { m -= 64; teY += gTHeight * 2; }   //Switch to third layer
     else if (m > 31) { m -= 32; teY += gTHeight; } //Switch to second layer
    teX = m * gTWidth;
}

void display ()
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
            groundTile.setPosition(sf::Vector2f(pX, pY));
          //Draw biomes
            if (selX == x && selY == y) { //Check if selected
                groundTile.setColor(sf::Color(200, 200, 255));
            } else if (hovX == x && hovY == y) { //Check if hovered over
                groundTile.setColor(sf::Color(200, 255, 255));
            } else { //Normal
                groundTile.setColor(sf::Color(255, 255, 255));
            }
            window.draw(groundTile);
          //Draw entities (at a y offset, in order to not overlap with ground rendering)
            if (y > 0) {
                --y;
                if (entityMap[x][y]->inited) { //If there's an entity here
                    Entity* e = entityMap[x][y];
                    if (selX == x && selY == y) { //Check if selected
                        entityTile.setColor(sf::Color(100, 100, 255));
                        selEntity = e;
                    } else if (hovX == x && hovY == y) { //Check if hovered over
                        entityTile.setColor(sf::Color(100, 255, 255));
                    } else { //Normal
                        entityTile.setColor(sf::Color(200, 200, 200));
                        selEntity = new Entity();
                    }
                    entityTile.setTexture(*entityTextures[e->type], true);
                    uint teX, teY;
                    e->getTex(teX, teY);
                    entityTile.setTextureRect(sf::IntRect(teX, teY, eTWidth, eTHeight));
                    double eX = (e->Y - y);
                    double eY = (e->X - x);
                    double offX = (eX * (double)gTWidthH)   + (eY * (double)gTWidthH);
                    double offY = (eX * -(double)gTHeightH) + (eY * (double)gTHeightH) + (double)gTHeightH;
                    eX = (pX - eTWidth) + offX; //
                    eY = (pY - eTHeightH) - offY; // Calculate position based on current tile, an offset, and actual position
                    entityTile.setPosition(eX, eY);
                    window.draw(entityTile);
                }
                ++y;
            }
          //Draw sprites (at a y offset, in order to not overlap with ground rendering)
            if (drawSprites && y > 0) {
                --y;
                if (sprites[x][y]) { //If there's a sprite here
                    pY -= sTHeight - (sTHeightH / 2); //Adjust the sprite's position on the screen, as we are at an offset
                    if (pY + sTHeightH >= abs(pX - wWidthH) / 2) { //Only allow the tops of the sprites to show off-map
                        pX -= sTWidthH;
                        spriteTile.setTextureRect(sf::IntRect(sprites[x][y], 0, sTWidth, sTHeight));
                        spriteTile.setPosition(pX, pY);
                         if (hovX == x && hovY == y) { //Check if hovered over
                            spriteTile.setColor(sf::Color(100, 255, 255));
                        } else { //Normal
                            spriteTile.setColor(sf::Color(200, 200, 200));
                        }
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

ulong prevClick, prevPress;
void pollInput ()
{
  //Check if we want to exit
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
    }
  //Monitor mouse
    //Hover
    sf::Vector2i localPos = sf::Mouse::getPosition(window);
    int locX = ((double)wWidth / (double)window.getSize().x) * (double)localPos.x;  //
    int locY = ((double)wHeight / (double)window.getSize().y) * (double)localPos.y; // Adjust for scaled window
    //Detect where the click/hover has been made on the map (which tile)
    hovX = cameraX;
    hovY = cameraY;
    hovY += -((int)((wHeightH - locY) - locX / 2) / (int)gTHeight);
    hovX += wTilesX * 2.5 + ((int)((locX / 2 - wWidth) - locY) / (int)gTHeight) - 1;
    //Left click
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && prevClick < gtime - 100) {
        prevClick = gtime;
        selX = hovX;
        selY = hovY;
      //Check if we've selected an entity, and help if we just missed the corner of it
        if (!entityMap[selX][selY]->inited) {
          //Check all edges for an entity
            bool f = false; //Found flag
            if (selY > 1)                 { if (entityMap[selX][selY - 1]->inited) { --selY; f = true; } } //Northward
            if (selX < mWidth - 1 && !f)  { if (entityMap[selX + 1][selY]->inited) { ++selX; f = true; } } //Eastward
            if (selY < mHeight - 1 && !f) { if (entityMap[selX][selY + 1]->inited) { ++selY; f = true; } } //Southward
            if (selX > 1 && !f)           { if (entityMap[selX - 1][selY]->inited) { --selX; f = true; } } //Westward
          //Check corners for an entity
            if (selY > 1 && selX < mWidth - 1 && !f)           { if (entityMap[selX + 1][selY - 1]->inited) { ++selX; --selY; f = true; } } //North-Eastward
            if (selY < mHeight - 1 && selX < mWidth - 1 && !f) { if (entityMap[selX + 1][selY + 1]->inited) { ++selX; ++selY; f = true; } } //South-Eastward
            if (selY < mHeight - 1 && selX > 1 && !f)          { if (entityMap[selX - 1][selY + 1]->inited) { --selX; ++selY; f = true; } } //South-Westward
            if (selY > 1 && selX > 1 && !f)                    { if (entityMap[selX - 1][selY - 1]->inited) { --selX; --selY; f = true; } } //North-Westward
        }
      //If we had selected an entity, perform an action
        if (selEntity->inited) {
          //Move it to the new selected location
            selEntity->moveTo(selX + .5, selY + .5);
            selX = selY = -1;
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
  //Cheats & commands
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::T) && prevPress < gtime - 100) { //Toggle sprites
        prevPress = gtime;
        drawSprites = !drawSprites;
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

void setWatersEdge (byte &b, byte tile)
{
    b = T_WATER + tile + (b == T_SAND ? 32 : 0);
}

int main ()
{
  //Load window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  //Load screen
    //Init display text
    if (!arialFont.loadFromFile("arial.ttf")) {
        std::cout << "ERR: Couldn't load font: arial.ttf" << std::endl;
    }
    displayText.setFont(arialFont);
    displayText.setColor(sf::Color::Black);
    displayText.setCharacterSize(24);
    displayText.setPosition(0, wHeight - 26);
    //Display loading text
    displayText.setString("Loading...");
    window.clear(sf::Color(255, 255, 255));
    window.draw(displayText);
    window.display();
  //Load textures & tiles
    //Ground
    if (!groundTexImg.loadFromFile("textures/ground.png")) {
        std::cout << "Couldn't load textures/ground.png" << std::endl;
    }
    groundTexture.loadFromImage(groundTexImg);
    groundTile.setTexture(groundTexture);
    groundTexture.setSmooth(true);
    //Sprites
    if (!spriteTexImg.loadFromFile("textures/sprites.png")) {
        std::cout << "Couldn't load textures/sprites.png" << std::endl;
    }
    spriteTexture.loadFromImage(spriteTexImg);
    spriteTile.setTexture(spriteTexture);
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
            if (biome[x][y] == T_GRASS && rb(0.2)) { sprites[x][y] = 1; }
        }
    }
  //Entities
    //Init the entity list
    for (uint e = 0; e < MAXENTITIES; ++e) {
        entity[e] = new Entity();
    }
    //Init the entityMap
    for (uint y = 0; y < mHeight; ++y) {
        for (uint x = 0; x < mWidth; ++x) {
            entityMap[x][y] = new Entity();
        }
    }
  //First worker
    addEntity(cameraX + wTilesX / 2, cameraY + wTilesY / 2, 0);
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
