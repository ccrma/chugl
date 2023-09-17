# To scenegraph or not to scenegraph


## Arguments on Why NOT to scenegraph


[TomF's Tech Blog](https://tomforsyth1000.github.io/blog.wiki.html#%5B%5BScene%20Graphs%20-%20just%20say%20no%5D%5D)
- world is not a scenegraph, e.g. coffee mug is not actually a child of the table it sits on
- not optimal, modern graphics cards don't care about minimal state change between edges of scenegraph
- graphics uses tree and graph structures everywhere--skeletons, spatial partitioning, lists of meshes that use the same shader/texture etc..
  - all these subgraphs are different in meaning and structure and therefore don't all fit into one big scenegraph
  - teh resulting scenegraph often turns out to be filled with hacks/loopholes...(kind of already feeling this with ChuGL)
- Scenegraphs are **retained mode** and note **immediate mode**
  - retained mode: renderer/graphics API retains the state
    - more plug-and-play, user doesn't have to implement parent/child transform inheritance
    - extra memory
    - for ChuGL: synchronizing 2 scenegraphs across frame yuck....
  - immediate mode: procedural. each time a new frame is drawn, application directly issues the drawing commands. graphics library does NOT store a scene model between frames; that responsibility is up to the user application
    - less memory footprint
    - more flexible
    - more potential for optimization
    - cons: user must do extra bookeeping themselves
  - immediate mode is what imGUI uses
    - [Casy Muratori blog post](https://caseymuratori.com/blog_0001)


## Alternatives

[DAGs instead???](http://gamearchitect.net/Articles/GameObjects3.html)

## API Thoughts

Scenegraph synchronization architecture arises because we are training to do a retained-mode, scenegraph-driven renderer in the first place. What happens if we do away with the scenegraph, and have an immediate mode 3D graphics API?
- probably still need the command queue double buffer to parallize update and render logic
  - depends how webgpu draw commands work actually
- BUT the contents of command queue will be actual draw calls, and chugl backend no longer needs to maintain any scenegraph state
- instead, state is maintained by the chuck programmer, however they want
  - most applications probably won't even need a scenegraph
  - also lets people program different kinds of scenegraphs (e.g. ones where transformations/shaders etc can themselves be nodes that affect all children)
    - scenegraph and retained mode offer convenience at the cost of forcing a way of thinking/coding
      - (also polymorphism and OOP is largely, in the words of Casey Muratori, "a load of horseshit")
    - but chugl isnt about convenience, and convenience anyways is a sidesffect of a simple but powerful/clean/lean/flexible API that empowers chuckerd to make their own abstractions.

Instead, emphasis of chugl can be on procedural, reactive, shaders and such, doing new things!!
less code, not more. direct and immediate

We want the API to feel like LOVE:
- in the background, it understands you (you dont have to understand it) 
- abstract enough you don't have to waste time writing uncreative boilerplate
- but ALSO simple enough that when using it you are forced to think for urself, how do i want to code my system, do i want a scenegraph or not etc, do i want polymorphism or not
  - creating your own libraries/helper utilities camera class, tweening class etc in chuck itself!! rather than cpp dlls or whatmot. carry this code through to future projects and share it with others.
  - move as much of the potentially-creative programming to chuck script, and have a minimal cpp rendering backend
  - API should assume nothing about programmers intent other than they have the passion to do things in their own way


## Musings

ChucK (and Nyquist, supposedly) is one of the first computer music programming languages to integrate sound synthesis and musical events (e.g. MSP and Max, audio signal and control signals) into a single programming environment.

Something similar can be said about ChuGL and it's integration of sound and graphics control/synthesis.

Reminded of [Conway's law](https://en.wikipedia.org/wiki/Conway%27s_law) which states that the structure of a product and the structure of the organization that produces said product are inexorably tied together. E.g., Microsoft's DirectX API has different interfaces for DirectSound, Direct2D, Direct3D, because that is exactly how the teams are organized: there's a sound team, 2d and 3d team. This happens because the moment you create a separation in the organization, a division of labor, communication is stymied across that link, and any possibility of integrated/intersectional design is lost. 

As Conway's Law predicts, because Audio and Graphics are always(?) separate divisions of an organization, the tools and APIs etc have always evolved to be separate. And this separation is evinced in every layer from the team organization, individual profressions, to the tools and the work flows. E.g. when making a game, the SFX artists and composer work completely separate from the actual game designers using different tools, and it's only towards the very end of code completion that audio is finally added on. And that makes perfect sense because it's the only kind of workflow that current tools facilitate

But what if it weren't? What if from the get-go, Microsoft, say, only had an "audiovisual" team and they had to create a unified API to control both sound and visuals, in tandem? What would that even look like? Would it be some abstraction of "signal" pipelines that could filter/transform/render to both a framebuffer and an audiobuffer? Would there be a distinction between framebuffers and audiobuffers (aren't they both buffers at the end of the day? Would we get realisic sound simulation from our rendering engines in the exact same way we get realistic lighting/collision too (recall Doug James research). Would there have arisen highly stylized "audio" language analogous to visuals, e.g. you can have toon-shaders for light and materials, but what does a toon-filter for acoustic properties sound like? Would our ears be able to expand and accept new sonic world for interaction? 
- obviously it makes sense to separate sound and visuals, (they are two separate sensory modes) but there are many similarities!
- many of the concepts of signal processing, frequency domain analysis, transforms, filters, spatialization, have common ground between these two fields

ChuGL is a step in that direction of breaking down an age-old wall. It's like for the first time, audio and graphics and decided to move in together as roomates (whereas before they could only talk over the phone) It might turn out to be a terrible idea, but even if it fails, it will undoubtedly reveal new insights about the personality of each. Closeness between any two things is a closeness in terms of communication. The more friction there is, the more they separate (this is true for people and programs, organic and inorganic matter...). But the more there is crosstalk, interchange, mutual dependence and understanding...two becomes one. And what does that one look like? It might not look any different at all. It might just be a slightly more convenient way to arrive at the same end. It might also be a slightly more *inconvenient* way to arrive at the end. But it might be that a reaction occurs which opens possibility of creating something fundamentally different, 





