/* Copyright (c) 2024 Lilian Buzer - All rights reserved - */
#pragma once

#include "Color.h"
#include "V2.h"
#include "ObjAttr.h"
#include "Graphics.h"
#include <vector>
#include <memory>
using namespace std;

/*
 Base class for all geometric objects.
 Supports:
 - Drawing
 - Hit testing (for selection)
 - Point editing (for ToolEditPoints)
*/
class ObjGeom
{
public:
    ObjAttr drawInfo_;

    ObjGeom() {}
    ObjGeom(ObjAttr di) : drawInfo_(di) {}

    // Draw the object
    virtual void draw(Graphics& G) {}

    //  Point Editing Interface 
    virtual int getPointCount() const { return 0; }
    virtual V2 getPoint(int i) const { return V2(); }
    virtual void setPoint(int i, V2 P) {}
    virtual void drawPoints(Graphics& G) {}

    //  Hit Testing for selection 
    virtual bool hitTest(const V2& P) { return false; }
    
    //  Move object by delta 
    virtual void moveBy(const V2& delta) {}
};

///////////////////////////////////////////////////////////////
// RECTANGLE
///////////////////////////////////////////////////////////////
class ObjRectangle : public ObjGeom
{
public:
    V2 P1_;
    V2 P2_;

    ObjRectangle(ObjAttr di, V2 A, V2 B) : ObjGeom(di), P1_(A), P2_(B) {}

    void draw(Graphics& G) override
    {
        V2 P, size;
        getPLH(P1_, P2_, P, size);

        if (drawInfo_.isFilled_)
            G.drawRectangle(P, size, drawInfo_.interiorColor_, true);

        G.drawRectangle(P, size, drawInfo_.borderColor_, false, drawInfo_.thickness_);
    }

    //  Hit Test (inside rectangle) 
    bool hitTest(const V2& P) override
    {
        V2 Q, size;
        getPLH(P1_, P2_, Q, size);
        return P.isInside(Q, size);
    }
    
    //  Move object 
    void moveBy(const V2& delta) override
    {
        P1_ = P1_ + delta;
        P2_ = P2_ + delta;
    }

    //  Point Editing 
    int getPointCount() const override { return 2; }

    V2 getPoint(int i) const override
    {
        return (i == 0 ? P1_ : P2_);
    }

    void setPoint(int i, V2 P) override
    {
        if (i == 0) P1_ = P;
        else P2_ = P;
    }

    void drawPoints(Graphics& G) override
    {
        int s = 5;
        Color c = Color::Yellow;

        G.drawRectangle(P1_ - V2(s, s), V2(2 * s, 2 * s), c, true);
        G.drawRectangle(P2_ - V2(s, s), V2(2 * s, 2 * s), c, true);
    }
};

///////////////////////////////////////////////////////////////
// SEGMENT
///////////////////////////////////////////////////////////////
class ObjSegment : public ObjGeom
{
public:
    V2 P1_;
    V2 P2_;

    ObjSegment(ObjAttr di, V2 A, V2 B) : ObjGeom(di), P1_(A), P2_(B) {}

    void draw(Graphics& G) override
    {
        G.drawLine(P1_, P2_, drawInfo_.borderColor_, drawInfo_.thickness_);
    }

    //  Hit test
    bool hitTest(const V2& P) override
    {
        V2 A = P1_;
        V2 B = P2_;

        V2 AP = P - A;
        V2 AB = B - A;

        float denom = AB.dot(AB);
        if (denom < 0.0001f) return false;

        float t = AP.dot(AB) / denom;
        t = max(0.0f, min(1.0f, t));

        V2 proj = A + AB * t;
        float dist = (P - proj).norm();

        return dist <= 6.0f; // tolerance
    }
    
    //  Move object 
    void moveBy(const V2& delta) override
    {
        P1_ = P1_ + delta;
        P2_ = P2_ + delta;
    }

    //  Point Editing 
    int getPointCount() const override { return 2; }
    V2 getPoint(int i) const override { return (i == 0 ? P1_ : P2_); }

    void setPoint(int i, V2 P) override
    {
        if (i == 0) P1_ = P;
        else P2_ = P;
    }

    void drawPoints(Graphics& G) override
    {
        int s = 5;
        Color c = Color::Yellow;
        G.drawRectangle(P1_ - V2(s, s), V2(2 * s, 2 * s), c, true);
        G.drawRectangle(P2_ - V2(s, s), V2(2 * s, 2 * s), c, true);
    }
};

///////////////////////////////////////////////////////////////
// CIRCLE
///////////////////////////////////////////////////////////////
class ObjCircle : public ObjGeom
{
public:
    V2 center_;
    float radius_;

    ObjCircle(ObjAttr di, V2 C, V2 boundary) : ObjGeom(di)
    {
        center_ = C;
        radius_ = (boundary - C).norm();
    }

    void draw(Graphics& G) override
    {
        if (drawInfo_.isFilled_)
            G.drawCircle(center_, radius_, drawInfo_.interiorColor_, true);

        G.drawCircle(center_, radius_, drawInfo_.borderColor_,
            false, drawInfo_.thickness_);
    }

    //  Hit Test (click inside circle) 
    bool hitTest(const V2& P) override
    {
        float dist = (P - center_).norm();
        return dist <= radius_;
    }
    
    //  Move object 
    void moveBy(const V2& delta) override
    {
        center_ = center_ + delta;
    }

    //  Point Editing 
    int getPointCount() const override { return 2; }

    V2 getPoint(int i) const override
    {
        if (i == 0) return center_;
        return center_ + V2((int)radius_, 0); // radius handle
    }

    void setPoint(int i, V2 P) override
    {
        if (i == 0)
        {
            center_ = P;
        }
        else
        {
            radius_ = (P - center_).norm();
        }
    }

    void drawPoints(Graphics& G) override
    {
        int s = 5;
        Color c = Color::Yellow;

        V2 p1 = center_;
        V2 p2 = center_ + V2((int)radius_, 0);

        G.drawRectangle(p1 - V2(s, s), V2(2 * s, 2 * s), c, true);
        G.drawRectangle(p2 - V2(s, s), V2(2 * s, 2 * s), c, true);
    }
};

///////////////////////////////////////////////////////////////
// POLYGON
///////////////////////////////////////////////////////////////
class ObjPolygon : public ObjGeom
{
public:
    vector<V2> pts_;

    ObjPolygon(ObjAttr di) : ObjGeom(di) {}

    void addPoint(V2 P) { pts_.push_back(P); }

    void draw(Graphics& G) override
    {
        for (int i = 0; i < (int)pts_.size() - 1; i++)
        {
            G.drawLine(pts_[i], pts_[i + 1],
                drawInfo_.borderColor_, drawInfo_.thickness_);
        }
    }

    //  Hit Test 
    bool hitTest(const V2& P) override
    {
        for (int i = 0; i < (int)pts_.size() - 1; i++)
        {
            V2 A = pts_[i];
            V2 B = pts_[i + 1];

            V2 AP = P - A;
            V2 AB = B - A;

            float denom = AB.dot(AB);
            if (denom < 0.0001f) continue;

            float t = AP.dot(AB) / denom;
            t = max(0.0f, min(1.0f, t));

            V2 proj = A + AB * t;
            float dist = (P - proj).norm();

            if (dist <= 6.0f) return true;
        }
        return false;
    }
    
    //  Move object 
    void moveBy(const V2& delta) override
    {
        for (int i = 0; i < (int)pts_.size(); i++)
        {
            pts_[i] = pts_[i] + delta;
        }
    }

    //  Point Editing 
    int getPointCount() const override { return pts_.size(); }

    V2 getPoint(int i) const override { return pts_[i]; }

    void setPoint(int i, V2 P) override { pts_[i] = P; }

    void drawPoints(Graphics& G) override
    {
        int s = 5;
        Color c = Color::Yellow;

        for (auto& p : pts_)
            G.drawRectangle(p - V2(s, s), V2(2 * s, 2 * s), c, true);
    }
};



