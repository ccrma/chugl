# ChuGL Workflow

Hoping this can become a developer runbook for future ChuGLers.

## Adding a New Feature

For example, adding support for 2DTextures

- Add frontend representation to CGL scenegraph. `CGL_Texture.xx`
  - this representation is the minimum amount of CPU-side data needed for the renderer to later use render this texture on the GPU
- Add Create/Update/Destroy Commands to `Command.h`. These encapsulate changes made to the Texture via chuck on the main audio thread, for later use by the render thread to synchronize its CGL scenegraph
- Create setters/getters through chugin interface to work with the Texture scenegraph class via chuck scripts (`ulib_cgl.cpp`)
  - all state modification needs to push a corresponding command to the command queue, e.g. `UpdateTextureCommand`
- create corresponding Renderer representation. Renderer needs to handle Creation/Update/Destroy commands and make the corresponding updates to GPU-side data
  - scenegraph geometry <--> `RenderGeometry`
  - scenegraph material <--> `RenderMaterial`
  - `CGL_Texture` <--> `Texture`
- Write ChucK code to unit test the new feature!

TODO: would be amazing if parts of this could be automated with codegen. 