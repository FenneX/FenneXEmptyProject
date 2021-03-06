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

#include "LayoutHandler.h"
#include "Monkey.h"
#include "AppMacros.h"
#include "editor-support/cocosbuilder/CocosBuilder.h"
#include "FenneXCore.h"
#include "FenneXWrappers.h"
#include "AppDelegate.h"

USING_NS_FENNEX;

//this is the custom scene creation. The corresponding method are in LayoutHandler.h, and are private

void LayoutHandler::createSceneGraphics(Scene* target)
{
#if VERBOSE_PERFORMANCE_TIME
    timeval startTime;
    gettimeofday(&startTime, NULL);
#endif
	log("Starting scene creation");
    
    //Stop sound and all delayed play sound
    AudioPlayerRecorder::sharedRecorder()->stopPlaying();
	switch (target->getSceneName())
	{
        case Home:
            //Your first scene, do your app logic here
            break;
        default:
            //Scenes that don't require any logic can be automatically loaded from their name
            loadCCBFromFileToFenneX(string_format("ccbi/%s", formatSceneToString(target->getSceneName())));
            break;
    }
    
    //You can add some global code here, for example adding Touch Recognizers
    
#if VERBOSE_PERFORMANCE_TIME
    timeval endTime;
    gettimeofday(&endTime, NULL);
    log("Scene %s loaded in %f ms", formatSceneToString(target->getSceneName()), getTimeDifferenceMS(startTime, endTime));
#endif
}

#define ADD_LAYOUT_OBSERVER(func, notifName) (listeners.pushBack(Director::getInstance()->getEventDispatcher()->addCustomEventListener(notifName, std::bind(&EventResponder::func, responder, std::placeholders::_1))))
void LayoutHandler::catchEvents(Scene* target)
{
    //Uncomment the following line to have a monkey tap everywhere. It catchs some types of bug very quickly!
    //target->addUpdatable(Monkey::excitedMonkey());
    
    ADD_LAYOUT_OBSERVER(keyBackClicked, "KeyBackClicked");
    ADD_LAYOUT_OBSERVER(back, "Back");
    ADD_LAYOUT_OBSERVER(quitApp, "QuitApp");
    ADD_LAYOUT_OBSERVER(planSceneSwitch, "PlanSceneSwitch");
    switch(target->getSceneName())
    {
        case Home:
            //Add the events you want to respond to here. ADD_OBSERVER redirect them to EventResponder
            break;
        default:
            break;
    }
}
