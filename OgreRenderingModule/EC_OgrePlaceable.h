// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_OgreRenderingSystem_EC_OgrePlaceable_h
#define incl_OgreRenderingSystem_EC_OgrePlaceable_h

#include "ComponentInterface.h"
#include "Foundation.h"

namespace Ogre
{
    class SceneNode;
}

namespace OgreRenderer
{
    class Renderer;
    class OgreRenderingModule;
    
    //! Ogre placeable component. Stores Ogre scenenode.
    class MODULE_API EC_OgrePlaceable : public Foundation::ComponentInterface
    {
        DECLARE_EC(EC_OgrePlaceable);
    public:
        virtual ~EC_OgrePlaceable();
        
        Core::Vector3df GetPosition() const;
        void SetPosition(const Core::Vector3df& position);
        
        Ogre::SceneNode* GetSceneNode() const { return scene_node_; }

    private:
        EC_OgrePlaceable(Foundation::ModuleInterface* module);

        OgreRenderingModule* module_;
        Ogre::SceneNode* scene_node_;
    };
}

#endif
