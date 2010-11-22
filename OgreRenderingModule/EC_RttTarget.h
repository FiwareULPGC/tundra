// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_OgreRenderer_EC_RttTarget_h
#define incl_OgreRenderer_EC_RttTarget_h

#include "StableHeaders.h"
#include "IComponent.h"
#include "Declare_EC.h"
#include "Core.h"


namespace OgreRenderer { class OgreRenderingModule; };

/// Ogre render-to-texture component
/**
<table class="header">
<tr>
<td>
<h2>RttTarget</h2>
Render-to-texture target: renders a view from a camera to a texture.
Registered by OgreRenderer::OgreRenderingModule.

<b>Attributes</b>:
<ul>
<li>Cameraref? Or uses cam in the same entity?
<div>Name of the compositor (Ogre resource name), f.ex. "HDR"</div>
<li>QString targettexture
<div>Name of the target texture where to render the image
</ul>

<b>Exposes the following scriptable functions:</b>
<ul>
<li>...
</ul>

<b>Reacts on the following actions:</b>
<ul>
<li>...
</ul>
</td>
</tr>

Does not emit any actions.

<b>Depends on a camera component.</b>.
</table>
*/
class EC_RttTarget : public IComponent
{
    Q_OBJECT
    
    DECLARE_EC(EC_RttTarget);
public:
    //Q_PROPERTY(QString compositorref READ getcompositorref WRITE setcompositorref);
    //DEFINE_QPROPERTY_ATTRIBUTE(QString, compositorref);

    Q_PROPERTY(QString targettexture READ gettargettexture WRITE settargettexture);
    DEFINE_QPROPERTY_ATTRIBUTE(QString, targettexture);

    virtual ~EC_RttTarget();

    //! Set component as serializable.
    virtual bool IsSerializable() const { return true; }

private slots:
    void AttributeUpdated(IAttribute* attribute);
    //void UpdateRtt();
    void SetupRtt();

private:
    //! constructor
    /*! \param module Ogre module
     */
    EC_RttTarget(IModule* module);

    //! Owner module of this component
    //OgreRenderer::OgreRenderingModule *owner_;

    Ogre::TexturePtr tex;
    //void ScheduleRender();
};

#endif
