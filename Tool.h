/* Copyright (c) 2024 Lilian Buzer - All rights reserved - */
#pragma once

#include "Event.h"
#include "ObjGeom.h"
#include "Model.h"
#include "Graphics.h"
#include <memory>

enum class State { WAIT, INTERACT };

class Tool
{
public:
    State currentState = State::WAIT;

    Tool() {}
    virtual ~Tool() {}

    virtual void processEvent(const Event& E, Model& Data) {}
    virtual void draw(Graphics& G, const Model& Data) {}
};

///////////////////////////////////////////////////////////////
// SEGMENT TOOL
///////////////////////////////////////////////////////////////

class ToolSegment : public Tool
{
    V2 Pstart;

public:
    ToolSegment() : Tool() {}

    void processEvent(const Event& E, Model& Data) override
    {
        if (E.Type == EventType::MouseDown && E.info == "0")
        {
            Pstart = Data.currentMousePos;
            currentState = State::INTERACT;
            return;
        }

        if (E.Type == EventType::MouseUp && E.info == "0" &&
            currentState == State::INTERACT)
        {
            pushUndoState(Data);  // Save state before adding object
            auto obj = std::make_shared<ObjSegment>(
                Data.drawingOptions, Pstart, Data.currentMousePos);
            Data.LObjets.push_back(obj);
            currentState = State::WAIT;
        }
    }

    void draw(Graphics& G, const Model& Data) override
    {
        if (currentState == State::INTERACT)
        {
            G.drawLine(Pstart, Data.currentMousePos,
                Data.drawingOptions.borderColor_,
                Data.drawingOptions.thickness_);
        }
    }
};

///////////////////////////////////////////////////////////////
// RECTANGLE TOOL
///////////////////////////////////////////////////////////////

class ToolRectangle : public Tool
{
    V2 Pstart;

public:
    ToolRectangle() : Tool() {}

    void processEvent(const Event& E, Model& Data) override
    {
        if (E.Type == EventType::MouseDown && E.info == "0")
        {
            Pstart = Data.currentMousePos;
            currentState = State::INTERACT;
            return;
        }

        if (E.Type == EventType::MouseUp && E.info == "0" &&
            currentState == State::INTERACT)
        {
            pushUndoState(Data);  // Save state before adding object
            auto obj = std::make_shared<ObjRectangle>(
                Data.drawingOptions, Pstart, Data.currentMousePos);
            Data.LObjets.push_back(obj);
            currentState = State::WAIT;
        }
    }

    void draw(Graphics& G, const Model& Data) override
    {
        if (currentState != State::INTERACT)
            return;

        V2 P, size;
        getPLH(Pstart, Data.currentMousePos, P, size);

        if (Data.drawingOptions.isFilled_)
            G.drawRectangle(P, size,
                Data.drawingOptions.interiorColor_, true);

        G.drawRectangle(P, size,
            Data.drawingOptions.borderColor_,
            false,
            Data.drawingOptions.thickness_);
    }
};

///////////////////////////////////////////////////////////////
// CIRCLE TOOL
///////////////////////////////////////////////////////////////

class ToolCircle : public Tool
{
    V2 center_;

public:
    ToolCircle() : Tool() {}

    void processEvent(const Event& E, Model& Data) override
    {
        if (E.Type == EventType::MouseDown && E.info == "0")
        {
            center_ = Data.currentMousePos;
            currentState = State::INTERACT;
            return;
        }

        if (E.Type == EventType::MouseUp && E.info == "0" &&
            currentState == State::INTERACT)
        {
            pushUndoState(Data);  // Save state before adding object
            auto obj = std::make_shared<ObjCircle>(
                Data.drawingOptions, center_, Data.currentMousePos);
            Data.LObjets.push_back(obj);
            currentState = State::WAIT;
        }
    }

    void draw(Graphics& G, const Model& Data) override
    {
        if (currentState != State::INTERACT)
            return;

        float r = (Data.currentMousePos - center_).norm();

        if (Data.drawingOptions.isFilled_)
            G.drawCircle(center_, r,
                Data.drawingOptions.interiorColor_, true);

        G.drawCircle(center_, r,
            Data.drawingOptions.borderColor_,
            false,
            Data.drawingOptions.thickness_);
    }
};

///////////////////////////////////////////////////////////////
// POLYGON TOOL
///////////////////////////////////////////////////////////////

class ToolPolygon : public Tool
{
    std::shared_ptr<ObjPolygon> poly_;
    bool building = false;

public:
    ToolPolygon() : Tool() {}

    void processEvent(const Event& E, Model& Data) override
    {
        // Left click: add point
        if (E.Type == EventType::MouseDown && E.info == "0")
        {
            if (!building)
            {
                pushUndoState(Data);  // Save state before starting polygon
                poly_ = std::make_shared<ObjPolygon>(Data.drawingOptions);
                Data.LObjets.push_back(poly_);
                building = true;
                currentState = State::INTERACT;
            }

            poly_->addPoint(Data.currentMousePos);
            return;
        }

        // Right-click (or any non-left click): finish polygon
        if (E.Type == EventType::MouseDown && E.info != "0")
        {
            if (building)
            {
                if (poly_->pts_.size() < 2)
                    Data.LObjets.pop_back();

                building = false;
                poly_.reset();
                currentState = State::WAIT;
            }
            return;
        }

        // ESC: cancel polygon
        if (E.Type == EventType::KeyDown && E.info == "Escape")
        {
            if (building)
            {
                Data.LObjets.pop_back();
                building = false;
                poly_.reset();
                currentState = State::WAIT;
            }
        }
    }

    void draw(Graphics& G, const Model& Data) override
    {
        if (!building || !poly_) return;

        int n = poly_->pts_.size();

        for (int i = 0; i < n - 1; i++)
        {
            G.drawLine(poly_->pts_[i], poly_->pts_[i + 1],
                Data.drawingOptions.borderColor_,
                Data.drawingOptions.thickness_);
        }

        if (n >= 1)
        {
            G.drawLine(poly_->pts_.back(), Data.currentMousePos,
                Data.drawingOptions.borderColor_,
                Data.drawingOptions.thickness_);
        }
    }
};

///////////////////////////////////////////////////////////////
// SELECTION TOOL
///////////////////////////////////////////////////////////////

class ToolSelection : public Tool
{
    V2 lastMouse_;
    bool dragging_ = false;

public:
    ToolSelection() : Tool() {}

    void processEvent(const Event& E, Model& Data) override
    {
        // Drag
        if (E.Type == EventType::MouseMove)
        {
            if (dragging_ &&
                Data.selectedObject >= 0 &&
                Data.selectedObject < (int)Data.LObjets.size())
            {
                V2 delta = Data.currentMousePos - lastMouse_;
                Data.LObjets[Data.selectedObject]->moveBy(delta);
                lastMouse_ = Data.currentMousePos;
            }
            return;
        }

        // Left-click: select object
        if (E.Type == EventType::MouseDown && E.info == "0")
        {
            Data.selectedObject = -1;
            for (int i = Data.LObjets.size() - 1; i >= 0; --i)
            {
                if (Data.LObjets[i]->hitTest(Data.currentMousePos))
                {
                    Data.selectedObject = i;
                    dragging_ = true;
                    lastMouse_ = Data.currentMousePos;
                    break;
                }
            }
            return;
        }

        // Mouse-up: stop drag
        if (E.Type == EventType::MouseUp && E.info == "0")
        {
            dragging_ = false;
            return;
        }

        // Right-click: delete
        if (E.Type == EventType::MouseDown && E.info != "0")
        {
            if (Data.selectedObject >= 0 &&
                Data.selectedObject < (int)Data.LObjets.size())
            {
                Data.LObjets.erase(Data.LObjets.begin() + Data.selectedObject);
                Data.selectedObject = -1;
            }
            return;
        }

        // any key deletes the selected object
        if (E.Type == EventType::KeyDown)
        {
            if (Data.selectedObject >= 0 &&
                Data.selectedObject < (int)Data.LObjets.size())
            {
                Data.LObjets.erase(Data.LObjets.begin() + Data.selectedObject);
                Data.selectedObject = -1;
            }
        }
    }

    void draw(Graphics& G, const Model& Data) override
    {
        if (Data.selectedObject < 0 ||
            Data.selectedObject >= (int)Data.LObjets.size())
            return;

        auto obj = Data.LObjets[Data.selectedObject];

        // highlight selected object
        if (auto R = dynamic_pointer_cast<ObjRectangle>(obj))
        {
            V2 P, size;
            getPLH(R->P1_, R->P2_, P, size);
            G.drawRectangle(P, size, Color::Yellow, false, 2);
        }
        else if (auto S = dynamic_pointer_cast<ObjSegment>(obj))
        {
            G.drawLine(S->P1_, S->P2_, Color::Yellow, 2);
        }
        else if (auto C = dynamic_pointer_cast<ObjCircle>(obj))
        {
            G.drawCircle(C->center_, C->radius_, Color::Yellow, false, 2);
        }
        else if (auto Pp = dynamic_pointer_cast<ObjPolygon>(obj))
        {
            for (int i = 0; i < (int)Pp->pts_.size() - 1; i++)
            {
                G.drawLine(Pp->pts_[i], Pp->pts_[i + 1],
                    Color::Yellow, 2);
            }
        }
    }
};

///////////////////////////////////////////////////////////////
// EDIT POINTS TOOL
///////////////////////////////////////////////////////////////

class ToolEditPoints : public Tool
{
    int objIndex = -1;
    int ptIndex = -1;
    bool dragging = false;

public:
    ToolEditPoints() : Tool() {}

    bool isMouseNearPoint(V2 mouse, V2 point)
    {
        return (mouse - point).norm() <= 8.0f;
    }

    void processEvent(const Event& E, Model& Data) override
    {
        if (E.Type == EventType::MouseMove && dragging)
        {
            if (objIndex >= 0 && ptIndex >= 0)
            {
                Data.LObjets[objIndex]->setPoint(ptIndex, Data.currentMousePos);
            }
            return;
        }

        if (E.Type == EventType::MouseDown && E.info == "0")
        {
            objIndex = -1;
            ptIndex = -1;

            for (int i = Data.LObjets.size() - 1; i >= 0; --i)
            {
                int count = Data.LObjets[i]->getPointCount();

                for (int p = 0; p < count; ++p)
                {
                    if (isMouseNearPoint(Data.currentMousePos,
                                         Data.LObjets[i]->getPoint(p)))
                    {
                        objIndex = i;
                        ptIndex = p;
                        dragging = true;
                        return;
                    }
                }
            }

            dragging = false;
            objIndex = -1;
            ptIndex = -1;
            return;
        }

        if (E.Type == EventType::MouseUp && E.info == "0")
        {
            dragging = false;
            return;
        }
    }

    void draw(Graphics& G, const Model& Data) override
    {
        for (auto& obj : Data.LObjets)
            obj->drawPoints(G);

        if (objIndex >= 0 && ptIndex >= 0)
        {
            V2 P = Data.LObjets[objIndex]->getPoint(ptIndex);

            int s = 6;
            G.drawRectangle(P - V2(s, s), V2(2*s, 2*s), Color::Red, true);
        }
    }
};
