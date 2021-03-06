/******************************************************************************
* THE OMEGA LIB PROJECT
*-----------------------------------------------------------------------------
* Copyright 2010-2014		Electronic Visualization Laboratory,
*							University of Illinois at Chicago
* Authors:
*  Alessandro Febretti		febret@gmail.com
*-----------------------------------------------------------------------------
* Copyright (c) 2010-2013, Electronic Visualization Laboratory,
* University of Illinois at Chicago
* All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer. Redistributions in binary
* form must reproduce the above copyright notice, this list of conditions and
* the following disclaimer in the documentation and/or other materials provided
* with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*-----------------------------------------------------------------------------
* What's in this file:
*	An example of immersive multiview support in omegalib
******************************************************************************/
#include <omega.h>
#include <omegaGl.h>

#include "cube.h"

using namespace omega;

class HelloApplication;

///////////////////////////////////////////////////////////////////////////////
class HelloRenderPass: public RenderPass
{
public:
    HelloRenderPass(Renderer* client, HelloApplication* app) : 
        myApplication(app),
        RenderPass(client, "HelloRenderPass") 
    {}
	virtual void initialize();
	virtual void render(Renderer* client, const DrawContext& context);

private:
    Ref<Cube> myCube;
    HelloApplication* myApplication;
};

///////////////////////////////////////////////////////////////////////////////
class HelloApplication : public EngineModule
{
    friend class HelloRenderPass;
public:
	HelloApplication(): 
        EngineModule("HelloApplication"),
        myChangeView(false)
    {}

	virtual void initializeRenderer(Renderer* r) 
	{ 
        RenderPass* rp = new HelloRenderPass(r, this);
		r->addRenderPass(rp);
        rp->setCameraMask(myViewCamera[0]->getMask());
	}

    virtual void initialize()
    {
        myViewCamera[0] = getEngine()->createCamera();
        myViewCamera[0]->setMask(1 << 1);
        myViewCamera[0]->setViewPosition(0.0f, 0.0f);
        myViewCamera[0]->setViewSize(0.2f, 0.2f);
        myViewCamera[0]->clearColor(true);
        myViewCamera[0]->clearDepth(true);
        myViewCamera[0]->setBackgroundColor(Color::Blue);

        // Setup same as default camera
        myViewCamera[0]->setup(
            SystemManager::instance()->getSystemConfig()->lookup("config/camera"));

        myViewCamera[1] = getEngine()->createCamera();
        myViewCamera[1]->setMask(1 << 1);
        myViewCamera[1]->setViewPosition(0.3f, 0.0f);
        myViewCamera[1]->setViewSize(0.2f, 0.2f);
        myViewCamera[1]->clearColor(true);
        myViewCamera[1]->setBackgroundColor(Color::Navy);
        // Setup same as default camera
        myViewCamera[1]->setup(
            SystemManager::instance()->getSystemConfig()->lookup("config/camera"));

        myActiveView = myViewCamera[0];
    }

    virtual void handleEvent(const Event& evt)
    {
        if(evt.getServiceType() == Service::Keyboard)
        {
            if(evt.isFlagSet(Event::Alt)) myChangeView = true;
            else myChangeView = false;

            if(evt.isKeyDown('1')) myActiveView = myViewCamera[0];
            if(evt.isKeyDown('2')) myActiveView = myViewCamera[1];
        }
        else if(evt.getServiceType() == Service::Pointer &&
            evt.getType() == Event::Down)
        {
            if(myChangeView)
            {
                DisplayConfig& dcfg = getEngine()->getDisplaySystem()->getDisplayConfig();

                float x = evt.getPosition()[0];
                float y = evt.getPosition()[1];

                // Normalize
                x = x / dcfg.canvasPixelSize[0];
                y = y / dcfg.canvasPixelSize[1];

                if(evt.isFlagSet(Event::Left))
                {
                    myActiveView->setViewPosition(x, y);
                }
                else if(evt.isFlagSet(Event::Right))
                {
                    const Vector2f& vp = myActiveView->getViewPosition();
                    myActiveView->setViewSize(x - vp[0], y - vp[1]);
                }
            }
        }
    }

private:
    bool myChangeView;
    Camera* myActiveView;

    Ref<Camera> myViewCamera[2];
};

///////////////////////////////////////////////////////////////////////////////
void HelloRenderPass::initialize()
{
	RenderPass::initialize();
    myCube = new Cube(0.2f);
}

///////////////////////////////////////////////////////////////////////////////
void HelloRenderPass::render(Renderer* client, const DrawContext& context)
{
	if(context.task == DrawContext::SceneDrawTask)
	{
		client->getRenderer()->beginDraw3D(context);

		// Enable depth testing and lighting.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
	
		// Setup light.
		glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, Color(1.0, 1.0, 1.0).data());
		glLightfv(GL_LIGHT0, GL_POSITION, Vector3s(0.0f, 0.0f, 1.0f).data());

		// Draw a rotating cube.
		glTranslatef(0, 2, -2); 
		glRotatef(10, 1, 0, 0);
		glRotatef((float)context.frameNum * 0.1f, 0, 1, 0);
		glRotatef((float)context.frameNum * 0.2f, 1, 0, 0);
        
        myCube->draw();

		client->getRenderer()->endDraw();
	}
    // Draw a border around the view
    else if(context.task == DrawContext::OverlayDrawTask)
    {
        DrawInterface* di = client->getRenderer();
        DisplayConfig& dcfg = client->getDisplaySystem()->getDisplayConfig();
        di->beginDraw2D(context);
        
        Color c = Color::Gray;
        if(myApplication->myActiveView == context.camera)
        {
            glLineWidth(4.0f);
            c = Color::Fuchsia;
        }

        Vector2f vpos = context.camera->getViewPosition();
        vpos[0] *= dcfg.canvasPixelSize[0];
        vpos[1] *= dcfg.canvasPixelSize[1];

        Vector2f vsize = context.camera->getViewSize();
        vsize[0] *= dcfg.canvasPixelSize[0];
        vsize[1] *= dcfg.canvasPixelSize[1];

        di->drawRectOutline(
            Vector2f::Zero(),
            vsize,
            c);

        glLineWidth(1.0f);

        di->endDraw();
    }
}

///////////////////////////////////////////////////////////////////////////////
// ApplicationBase entry point
int main(int argc, char** argv)
{
	Application<HelloApplication> app("ohelloView");
    return omain(app, argc, argv);
}
