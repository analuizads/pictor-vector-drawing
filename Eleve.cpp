/* Copyright (c) 2024 Lilian Buzer - All rights reserved - */

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "V2.h"
#include "Graphics.h"
#include "Event.h"
#include "Model.h"
#include "Button.h"
#include "Tool.h"

using namespace std;

// Forward declarations
void drawApp(Graphics& G, const Model& D);
void processEvent(const Event& Ev, Model& Data);
void drawCursor(Graphics& G, const Model& D);
void pushUndoState(Model& Data);

// SERIALIZATION HELPERS //////////////////////////////////////////

// Helper: serialize a Color to string
string serializeColor(const Color& c)
{
    // Use RGB values (0-1 range)
    ostringstream oss;
    oss << c.R << " " << c.G << " " << c.B;
    return oss.str();
}

// Helper: deserialize a Color from stream
Color deserializeColor(istringstream& iss)
{
    float r, g, b;
    iss >> r >> g >> b;
    return Color(r, g, b);
}

string serializeScene(const Model& Data)
{
    ostringstream oss;
    oss << Data.LObjets.size() << "\n";
    for (auto& obj : Data.LObjets)
    {
        // Serialize drawing attributes (colors, thickness, fill)
        auto& attr = obj->drawInfo_;
        string borderCol = serializeColor(attr.borderColor_);
        string fillCol = serializeColor(attr.interiorColor_);
        int thick = attr.thickness_;
        int filled = attr.isFilled_ ? 1 : 0;

        if (auto r = dynamic_pointer_cast<ObjRectangle>(obj))
        {
            oss << "RECT "
                << r->P1_.x << " " << r->P1_.y << " "
                << r->P2_.x << " " << r->P2_.y << " "
                << borderCol << " " << fillCol << " "
                << thick << " " << filled << "\n";
        }
        else if (auto s = dynamic_pointer_cast<ObjSegment>(obj))
        {
            oss << "SEG "
                << s->P1_.x << " " << s->P1_.y << " "
                << s->P2_.x << " " << s->P2_.y << " "
                << borderCol << " " << fillCol << " "
                << thick << " " << filled << "\n";
        }
        else if (auto c = dynamic_pointer_cast<ObjCircle>(obj))
        {
            oss << "CIRC "
                << c->center_.x << " " << c->center_.y << " "
                << c->radius_ << " "
                << borderCol << " " << fillCol << " "
                << thick << " " << filled << "\n";
        }
        else if (auto p = dynamic_pointer_cast<ObjPolygon>(obj))
        {
            oss << "POLY " << p->pts_.size();
            for (auto& pt : p->pts_)
                oss << " " << pt.x << " " << pt.y;
            oss << " " << borderCol << " " << fillCol << " "
                << thick << " " << filled << "\n";
        }
    }
    return oss.str();
}

void deserializeScene(Model& Data, const string& s)
{
    istringstream iss(s);
    size_t n = 0;
    iss >> n;
    Data.LObjets.clear();
    Data.selectedObject = -1;

    for (size_t i = 0; i < n; ++i)
    {
        string type;
        iss >> type;
        
        if (type == "RECT")
        {
            V2 p1, p2;
            iss >> p1.x >> p1.y >> p2.x >> p2.y;
            
            Color borderCol = deserializeColor(iss);
            Color fillCol = deserializeColor(iss);
            int thick, filled;
            iss >> thick >> filled;

            ObjAttr opt(borderCol, filled != 0, fillCol, thick);
            auto r = make_shared<ObjRectangle>(opt, p1, p2);
            Data.LObjets.push_back(r);
        }
        else if (type == "SEG")
        {
            V2 p1, p2;
            iss >> p1.x >> p1.y >> p2.x >> p2.y;
            
            Color borderCol = deserializeColor(iss);
            Color fillCol = deserializeColor(iss);
            int thick, filled;
            iss >> thick >> filled;

            ObjAttr opt(borderCol, filled != 0, fillCol, thick);
            auto sObj = make_shared<ObjSegment>(opt, p1, p2);
            Data.LObjets.push_back(sObj);
        }
        else if (type == "CIRC")
        {
            V2 c; float r;
            iss >> c.x >> c.y >> r;
            
            Color borderCol = deserializeColor(iss);
            Color fillCol = deserializeColor(iss);
            int thick, filled;
            iss >> thick >> filled;

            ObjAttr opt(borderCol, filled != 0, fillCol, thick);
            auto cObj = make_shared<ObjCircle>(opt, c, c + V2((int)r, 0));
            Data.LObjets.push_back(cObj);
        }
        else if (type == "POLY")
        {
            int m = 0;
            iss >> m;

            vector<V2> points;
            for (int k = 0; k < m; ++k)
            {
                V2 pt;
                iss >> pt.x >> pt.y;
                points.push_back(pt);
            }
            
            Color borderCol = deserializeColor(iss);
            Color fillCol = deserializeColor(iss);
            int thick, filled;
            iss >> thick >> filled;

            ObjAttr opt(borderCol, filled != 0, fillCol, thick);
            auto pObj = make_shared<ObjPolygon>(opt);
            pObj->pts_ = points;
            Data.LObjets.push_back(pObj);
        }
    }
}

// UNDO ////////////////////////////////////////////////////////////

void pushUndoState(Model& Data)
{
    string s = serializeScene(Data);
    Data.undoStack.push_back(s);
    if (Data.undoStack.size() > 20)
        Data.undoStack.erase(Data.undoStack.begin());
}

void doUndo(Model& Data)
{
    if (Data.undoStack.empty()) return;
    string s = Data.undoStack.back();
    Data.undoStack.pop_back();
    deserializeScene(Data, s);
}

// ================================================================
// DRAWING OPTIONS (PLOTTING OPTIONS)
// ================================================================

// simple 6-color palette indices
static int borderPaletteIndex = 0;
static int fillPaletteIndex = 0;

Color paletteColorFromIndex(int idx)
{
    idx = idx % 6;
    switch (idx)
    {
    case 0: return Color::White;
    case 1: return Color::Red;
    case 2: return Color::Green;
    case 3: return Color::Blue;
    case 4: return Color::Yellow;
    case 5: return Color::Cyan;
    default: return Color::White;
    }
}

// Border color button
void bntBorderColorClick(Model& Data)
{
    borderPaletteIndex = (borderPaletteIndex + 1) % 6;
    Data.drawingOptions.borderColor_ = paletteColorFromIndex(borderPaletteIndex);
}

// Fill color button
void bntFillColorClick(Model& Data)
{
    fillPaletteIndex = (fillPaletteIndex + 1) % 6;
    Data.drawingOptions.interiorColor_ = paletteColorFromIndex(fillPaletteIndex);
}

// Thickness button (cycles 1, 3, 5, 7)
void bntThicknessClick(Model& Data)
{
    int t = Data.drawingOptions.thickness_;
    if (t <= 1)      t = 3;
    else if (t == 3) t = 5;
    else if (t == 5) t = 7;
    else             t = 1;
    Data.drawingOptions.thickness_ = t;
}

// Fill toggle (on/off)
void bntFillToggleClick(Model& Data)
{
    Data.drawingOptions.isFilled_ = !Data.drawingOptions.isFilled_;
}

// ================================================================
// CALLBACKS FOR TOOLS
// ================================================================

// Segment: usa drawingOptions atual
void bntToolSegmentClick(Model& Data)
{
    Data.currentTool = make_shared<ToolSegment>();
}

// Rectangle
void bntToolRectangleClick(Model& Data)
{
    Data.currentTool = make_shared<ToolRectangle>();
}

// Circle
void bntToolCircleClick(Model& Data)
{
    Data.currentTool = make_shared<ToolCircle>();
}

// Polygon
void bntToolPolygonClick(Model& Data)
{
    Data.currentTool = make_shared<ToolPolygon>();
}

// Selection
void bntToolSelectionClick(Model& Data)
{
    Data.currentTool = make_shared<ToolSelection>();
}

// Edit Point
void bntToolEditPointsClick(Model& Data)
{
    Data.currentTool = make_shared<ToolEditPoints>();
}

// FRONT / BACK //////////////////////////////////////////////////////

void bntToolMoveFrontClick(Model& Data)
{
    int idx = Data.selectedObject;
    if (idx < 0 || idx >= (int)Data.LObjets.size() - 1) return;
    pushUndoState(Data);
    std::swap(Data.LObjets[idx], Data.LObjets[idx + 1]);
    Data.selectedObject = idx + 1;
}

void bntToolMoveBackClick(Model& Data)
{
    int idx = Data.selectedObject;
    if (idx <= 0 || idx >= (int)Data.LObjets.size()) return;
    pushUndoState(Data);
    std::swap(Data.LObjets[idx], Data.LObjets[idx - 1]);
    Data.selectedObject = idx - 1;
}

// RAZ ///////////////////////////////////////////////////////////////

void bntToolRAZClick(Model& Data)
{
    if (!Data.LObjets.empty())
        pushUndoState(Data);

    Data.LObjets.clear();
    Data.selectedObject = -1;

    // Reset tool and drawing options
    Data.currentTool = make_shared<ToolSegment>();
    Data.drawingOptions = ObjAttr(Color::White, false, Color::White, 2);
    borderPaletteIndex = 0;
    fillPaletteIndex = 0;
}

// SAVE / LOAD ///////////////////////////////////////////////////////

void bntToolSaveClick(Model& Data)
{
    ofstream F("scene.txt");
    if (!F) return;
    F << serializeScene(Data);
}

void bntToolLoadClick(Model& Data)
{
    ifstream F("scene.txt");
    if (!F) return;
    stringstream buf;
    buf << F.rdbuf();
    deserializeScene(Data, buf.str());
}

// UNDO //////////////////////////////////////////////////////////////

void bntToolUndoClick(Model& Data)
{
    doUndo(Data);
}

// INIT APP //////////////////////////////////////////////////////////

void initApp(Model& App)
{
    App.drawingOptions = ObjAttr(Color::White, false, Color::White, 2);
    App.currentTool = make_shared<ToolSegment>();

    int x = 0;
    int s = 70;

    // Tools
    App.LButtons.push_back(make_shared<Button>("Segment", V2(x, 0), V2(s, s), "outil_segment.png", bntToolSegmentClick));     x += s;
    App.LButtons.push_back(make_shared<Button>("Rectangle", V2(x, 0), V2(s, s), "outil_rectangle.png", bntToolRectangleClick));   x += s;
    App.LButtons.push_back(make_shared<Button>("Circle", V2(x, 0), V2(s, s), "outil_ellipse.png", bntToolCircleClick));      x += s;
    App.LButtons.push_back(make_shared<Button>("Polygon", V2(x, 0), V2(s, s), "outil_polygone.png", bntToolPolygonClick));     x += s;
    App.LButtons.push_back(make_shared<Button>("Select", V2(x, 0), V2(s, s), "outil_move.png", bntToolSelectionClick));   x += s;
    App.LButtons.push_back(make_shared<Button>("Edit Points", V2(x, 0), V2(s, s), "outil_edit.png", bntToolEditPointsClick));  x += s;

    // Plotting options
    App.LButtons.push_back(make_shared<Button>("Border Color", V2(x, 0), V2(s, s), "outil_bordercolor.png", bntBorderColorClick)); x += s;
    App.LButtons.push_back(make_shared<Button>("Fill Color", V2(x, 0), V2(s, s), "outil_fillcolor.png", bntFillColorClick));   x += s;
    App.LButtons.push_back(make_shared<Button>("Thickness", V2(x, 0), V2(s, s), "outil_thickness.png", bntThicknessClick));   x += s;
    App.LButtons.push_back(make_shared<Button>("Fill On/Off", V2(x, 0), V2(s, s), "outil_filltoggle.png", bntFillToggleClick));  x += s;

    // Scene & Z-order
    App.LButtons.push_back(make_shared<Button>("Front", V2(x, 0), V2(s, s), "outil_up.png", bntToolMoveFrontClick)); x += s;
    App.LButtons.push_back(make_shared<Button>("Back", V2(x, 0), V2(s, s), "outil_down.png", bntToolMoveBackClick));  x += s;
    App.LButtons.push_back(make_shared<Button>("RAZ", V2(x, 0), V2(s, s), "outil_delete.png", bntToolRAZClick));       x += s;
    App.LButtons.push_back(make_shared<Button>("Save", V2(x, 0), V2(s, s), "outil_save.png", bntToolSaveClick));      x += s;
    App.LButtons.push_back(make_shared<Button>("Load", V2(x, 0), V2(s, s), "outil_load.png", bntToolLoadClick));      x += s;
    App.LButtons.push_back(make_shared<Button>("Undo", V2(x, 0), V2(s, s), "outil_undo.png", bntToolUndoClick));      x += s;

    cout << "Total de botoes criados: " << App.LButtons.size() << endl;
}

// MAIN //////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
    cout << "Press ESC to abort" << endl;
    Graphics::initMainWindow("Pictor", V2(1200, 800), V2(200, 200));
    return 0;
}

// EVENTS ///////////////////////////////////////////////////////////

void processEvent(const Event& Ev, Model& Data)
{
    if (Ev.Type == EventType::MouseMove)
        Data.currentMousePos = V2(Ev.x, Ev.y);

    // Button click
    for (auto& B : Data.LButtons)
    {
        if (Ev.Type == EventType::MouseDown &&
            Data.currentMousePos.isInside(B->getPos(), B->getSize()))
        {
            B->manageEvent(Ev, Data);
            return;
        }
    }

    // Tool handles event
    if (Data.currentTool)
        Data.currentTool->processEvent(Ev, Data);
}

// CURSOR ///////////////////////////////////////////////////////////

void drawCursor(Graphics& G, const Model& D)
{
    V2 P = D.currentMousePos;
    int r = 6;

    G.drawLine(P - V2(r, 0), P + V2(r, 0), Color::White);
    G.drawLine(P - V2(0, r), P + V2(0, r), Color::White);
}

// DRAW /////////////////////////////////////////////////////////////

void drawApp(Graphics& G, const Model& D)
{
    G.clearWindow(Color::Black);

    for (auto& O : D.LObjets)
        O->draw(G);

    for (auto& B : D.LButtons)
        B->draw(G);

    if (D.currentTool)
        D.currentTool->draw(G, D);

    drawCursor(G, D);

    // Draw current drawing options 
    int yPos = 750;
    
    // Border color indicator
    G.drawRectangle(V2(10, yPos), V2(30, 30), D.drawingOptions.borderColor_, true);
    G.drawRectangle(V2(10, yPos), V2(30, 30), Color::White, false, 1);
    
    // Fill color indicator
    G.drawRectangle(V2(50, yPos), V2(30, 30), D.drawingOptions.interiorColor_, true);
    G.drawRectangle(V2(50, yPos), V2(30, 30), Color::White, false, 1);
    
    // Thickness indicator (draw line)
    int thick = D.drawingOptions.thickness_;
    G.drawLine(V2(100, yPos + 15), V2(150, yPos + 15), Color::White, thick);
    
    // Fill toggle indicator (square with or without fill)
    if (D.drawingOptions.isFilled_)
        G.drawRectangle(V2(170, yPos), V2(30, 30), Color::Green, true);
    else
        G.drawRectangle(V2(170, yPos), V2(30, 30), Color::Red, false, 2);
}
