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

#include "Panel.h"
#include "Shorteners.h"
#include "GraphicLayer.h"
#include "AppMacros.h"

NS_FENNEX_BEGIN
Node* Panel::getNode()
{
    return delegate;
}

void Panel::setNode(Node* node)
{
    node->setPosition(delegate->getPosition());
    node->setScaleX(delegate->getScaleX());
    node->setScaleY(delegate->getScaleY());
    node->setContentSize(delegate->getContentSize());
    node->setAnchorPoint(delegate->getAnchorPoint());
    Vector<Node*> childrenNode;
    //Deep copy because by default it's a reference
    for(Node* child : delegate->getChildren())
    {
        childrenNode.pushBack(child);
    }
    delegate->removeAllChildrenWithCleanup(false);
    for(Node* child : childrenNode)
    {
        node->addChild(child);
    }
    delegate->getParent()->addChild(node);
    delegate->removeFromParentAndCleanup(true);
    delegate->release();
    
    delegate = node;
    delegate->retain();
}

void Panel::setClippingNode()
{
    ClippingNode* clipNode = ClippingNode::create();
    this->setNode(clipNode);
    clipNode->setSquareStencil();
}

Panel::Panel(std::string panelName, Vec2 location)
{
    delegate = Node::create();
    delegate->retain();
    delegate->setPosition(location);
    name = panelName;
}

Panel::Panel(Node* node, std::string panelName)
{
    name = panelName != "" ? panelName : "Panel";
    delegate = node;
    delegate->retain();
}

Panel::~Panel()
{
    delegate->release();
#if VERBOSE_DEALLOC
    log("Dealloc Panel %s", name.c_str());
#endif
}

void Panel::addChild(RawObject* child)
{
    if(child->getNode() != NULL)
    {
        children.pushBack(child);
        if(delegate->getChildren().empty() || !delegate->getChildren().contains(child->getNode()))
        {
            delegate->addChild(child->getNode());
        }
    }
    else
    {
        log("Warning : child %s doesn't have a Node, you shouldn't try to place it on a panel", child->getName().c_str());
    }
}

bool Panel::containsObject(RawObject* child)
{
    return children.contains(child);
}

void Panel::removeChild(RawObject* child)
{
    if(child->getNode() != NULL)
    {
        children.eraseObject(child);
        delegate->removeChild(child->getNode(), false);
    }
    else
    {
        log("Warning : child %s doesn't have a Node, you shouldn't try to remove it from panel", child->getName().c_str());
    }
}

void Panel::reorderChild(RawObject* child, int zOrder)
{
    if(this->containsObject(child))
    {
        delegate->reorderChild(child->getNode(), zOrder);
    }
}

Vector<RawObject*> Panel::getChildren()
{
    return children;
}

void Panel::update(float deltaTime)
{
    
}

bool Panel::collision(Vec2 point)
{
    if(this->getSize().width == 0 && this->getSize().height == 0)
    {
        return true;
    }
    else if(isValueOfType(eventInfos["InfiniteScrolling"], BOOLEAN) && eventInfos["InfiniteScrolling"].asBool())
    {
        bool vertical = isValueOfType(eventInfos["isVertical"], BOOLEAN) && eventInfos["isVertical"].asBool();
        if(vertical
           && point.x > this->getPosition().x - this->getSize().width * this->getNode()->getAnchorPoint().x  * this->getScaleX()
           && point.x < this->getPosition().x + this->getSize().width * (1-this->getNode()->getAnchorPoint().x)  * this->getScaleX())
        {
            return true;
        }
        else if(!vertical
                && point.y > this->getPosition().y - this->getSize().height * this->getNode()->getAnchorPoint().y * this->getScaleY()
                && point.y < this->getPosition().y + this->getSize().height * (1-this->getNode()->getAnchorPoint().y) * this->getScaleY())
        {
            return true;
        }
        return false;
    }
    return RawObject::collision(point);
}

bool Panel::collision(Rect rect)
{
    if(this->getSize().width == 0 && this->getSize().height == 0)
    {
        return true;
    }
    return RawObject::collision(rect);
}

bool Panel::containsRect(Rect rect)
{
    
    if(this->getSize().width == 0 && this->getSize().height == 0)
    {
        return true;
    }
    return RawObject::containsRect(rect);
}
NS_FENNEX_END
