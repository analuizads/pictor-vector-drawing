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

string serializeScene(const Model& Data)
{
    ostringstream oss;
    oss << Data.LObjets.size() << "\n";
    for (auto& obj : Data.LObjets)
    {
        if (auto r = dynamic_pointer_cast<ObjRectangle>(obj))
        {
            oss << "RECT "
                << r->P1_.x << " " << r->P1_.y << " "
                << r->P2_.x << " " << r->P2_.y << "\n";
        }
        else if (auto s = dynamic_pointer_cast<ObjSegment>(obj))
        {
            oss << "SEG "
                << s->P1_.x << " " << s->P1_.y << " "
                << s->P2_.x << " " << s->P2_.y << "\n";
        }
        else if (auto c = dynamic_pointer_cast<ObjCircle>(obj))
        {
            oss << "CIRC "
                << c->center_.x << " " << c->center_.y << " "
                << c->radius_ << "\n";
        }
        else if (auto p = dynamic_pointer_cast<ObjPolygon>(obj))
        {
            oss << "POLY " << p->pts_.size();
            for (auto& pt : p->pts_)
                oss << " " << pt.x << " " << pt.y;
            oss << "\n";
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

            ObjAttr opt(Color::Blue, true, Color::Blue, 3);
            auto r = make_shared<ObjRectangle>(opt, p1, p2);
            Data.LObjets.push_back(r);
        }
        else if (type == "SEG")
        {
            V2 p1, p2;
            iss >> p1.x >> p1.y >> p2.x >> p2.y;

            ObjAttr opt(Color::White, false, Color::White, 2);
            auto sObj = make_shared<ObjSegment>(opt, p1, p2);
            Data.LObjets.push_back(sObj);
        }
        else if (type == "CIRC")
        {
            V2 c; float r;
            iss >> c.x >> c.y >> r;

            ObjAttr opt(Color::Green, true, Color::Green, 3);
            auto cObj = make_shared<ObjCircle>(opt, c, c + V2((int)r, 0));
            Data.LObjets.push_back(cObj);
        }
        else if (type == "POLY")
        {
            int m = 0;
            iss >> m;

            ObjAttr opt(Color::Red, false, Color::Red, 3);
            auto pObj = make_shared<ObjPolygon>(opt);

            for (int k = 0; k < m; ++k)
            {
                V2 pt;
                iss >> pt.x >> pt.y;
                pObj->pts_.push_back(pt);
            }
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

// CALLBACKS FOR TOOLS /////////////////////////////////////////////

// Segment: white line
void bntToolSegmentClick(Model& Data)
{
    Data.drawingOptions = ObjAttr(Color::White, false, Color::White, 2);
    Data.currentTool = make_shared<ToolSegment>();
}

// Rectangle: blue
void bntToolRectangleClick(Model& Data)
{
    Data.drawingOptions = ObjAttr(Color::Blue, true, Color::Blue, 3);
    Data.currentTool = make_shared<ToolRectangle>();
}

// Circle: green
void bntToolCircleClick(Model& Data)
{
    Data.drawingOptions = ObjAttr(Color::Green, true, Color::Green, 3);
    Data.currentTool = make_shared<ToolCircle>();
}

// Polygon: red
void bntToolPolygonClick(Model& Data)
{
    Data.drawingOptions = ObjAttr(Color::Red, false, Color::Red, 3);
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
    std::swap(Data.LObjets[idx], Data.LObjets[idx + 1]);
    Data.selectedObject = idx + 1;
}

void bntToolMoveBackClick(Model& Data)
{
    int idx = Data.selectedObject;
    if (idx <= 0 || idx >= (int)Data.LObjets.size()) return;
    std::swap(Data.LObjets[idx], Data.LObjets[idx - 1]);
    Data.selectedObject = idx - 1;
}

// RAZ ///////////////////////////////////////////////////////////////

void bntToolRAZClick(Model& Data)
{
    Data.LObjets.clear();
    Data.selectedObject = -1;
    
    // Reset tool to prevent crash when tool has references to deleted objects
    Data.currentTool = make_shared<ToolSegment>();
    Data.drawingOptions = ObjAttr(Color::White, false, Color::White, 2);
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

    App.LButtons.push_back(make_shared<Button>("Segment", V2(x, 0), V2(s, s), "outil_segment.png", bntToolSegmentClick));   x += s;
    App.LButtons.push_back(make_shared<Button>("Rectangle", V2(x, 0), V2(s, s), "outil_rectangle.png", bntToolRectangleClick)); x += s;
    App.LButtons.push_back(make_shared<Button>("Circle", V2(x, 0), V2(s, s), "outil_ellipse.png", bntToolCircleClick));   x += s;
    App.LButtons.push_back(make_shared<Button>("Polygon", V2(x, 0), V2(s, s), "outil_polygone.png", bntToolPolygonClick));  x += s;
    App.LButtons.push_back(make_shared<Button>("Select", V2(x, 0), V2(s, s), "outil_move.png", bntToolSelectionClick)); x += s;
    App.LButtons.push_back(make_shared<Button>("Edit Points", V2(x, 0), V2(s, s), "outil_edit.png", bntToolEditPointsClick)); x += s;
    App.LButtons.push_back(make_shared<Button>("Front", V2(x, 0), V2(s, s), "outil_up.png", bntToolMoveFrontClick)); x += s;
    App.LButtons.push_back(make_shared<Button>("Back", V2(x, 0), V2(s, s), "outil_down.png", bntToolMoveBackClick)); x += s;
    App.LButtons.push_back(make_shared<Button>("RAZ", V2(x, 0), V2(s, s), "outil_delete.png", bntToolRAZClick));      x += s;
    App.LButtons.push_back(make_shared<Button>("Save", V2(x, 0), V2(s, s), "outil_save.png", bntToolSaveClick));     x += s;
    App.LButtons.push_back(make_shared<Button>("Load", V2(x, 0), V2(s, s), "outil_load.png", bntToolLoadClick));     x += s;
    App.LButtons.push_back(make_shared<Button>("Undo", V2(x, 0), V2(s, s), "outil_undo.png", bntToolUndoClick));     x += s;
    
    cout << "Total de botoes criados: " << App.LButtons.size() << endl;
    cout << "Ultimo botao (Undo) em posicao x=" << (x - s) << " ate x=" << x << endl;
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
}
