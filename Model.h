/* Copyright (c) 2024 Lilian Buzer - All rights reserved - */
#pragma once

#include "Color.h"
#include "ObjGeom.h"
#include "V2.h"
#include "ObjAttr.h"
#include <vector>
#include <memory>
#include <string>
using namespace std;

// forward declarations
class Tool;
class Button;
class Model;
void initApp(Model& Data);
void pushUndoState(Model& Data);

class Model
{
public:

    shared_ptr<Tool> currentTool;

    V2 currentMousePos;

    ObjAttr drawingOptions;

    vector< shared_ptr<ObjGeom> > LObjets;

    vector< shared_ptr<Button> > LButtons;

    // index of selected object (-1 if none)
    int selectedObject = -1;

    // undo stack
    vector<string> undoStack;

    Model()
    {
        initApp(*this);
    }
};

