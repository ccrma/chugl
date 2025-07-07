# Changes

## 0.2.5 (alpha)

- Changed `TextureDesc.mips()` semantics. Set to `true` to generate a full mip chain, set to `false` to not create any mips beyond level 0.
- Add `TextureDesc.resizable`, `TextureDesc.widthRatio` and `TextureDesc.heightRatio` to create resizable textures, i.e. textures that change in dimensions relative to window resolution. (helpful to make your app robust against changing screen sizes)
- finish basic implementation of ScenePass 
- Reworked ChuGL's render pass API. `RenderPass` is now the base class of `ScenePass` (which renderas a GScene to a target texture, formerly this was called `RenderPass`) and `ScreenPass` (which renders a full-screen quad, useful for post processing effects)
  - Added new methods `RenderPass` methods `.clear()`, `.scissor()`, `.scissorNormalized()`, `.viewport()` and `.viewportNormalized()` 
  - See example [TODO]
- change default surface texture to be non-srgb, add `OutputPass.gamma()` to control whether or not to apply gamma correction.
  - this simplifies getting accurate colors in basic 2D scenes without complex lighting. Before, all colors would get rendered into an srgb buffer meaning the user would have to manually convert them into *linear* space in their code. Now a simple `ScenePass` that outputs directly to the graphics window will preserve color values. For more details, read [this](https://medium.com/@tomforsyth/the-srgb-learning-curve-773b7f68cf7a).
- Add resolution, time, delta_time, frame_count, mouse state, and chuck VM sample rate info to shader #FRAME_UNIFORMS. Simplifies writing generative shaders in shadertoy-style.
- Add alpha-blend transparency. See examples/basic/transparency.ck
- Add antialias method GText.antialias()
- Add UI.knob(...) widgets
- Bug fixes
  - fix `GText.defaultFont()` not setting custom font path correctly
  - fix segfault caused by sending large amount of data to `UI.plotLines()`
  - setting `TextureLoadDesc.gen_mips` to false no longer erroneously generates a mip chain
- Under the Hood
  - Large architecture refactor to use a rendergraph + gpu resource cache + sorted drawcall structs to organize and dispatch GPU commands.
    - Inspired by [this blog](https://blog.mecheye.net/2023/09/how-to-write-a-renderer-for-modern-apis/) and the [noclip.website renderer](https://github.com/magcius/gfxrlz)
    - Caching GPU resources (namely BindGroups and TextureViews) significantly improves performance and increases the maximum number of draw calls that can be issued each frame
      - See here [TODO] for a performance analysis of the cost of recreating bindgroups every frame in WebGPU
    - 