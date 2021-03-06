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

#include "AppDelegate.h"
#include "AppMacros.h"
#include "editor-support/cocosbuilder/CocosBuilder.h"
#include "FenneXCore.h"
#include "FenneXWrappers.h"
#include <vector>
#include <string>

using namespace std;
USING_NS_CC;


AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate() 
{
}

void AppDelegate::initGLContextAttrs()
{
    //set OpenGL context attributions,now can only set six attributions:
    //red,green,blue,alpha,depth,stencil
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8};
    
    GLView::setGLContextAttrs(glContextAttrs);
}

void AppDelegate::loadAnalytics()
{
    AnalyticsWrapper::setAppVersion(getAppVersionNumber());
    log("Set app version to %s", getAppVersionNumber().c_str());
    
#if DEBUG_ANALYTICS
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    AnalyticsWrapper::GAStartSession("YOUR-GA-KEY");
    log("start GA iOS debug session");
#endif
    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    AnalyticsWrapper::GAStartSession("YOUR-GA-KEY");
    log("start GA Android debug session");
#endif
#else
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    AnalyticsWrapper::GAStartSession("YOUR-GA-KEY");
    log("start GA iOS release session");
#endif
    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    AnalyticsWrapper::GAStartSession("YOUR-GA-KEY");
    log("start GA Android release session");
#endif
#endif
    AnalyticsWrapper::setDebugLogEnabled(VERBOSE_ANALYTICS);
}

void AppDelegate::initAppModules()
{
    //The scene switcher have to be initialized BEFORE most things (to get PlanSceneSwitch event before)
    SceneSwitcher::sharedSwitcher();
    //Disable pop up notify for fail (for example at resume, it will try to load some delete assets if you remove a custom photo
    FileUtils::getInstance()->setPopupNotify(false);
    
    Value settingsValue = loadValueFromFile("last_settings.plist");
    ValueMap settings = isValueOfType(settingsValue, MAP) ? settingsValue.asValueMap() : ValueMap();
    bool firstLaunch = !isValueOfType(settingsValue, MAP) || !isValueOfType(settingsValue.asValueMap()["FirstLaunch"], BOOLEAN) || settingsValue.asValueMap()["FirstLaunch"].asBool();
	if(firstLaunch)
	{
		
        std::string language = "Language: " + getLocalLanguage();
        log("%s", language.c_str());
        AnalyticsWrapper::logEvent(language);
        settings["Language"] = Value(getLocalLanguage());
        settings["FirstLaunch"] = Value(false);
    }
    else if(getLocalLanguage() != settings["Language"].asString())
    {
        std::string language = "Change to language: " + getLocalLanguage() + ", previous language: " + settings["Language"].asString();
        log("%s", language.c_str());
        AnalyticsWrapper::logEvent(language);
        settings["Language"] = Value(getLocalLanguage());
	}
    settingsValue = Value(settings);
    saveValueToFile(settingsValue, "last_settings.plist");
    
    //Initialize Audio session (not in recording mode at startup)
	AudioPlayerRecorder::sharedRecorder();
}

bool AppDelegate::applicationDidFinishLaunching()
{
	vector<string> searchPath;

	loadAnalytics();
    Director* pDirector = Director::getInstance();
    GLView* pEGLView = Director::getInstance()->getOpenGLView();

    pDirector->setOpenGLView(pEGLView);
    
	Size frameSize = pEGLView->getFrameSize();
    // Set the design resolution
    pEGLView->setDesignResolutionSize(frameSize.width, frameSize.height, kResolutionShowAll);

    //Default advised settings for ipadhd(100%)/ipad(50%)/iphone(21%)
    // if the frame's height is larger than the height of medium resource size, select large resource.
    // the selection is done using preset values to have the best directory selected on every tablet (for example, ipad retina size on 1280x800 isn't a good option)
	if (frameSize.width > 1280 && frameSize.height > 800)
	{ 
		searchPath.push_back(largeResource.directory);
        CCBReader::setResolutionScale(1/MAX(largeResource.size.width/frameSize.width, largeResource.size.height/frameSize.height));
        CCBLoaderSetScale(MIN(largeResource.size.width/designResolutionSize.width, largeResource.size.height/designResolutionSize.height));
	}
    // if the frame's height is larger than the height of small resource size, select medium resource.
    else if (frameSize.width > 640 && frameSize.height > 480)
    {
        searchPath.push_back(mediumResource.directory);
        CCBReader::setResolutionScale(1/MAX(mediumResource.size.width/frameSize.width, mediumResource.size.height/frameSize.height));
        CCBLoaderSetScale(MIN(mediumResource.size.width/designResolutionSize.width, mediumResource.size.height/designResolutionSize.height));
    }
    // if the frame's height is smaller than the height of medium resource size, select small resource.
	else
    { 
		searchPath.push_back(smallResource.directory);
        CCBReader::setResolutionScale(1/MAX(smallResource.size.width/frameSize.width, smallResource.size.height/frameSize.height));
        CCBLoaderSetScale(MIN(smallResource.size.width/designResolutionSize.width, smallResource.size.height/designResolutionSize.height));
    }
	FileUtils::getInstance()->setSearchPaths(searchPath);
    //If it's a phone, CCBLoader will automatically try to load ccb finishing by -phone first
    CCBLoaderSetPhoneLayout(isPhone());
    log("Scale factor for director : %f, for CCBReader : %f, resolution : %f, %f, resource directory : %s", pDirector->getContentScaleFactor(), CCBReader::getResolutionScale(), frameSize.width, frameSize.height, searchPath[0].c_str());
	
    // turn on display FPS
#ifdef BUILD_VERSION
    pDirector->setDisplayStatsWithBuild(DISPLAY_PERF_STATS, STRINGIFY(BUILD_VERSION));
#else
    pDirector->setDisplayStats(DISPLAY_PERF_STATS);
#endif

    // set FPS. the default value is 1.0/60 if you don't call this
    pDirector->setAnimationInterval(1.0 / 60);
    
    //Run the first scene. The splashscreen is runned outside of OpenGl View. It is custom on Android, you need to initialize it.
    AppDelegate::initAppModules();
    SceneSwitcher::sharedSwitcher()->initWithScene(FIRST_SCENE);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    discardSplashScreen();
#endif
    return true;
}

// This function will be called when the app is inactive. When comes a phone call,it's be invoked too
void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();
    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("AppEnterBackground");
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    log("Stopping GA session on enter background");
    AnalyticsWrapper::endSession();
#endif
    // if you use SimpleAudioEngine, it must be pause
    // SimpleAudioEngine::sharedEngine()->pauseBackgroundMusic();
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    log("Starting GA session on enter foreground");
    loadAnalytics();
#endif
    // if you use SimpleAudioEngine, it must resume here
    // SimpleAudioEngine::sharedEngine()->resumeBackgroundMusic();
}

void AppDelegate::applicationDidReceiveMemoryWarning()
{
    log("Removing unused textures ...");
    Director::getInstance()->getTextureCache()->removeUnusedTextures();
    log("Unused textures removed!");
    /*
     Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
     */
}
