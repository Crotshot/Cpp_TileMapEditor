#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <time.h>
#include <vector>
#include <sstream>
#include <stdio.h>

#include "TextBox.hpp"
#include "TileMap.hpp"
#include "json.hpp"
#include "SensorGrid.hpp"

const sf::Vector2i paintAreaSize(582,582), paintAreaPosition(310, 10), selectAreaSize(282,382), selectAreaPosition(10,210);

const float limiterMax = 0.0625;
float limiter = 0; //Reduces the frequaency that left/right mouse and return key input to limiterMax/1second

std::string tileMapName, spriteSheetName;
unsigned int ssRows, ssColumns, tilePixelWidth, tilePixelHeight; //Data input by the text boxes

std::ifstream inputMap;
std::ofstream outMap;
sf::Texture spriteSheet;
nlohmann::json j;

SensorGrid* selectionGrid = new SensorGrid();
SensorGrid* paintingGrid = new SensorGrid();

std::vector<sf::Sprite> selectionSprites;
unsigned int selectedTile = -1, editTileIndex;
sf::Vector2i selectedTileV2(0,0);
std::vector<int> tileMapData;
std::vector<sf::Sprite> tileMapVisuals;
float tileScale;

void Save(){ //In Save() and Load() no need to type .json or .png as it will auto be appendened to the end
    if(tileMapName.length() > 0){
        std::cout << "Saving "<< tileMapName <<".json" << std::endl;
        outMap = std::ofstream("TileMaps/" + tileMapName + ".json");
        
        if (!outMap.is_open()) {
            std::cout << "Failed to save " << tileMapName << ".json" << '\n';
        }
        else{
            j["columns"] = ssColumns;
            j["rows"] = ssRows;
            j["width"] = tilePixelWidth;
            j["height"] = tilePixelHeight;
            j["image"] = spriteSheetName;
            j["level"] = tileMapData;
            outMap << j;
            outMap.seekp(0, std::ios::end);  
            if (outMap.tellp() <= 4)//Check if file is empty, if yes delete it
            {
                outMap.close();
                if(remove(("TileMaps/" + tileMapName + ".json").c_str()) == 0){
                    std::cout << "Failed to save " << tileMapName << ".json, the file was empty" << '\n';
                }
                else{
                    std::cout << "File " << tileMapName << ".json was empty but could not be deleted, it has been saved in TileMaps/" << std::endl;
                }
            }
            else{
                std::cout << "Saved " << tileMapName << ".json, successfully!" << std::endl;
            }
            
            if(outMap.is_open())
                outMap.close();
        }
    }
    else
        std::cout << "No tile map name entered, please enter a tilemap name." << std::endl;
}

void selectionGridSetUp(){
    if(spriteSheetName.length() > 0){
        std::cout << "Loading "<< spriteSheetName <<".png" << std::endl;
        if(!spriteSheet.loadFromFile("SpriteSheets/" + spriteSheetName + ".png")){
            std::cout << "Failed to load " << spriteSheetName << ".png, maybe it does not exist?" << std::endl;
        }
        else{
            std::cout << spriteSheetName << ".png loaded successfully!" << std::endl;
            
            int sheetCols, sheetRows;
            sheetCols = spriteSheet.getSize().x / tilePixelWidth;
            sheetRows = spriteSheet.getSize().y / tilePixelHeight;
            std::cout << "Debug SheetCols: "<< sheetCols << ", "  << spriteSheet.getSize().x <<", "<< tilePixelWidth<< std::endl;
            std::cout << "Debug SheetRows: "<< sheetRows << ", " << spriteSheet.getSize().y << ", "<< tilePixelHeight << std::endl;
            selectionGrid->GenerateSensorGrid(selectAreaPosition.x, selectAreaPosition.y, sheetRows, sheetCols, tilePixelWidth, tilePixelHeight);
            selectionSprites.clear();
            
            for(unsigned int y = 0; y < sheetRows; y++){
                for(unsigned int x = 0; x < sheetCols; x++){
                    selectionSprites.push_back( sf::Sprite(spriteSheet,sf::IntRect(tilePixelWidth * x, tilePixelHeight * y, tilePixelWidth, tilePixelHeight)));
                    selectionSprites.back().setPosition(selectAreaPosition.x + tilePixelWidth * x, selectAreaPosition.y + tilePixelHeight * y);
                }
            }
        }
    }
    else
        std::cout << "No sprite Sheet name entered, please enter a sprite sheet name." << std::endl;
}

bool updateFields;
float paintedTileScale;

void Load(){
    selectionSprites.clear();
    tileMapVisuals.clear();
    
    if(tileMapName.length() > 0){
        std::cout << "Loading "<< tileMapName <<".json" << std::endl;
        inputMap = std::ifstream("TileMaps/" + tileMapName + ".json");
        
        if (!inputMap.is_open()) {//Set up for a blank tile map
            std::cout << "File:" << tileMapName << ".json did not exist, making new tilemap" << std::endl;
            paintingGrid->GenerateSensorGrid(paintAreaPosition.x, paintAreaPosition.y, ssRows, ssColumns, tilePixelWidth, tilePixelHeight);
            paintingGrid->applyScale(paintAreaSize.x, paintAreaSize.y);
            paintedTileScale = paintingGrid->getScale();
            for( unsigned int y = 0; y < ssRows; y++){
                for( unsigned int x = 0; x < ssColumns; x++){
                    tileMapData.push_back(-1);
                    sf::Sprite tile(spriteSheet,sf::IntRect(0,0,tilePixelWidth, tilePixelHeight));
                    tile.setColor(sf::Color::Black);//Black tiles which acts like a rubber, represented by -1 in tileMapData
                    tile.setPosition(paintAreaPosition.x + (x * tilePixelWidth) * paintedTileScale, paintAreaPosition.y + (y * tilePixelHeight) * paintedTileScale);
                    tile.scale(paintedTileScale,paintedTileScale);
                    tileMapVisuals.push_back(tile);
                }
            }
            selectionGridSetUp();
        }
        else{ //Set up for a loaded tile map
            
            inputMap >> j;
            ssColumns = j["columns"].get<int>();
            ssRows = j["rows"].get<int>();
            tilePixelWidth = j["width"].get<int>();
            tilePixelHeight = j["height"].get<int>();
            spriteSheetName = j["image"].get<std::string>();
            tileMapData = j["level"].get<std::vector<int>>();
            updateFields = true;
            paintingGrid->GenerateSensorGrid(paintAreaPosition.x, paintAreaPosition.y, ssRows, ssColumns, tilePixelWidth, tilePixelHeight);
            selectionGridSetUp();
            int count = 0;
            
            paintingGrid->applyScale(paintAreaSize.x, paintAreaSize.y);
            paintedTileScale = paintingGrid->getScale();
            std::cout << paintedTileScale << std::endl;
            for( unsigned int y = 0; y < ssRows; y++){
                for( unsigned int x = 0; x < ssColumns; x++){
                    sf::Vector2i v = selectionGrid->ReverseDimension(tileMapData[count], false);
                    sf::Sprite tile(spriteSheet,sf::IntRect(v.x * tilePixelWidth, v.y * tilePixelHeight,tilePixelWidth, tilePixelHeight));
                    if(tileMapData[count] == -1)
                        tile.setColor(sf::Color::Black);
                    
                    tile.setPosition(paintAreaPosition.x + (x * tilePixelWidth) * paintedTileScale, paintAreaPosition.y + (y * tilePixelHeight) * paintedTileScale);
                    tile.scale(paintedTileScale,paintedTileScale);
                    tileMapVisuals.push_back(tile);
                    count++;
                }
            }
            std::cout << "Opened " << tileMapName << ".json, successfully!" << std::endl;
        }
    }
    else
        std::cout << "No tile map name entered, please enter a tilemap name." << std::endl;
}

void editText(std::vector<TextBox*> textBoxes){
    for(TextBox* tB : textBoxes){
        tB->EditText(); //Iterate though the textBoxes and edit the one that has been changged
    }
}

int main(){
    sf::RenderWindow window(sf::VideoMode(900, 600), "TileMap Editor");
    sf::Vector2f mouseScreenPosition, lastMousePosition; //Mouse in currentPosition and lastPosition
    
    sf::Texture background; //Background
    if (!background.loadFromFile("Textures/Background.png")){std::cout << "Failed to Background.png" << std::endl; return -99;}
    sf::Sprite spriteBackground;
    spriteBackground.setTexture(background);
    
    sf::Font F_alagard; //Font set up
    if (!F_alagard.loadFromFile("Fonts/alagard.ttf")){std::cout << "Failed to load alagard.ttf" << std::endl; return -99;}
    sf::Text textDTime;
    textDTime.setFont(F_alagard);
    textDTime.setCharacterSize(16);
    textDTime.setFillColor(sf::Color::Green);
    
    //Time/Clock Setup
    sf::Time time;
    sf::Clock clock;
    
    //---------------------------------------------------------
    //Input textBoxes setup
    TextBox* jsonBox = new TextBox();
    TextBox* spriteSheetBox = new TextBox();
    TextBox* tileRows = new TextBox();
    TextBox* tileColumns = new TextBox();
    TextBox* tileSizeX = new TextBox();
    TextBox* tileSizeY = new TextBox();
    
    jsonBox = new TextBox(*jsonBox, 122, 33, true, "Textures/TextBox.png");
    spriteSheetBox = new TextBox(*spriteSheetBox, 122, 33, true,"Textures/TextBox.png");
    tileRows = new TextBox(*tileRows, 61, 33, true,"Textures/TextBoxHalf.png");
    tileColumns = new TextBox(*tileColumns, 61, 33, true,"Textures/TextBoxHalf.png");
    tileSizeX = new TextBox(*tileSizeX, 61, 33, true,"Textures/TextBoxHalf.png");
    tileSizeY = new TextBox(*tileSizeY, 61, 33, true,"Textures/TextBoxHalf.png");
    
    jsonBox->GenerateTextBox(20,20, 16, sf::Color::Black);
    spriteSheetBox->GenerateTextBox(157,20, 16, sf::Color::Black);
    tileRows->GenerateTextBox(20,85, 26, sf::Color::Black);
    tileColumns->GenerateTextBox(86,85, 26, sf::Color::Black);
    tileSizeX->GenerateTextBox(152,85, 26, sf::Color::Black);
    tileSizeY->GenerateTextBox(218,85, 26, sf::Color::Black);
    
    //--------------------------------------------------
    std::vector<TextBox*> textBoxes; //Vector of input textBoxes for iterating when getting input and rendering
    textBoxes.push_back(jsonBox);
    textBoxes.push_back(spriteSheetBox);
    textBoxes.push_back(tileRows);
    textBoxes.push_back(tileColumns);
    textBoxes.push_back(tileSizeX);
    textBoxes.push_back(tileSizeY);
    //--------------------------------------------------------
    
    
    //-------------------------------------------------------
    //Set up for Save and oad buttons;
    TextBox* saveButton = new TextBox();
    saveButton= new TextBox(*saveButton, 99, 33, false, "Textures/TextBox3Q.png");
    saveButton->GenerateTextBox(40,150, 30, sf::Color::Green);
    //"False" in constructor denotes that these are not editable
    TextBox* loadButton = new TextBox();
    loadButton= new TextBox(*loadButton, 99, 33, false, "Textures/TextBox3Q.png");
    loadButton->GenerateTextBox(172,150, 30, sf::Color::Green);
    
    saveButton->setText("SAVE");
    loadButton->setText("LOAD");
    
    std::vector<TextBox*> buttons;
    buttons.push_back(saveButton);
    buttons.push_back(loadButton);
    //-------------------------------------------------------
    bool mouseDown = false; //Used to prevent 1 mouse click from continuely inputing
    bool inFocus = true; //Used to stop code when window is not in focus
    bool editingText = false;
    bool resetLimiter = false;
    
    std::string* editString; //Pointer to string that is currently being edited 
    
    while(window.isOpen()){
        if(updateFields){//Updates textBoxes afer loading in a json file
            
            spriteSheetBox->setText(spriteSheetName);
            jsonBox->setText(tileMapName);
            tileRows->setText(std::to_string(ssRows));
            tileColumns->setText(std::to_string(ssColumns));
            tileSizeX->setText(std::to_string(tilePixelWidth));
            tileSizeY->setText(std::to_string(tilePixelHeight));
            
            spriteSheetBox->setTextString(spriteSheetName);
            jsonBox->setTextString(tileMapName);
            tileRows->setTextString(std::to_string(ssRows));
            tileColumns->setTextString(std::to_string(ssColumns));
            tileSizeX->setTextString(std::to_string(tilePixelWidth));
            tileSizeY->setTextString(std::to_string(tilePixelHeight));
            
            updateFields = false;
            editText(textBoxes);
        }
        sf::Time dt= clock.restart();
        limiter += dt.asSeconds();
        textDTime.setString(std::to_string(limiter));
        mouseScreenPosition = sf::Vector2f(sf::Mouse::getPosition(window));
        
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)//Keep track of focus
                window.close();
            else if(event.type == sf::Event::GainedFocus){
                std::cout << "Debug: Focus Gained" << std::endl;
                inFocus = true;
            }
            else if(event.type == sf::Event::LostFocus){
                std::cout << "Debug: Focus Lost" << std::endl;
                inFocus = false;
            }
            
            if(!inFocus) //If not in Focus d nothing
                continue;
            if(event.type == sf::Event::MouseMoved)//Mouse movement is event and randomly edits text
                continue;
            if(!editingText)
                continue;
            
            if (event.type == sf::Event::TextEntered) {
                if (std::isprint(event.text.unicode)){
                    *editString += (char)event.text.unicode;
                    editText(textBoxes);
                }
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::BackSpace) {
                    if (!(*editString).empty())
                        (*editString).pop_back();
                }
                if (event.key.code == sf::Keyboard::Space) {
                    *editString += (char) ' ';
                }
                editText(textBoxes);
            }
        }
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Return))
        {
            for(TextBox* tB : textBoxes){ //Finish editing a textField
                sf::Vector2f v(-9999,-9999);
                tB->CheckInput(v);
                editingText = false;
            }
            if(limiter >= limiterMax){
                std::cout << "Debug------------------------------------------" << std::endl;
                
                std::cout << "tileMapName: " <<  tileMapName << ", spriteSheetName: " << spriteSheetName << std::endl;
                std::cout << "ssRows: " <<  ssRows << ", ssColumns: " << ssColumns << std::endl;
                std::cout << "tilePixelWidth: " <<  tilePixelWidth << ", tilePixelHeight: " << tilePixelHeight << std::endl;
                
                std::cout << "Debug------------------------------------------" << std::endl;
                resetLimiter = true;
            }
            
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
        {
            window.close();
        }
        if(sf::Mouse::isButtonPressed(sf::Mouse::Right)){
            if(limiter >= limiterMax){
                int temp = paintingGrid->ClickCheckInt(mouseScreenPosition.x, mouseScreenPosition.y);
                if(temp != -1){//Tile erasing
                    sf::Vector2i vTemp = paintingGrid->ClickCheckVectorInt(mouseScreenPosition.x, mouseScreenPosition.y, true);
                    editTileIndex = temp;
                    tileMapData[editTileIndex] = -1;
                    sf::Sprite tile(spriteSheet,sf::IntRect(0,0,tilePixelWidth, tilePixelHeight));
                    tile.setColor(sf::Color::Black);
                    tile.setPosition(vTemp.x, vTemp.y);
                    tile.scale(paintedTileScale,paintedTileScale);
                    tileMapVisuals[editTileIndex] = tile;
                    resetLimiter = true;
                }
            }
        }
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
        {
            if(limiter >= limiterMax){
                if(tileMapData.size() > 0){//Tile painting
                    int temp = paintingGrid->ClickCheckInt(mouseScreenPosition.x, mouseScreenPosition.y);
                    if(temp != -1){
                        sf::Vector2i vTemp = paintingGrid->ClickCheckVectorInt(mouseScreenPosition.x, mouseScreenPosition.y, true);
                        sf::Vector2i iTemp = paintingGrid->ClickCheckVectorInt(mouseScreenPosition.x, mouseScreenPosition.y, false);
                        
                        editTileIndex = temp;
                        sf::Sprite tile(spriteSheet,sf::IntRect(tilePixelWidth * selectedTileV2.x, tilePixelHeight * selectedTileV2.y, tilePixelWidth, tilePixelHeight));
                        tile.setPosition(vTemp.x, vTemp.y);
                        tile.scale(paintedTileScale,paintedTileScale);
                        tileMapData[editTileIndex] = selectedTile;
                        tileMapVisuals[editTileIndex] = tile;
                    }
                }
                resetLimiter = true;
            }
            if(lastMousePosition !=  mouseScreenPosition){
                if(!mouseDown){
                    //---------------------------------------------------
                    //Debugging Sensor Grids
                    sf::Vector2i sGridV= selectionGrid->ClickCheckVectorInt(mouseScreenPosition.x, mouseScreenPosition.y, false);
                    sf::Vector2i pGridV= paintingGrid->ClickCheckVectorInt(mouseScreenPosition.x, mouseScreenPosition.y, false);
                    
                    if(sGridV.x != -1){
                        //Assigning tile to paint with
                        selectedTile = selectionGrid->ClickCheckInt(mouseScreenPosition.x, mouseScreenPosition.y);
                        selectedTileV2 = selectionGrid->ClickCheckVectorInt(mouseScreenPosition.x, mouseScreenPosition.y, false);
                        
                        std::cout << "Debug: Touched selectionGrid at pos: (" << mouseScreenPosition.x << ", " << mouseScreenPosition.y << "), the index was " << selectedTile << std::endl; 
                        std::cout << "Debug: (" << sGridV.x << ", " << sGridV.y << ")" << std::endl;
                    }
                    if(pGridV.x != -1){
                        std::cout << "Debug: Touched paintingGrid at pos: (" << mouseScreenPosition.x << ", " << mouseScreenPosition.y << "), the index was " << editTileIndex << std::endl;
                        std::cout << "Debug: (" << pGridV.x << ", " << pGridV.y << ")" << std::endl;
                    }
                    //---------------------------------------------------
                    
                    tileMapName = *(jsonBox->getTextString());
                    spriteSheetName = *(spriteSheetBox->getTextString());
                    if((*(tileRows->getTextString())).length() > 0)
                        ssRows = std::stoi(*(tileRows->getTextString()));
                    if((*(tileColumns->getTextString())).length() > 0)
                        ssColumns = std::stoi(*(tileColumns->getTextString()));
                    if((*(tileSizeX->getTextString())).length() > 0)
                        tilePixelWidth = std::stoi(*(tileSizeX->getTextString()));
                    if((*(tileSizeY->getTextString())).length() > 0)
                        tilePixelHeight = std::stoi(*(tileSizeY->getTextString()));
                    
                    
                    lastMousePosition = mouseScreenPosition;
                    textDTime.setPosition(mouseScreenPosition.x, mouseScreenPosition.y);
                    
                    //------------------------------------------------
                    for(TextBox* tB : textBoxes){//Code for check if an input textBox has been pressed.
                        if(tB->CheckInput(mouseScreenPosition)){
                            std::cout << "Editing text box"<< std::endl;
                            editingText = true;
                            editString = tB->getTextString();
                            tB->EditText();
                            //editBox = tB;
                            break;
                        }
                    }
                    
                    if(saveButton->CheckInput(mouseScreenPosition)){ //Check save and load buttons
                        Save();
                    }
                    else if(loadButton->CheckInput(mouseScreenPosition)){
                        Load();
                    }
                    //------------------------------------------------
                }
                mouseDown = true;
            }
        }
        else{
            mouseDown = false;
        }
        
        window.clear();
        
        window.draw(spriteBackground);
        
        for(TextBox* tB : textBoxes){//Draw all the text Boxses
            window.draw(tB->getSprite());
            window.draw(tB->getText());
        }
        
        for(TextBox* tB : buttons){//Draw all the text Buttons
            window.draw(tB->getSprite());
            window.draw(tB->getText());
        }
        
        for(sf::Sprite s : selectionSprites) 
            window.draw(s);
        
        for(sf::Sprite s : tileMapVisuals)
            window.draw(s);
        
        window.draw(textDTime);
        window.display();
        
        if(resetLimiter){
            limiter = 0;
            resetLimiter = false;
        }
    }
    return 0;
}