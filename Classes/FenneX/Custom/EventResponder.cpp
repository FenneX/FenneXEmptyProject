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

#include "EventResponder.h"
#include "AnalyticsWrapper.h"


void EventResponder::planSceneSwitch(EventCustom* event)
{
    ValueMap infos = ((Value*)event->getUserData())->asValueMap();
    SceneName scene = (SceneName)infos["Scene"].asInt();
    AnalyticsWrapper::logPageView(formatSceneToString(scene));
}

static time_t lastClickTime = 0;

void EventResponder::keyBackClicked(EventCustom* event)
{
    time_t currentTime;
    time(&currentTime);
    double diff = difftime(currentTime, lastClickTime);
    RawObject* back = layer->first("BackButton");
    SceneName previous = getPreviousSceneName(currentScene->getSceneName());
    
    if((back != NULL || previous != None) && fabs(diff) > 1.0)
    {
        if(back != NULL)
        {
            layer->touchObject(back, true, layer->getRealPosition(back));
        }
        else
        {
            this->back(NULL);
        }
        lastClickTime = currentTime;
    }
    else
    {
        this->quitApp(event);
    }
}

void EventResponder::back(EventCustom* event)
{
    ValueMap infos = event != NULL && event->getUserData() != NULL && ((Value*)event->getUserData())->getType() == Value::Type::MAP ? ((Value*)event->getUserData())->asValueMap() : ValueMap();
    infos["Scene"] = Value(getPreviousSceneName(currentScene->getSceneName()));
    Value toSend = Value(infos);
    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("PlanSceneSwitch", &toSend);
}

void EventResponder::quitApp(EventCustom* event)
{
    log("Quitting application");
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    log("Stopping GA session on enter background");
    AnalyticsWrapper::endSession();
#endif
    Director::getInstance()->end();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}
