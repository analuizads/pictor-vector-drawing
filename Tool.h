/* Copyright (c) 2024 Lilian Buzer - All rights reserved - */
#pragma once

#include <string>
#include "Event.h"
#include "Model.h"
#include "ObjGeom.h"

using namespace std;

enum State { WAIT, INTERACT  };

////////////////////////////////////////////////////////////////////

class Tool
{
protected :
	int currentState;

public:
	Tool() {   currentState = State::WAIT; }
	virtual void processEvent(const Event& E, Model & Data) {}

	virtual void draw(Graphics& G,const  Model& Data) {}
};

////////////////////////////////////////////////////////////////////


class ToolSegment : public Tool
{
	V2 Pstart;
public : 

	ToolSegment() : Tool() {}
	void processEvent(const Event& E, Model& Data) override
	{
		if (E.Type == EventType::MouseDown && E.info == "0") // left mouse button pressed
		{
			if (currentState == State::WAIT)
			{
				// interactive drawing
				Pstart = Data.currentMousePos;
				currentState = State::INTERACT;  
				return;
			}
		}

		if (E.Type == EventType::MouseUp && E.info == "0") // left mouse button released
		{
			if (currentState == State::INTERACT)
			{
				// add object in the scene
				V2 P2 = Data.currentMousePos;
				auto newObj = make_shared<ObjSegment>(Data.drawingOptions, Pstart, P2);
				Data.LObjets.push_back(newObj);
				currentState = State::WAIT;
				return;
			}
		}
	}


	void draw(Graphics& G, const Model& Data) override
	{
		if (currentState == State::INTERACT)
  		  G.drawLine(Pstart, Data.currentMousePos, Data.drawingOptions.borderColor_, Data.drawingOptions.thickness_);
	}

};


////////////////////////////////////////////////////////////////////


class ToolRectangle : public Tool
{
public:

	ToolRectangle() : Tool() {}
	void processEvent(const Event& E, Model& Data) override
	{

	}

	void draw(Graphics& G, const Model& Data) override
	{
	}
};
