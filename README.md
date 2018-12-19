# Cover

This is the source code for the cover of my PhD dissertation.

Front page is an image of a 3D microwave cavity as this was the main experimental setup of my research.

<img src="data/181216_160354_v7.png" alt="front" width="500"/>

The back page is an image of a tree, simply because I think trees are beautiful! And they are very tricky to generate procedurally.

<img src="data/181216_160349_v7.png" alt="back" width="500"/>

## Implementation

I should mention that these are _real-time_ rendering techniques, where the necessity of rendering a frame within 15 ms requires you to take full advantage of hardware acceleration. Honestly, for a still image this method is a bit overkill. Interactive/procedural graphics is an odd obsession of mine so I had an interest in doing it this way. You could make something similar using normal 3D rendering software. 

- First, the tree geometry is calculated on the CPU using the [space-colonization algorithm](http://algorithmicbotany.org/papers/colonization.egwnp2007.large.pdf). I realized that the trick to make it look nice was to vary the density of the attraction points from low (the trunk) to high (the canopy) using a smooth function. This took some trial and error. The attraction points are generated randomly, so changing the seed of the random number generator will give a slightly different tree. (See [algo.cpp](src/algo.cpp))
- Once all the tree nodes are calculated they are sent to the GPU and rendered as a point sprites. I initially intended to go crazy with procedural textures for each particle, but in the end the thing that looked best was just a circle. It would have also required particle sorting or order-independent transparency which is either difficult to implement or incredibly slow.
- The tree is captured into an FBO such that it can be blurred slightly using a Gaussian blur stage.
- The background is rendered as a full-screen quad with a pixel shader that implements the sky, sun, clouds, stars, and water reflection using various distortions of gradient or value noise. The tree is manually blended into the scene. (see [back.frag](glsl/back.frag))
- This is again captured into an FBO for further processing. The buffer is presented as-is, however, if rendering the backcover.
- The back cover is flipped along the x-axis and blurred again over multiple stages. To achieve a high kernel blur I just ran the FBO through this stage 4 times. There are definitely better ways of doing this.
- Last stage is another full-screen quad where a 3D microwave cavity-looking mask is created and blended on top the the blurred background. I though it would be cool to create an effect as if you can see through the dissertation. Not very accurate though seeing as the sun is both behind and in front of the cover. Oh well, good enough.
- Some values such as color require a lot of fine-tuning -- having to recompile for each tweak would be a pain in the ass. So for some often-changed variables I would emit them into a .json file and reload them when called as a command line argument (i.e. `./cover v7`). You can toggle through the list of variables using your arrows and drag to change the values (see [config.cpp](src/config.cpp)). Press `p` to emit a PNG.

## Dependencies
- [GLFW3](https://github.com/glfw/glfw)
- [GLM](https://github.com/g-truc/glm)
- [GLEW](https://github.com/nigels-com/glew)
- [json.hpp](https://github.com/nlohmann/json)
- [stb_image_write.h](https://github.com/nothings/stb)
