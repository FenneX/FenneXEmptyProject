/****************************************************************************
 Copyright (c) 2013-2014 Auticiel SAS
 
 http://www.fennex.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************///

#include "Monkey.h"

static Monkey *s_Monkey = NULL;

Monkey* Monkey::excitedMonkey(void)
{
    if (!s_Monkey)
    {
        s_Monkey = new Monkey();
        s_Monkey->init();
    }
    
    return s_Monkey;
}

void Monkey::update(float delta)
{
    if(state == SearchScenes)
    {
        isSceneVisited[SceneSwitcher::sharedSwitcher()->getCurrentSceneName()] = true;
        bool searchFinished = true;
        for(int i = 0; i < SCENES_NUMBER && searchFinished; i++)
        {
            if(!isSceneVisited[i])
            {
                searchFinished = false;
            }
        }
        if(searchFinished)
        {
            state = Random;
        }
    }
    Vector<RawObject*> buttons = GraphicLayer::sharedLayer()->all([](RawObject* obj) -> bool {
        return obj->getNode() != NULL &&
            GraphicLayer::sharedLayer()->isWorldVisible(obj) &&
            !obj->getEventName().empty() &&
            obj->getEventName()[0] != '\0' &&
            obj->getEventActivated();
    });
    this->removeBadButtons(buttons);
    Vector<RawObject*> probableTargets;
    if(state == SearchScenes)
    {
        probableTargets = this->selectUnknownScenesSwitchButtons(buttons);
        if(probableTargets.size() == 0)
        {
            probableTargets = this->selectAllScenesSwitchButtons(buttons);
        }
        if(probableTargets.size() == 0)
        {
            probableTargets = buttons;
        }
    }
    else
    {
        probableTargets = buttons;
    }
    if(probableTargets.size() != 0)
    {
        RawObject* target = (RawObject*)probableTargets.at(arc4random() % probableTargets.size());
        GraphicLayer::sharedLayer()->touchAtPosition(GraphicLayer::sharedLayer()->getRealPosition(target), true);
    }
    iterations++;
    if(state == SearchScenes && iterations > MAX_SEARCH_ITERATIONS)
    {
        state = Random;
    }
}

void Monkey::init()
{
    state = SearchScenes;
    iterations = 0;
    isSceneVisited[0] = true;//None can't be visited
    for(int i = 1; i < SCENES_NUMBER; i++)
    {
        isSceneVisited[i] = false;
    }
}

void Monkey::removeBadButtons(Vector<RawObject*> buttons)
{
    Rect windowRect = Rect(0, 0, Director::getInstance()->getWinSize().width, Director::getInstance()->getWinSize().height);
    GraphicLayer* layer = GraphicLayer::sharedLayer();
    for(long i = buttons.size() - 1; i >= 0; i--)
    {
        RawObject* obj = buttons.at(i);
        //Remove objects which don't collide with windowRect. It uses code from Rect::intersectRect (which can't be used directly because a Rect origin must be > (0,0))
        Vec2 pos = layer->getRealPosition(obj);
        Size size = SizeMult(obj->getSize(), layer->getRealScale(obj));
        if(windowRect.getMaxX() < pos.x - obj->getNode()->getAnchorPoint().x * size.width ||
           pos.x + (1-obj->getNode()->getAnchorPoint().x) * size.width <      windowRect.getMinX() ||
           windowRect.getMaxY() < pos.y - obj->getNode()->getAnchorPoint().y * size.height ||
           pos.y + (1-obj->getNode()->getAnchorPoint().y) * size.height <      windowRect.getMinY())
        {
            buttons.eraseObject(obj);
        }
        else if(obj->getEventName() == "PickImage"
                || obj->getEventName() == "OpenKeyboard"
                || obj->getEventName() == "DoNothing"
                || obj->getEventName() == "OpenURL"
                || obj->getEventName() == "OpenMail"
                //|| (strcmp(obj->getEventName(), "PlanSceneSwitch") == 0 && TOINT(obj->getEventInfos()->objectForKey("Scene")) == (int)ManagementHome)
                //|| (strcmp(obj->getEventName(), "PlanSceneSwitch") == 0 && TOINT(obj->getEventInfos()->objectForKey("Scene")) == (int)EditUser)
           )
        {
            buttons.eraseObject(obj);
        }
    }
}

Vector<RawObject*> Monkey::selectAllScenesSwitchButtons(Vector<RawObject*> buttons)
{
    Vector<RawObject*> result;
    for(RawObject* obj  : buttons)
    {
        if(obj->getEventName() == "PlanSceneSwitch")
        {
            result.pushBack(obj);
        }
    }
    return result;    
}

Vector<RawObject*> Monkey::selectUnknownScenesSwitchButtons(Vector<RawObject*> buttons)
{
    Vector<RawObject*> result;
    for(RawObject* obj  : buttons)
    {
        if(obj->getEventName() == "PlanSceneSwitch"
           && !isSceneVisited[TOINT(obj->getEventInfos()->objectForKey("Scene"))])
        {
            result.pushBack(obj);
        }
    }
    return result;
}
