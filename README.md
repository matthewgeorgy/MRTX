# glsl-raytracer

This is my raytracing rendering engine (code-jokename: MRTX) written using C++ and GLSL compute shaders (__heavily__).

## Rationale

I spent quite a few months getting into computer graphics and graphics programming. I started with regular OpenGL and rasterization for my Solar System project (https://github.com/matthewgeorgy/solarsystem), and then decided to dedicate some time to learn and understand raytracing.
After reading "Raytracing in One Weekend" (hereafter referred to as 'RTIOW' by Peter Shirley (https://raytracing.github.io/), the goal for my next project was to build a simple raytracing engine that works in real-time; meaning that you can move and look around within the scene, rather than a static image that RTIOW generates.

## How + Challeneges

### OpenGL

One of the bigger hurdles that had to be overcome to build a raytracing engine using OpenGL was actually OpenGL itself. Unlike some newer API's like Direct3D 12 or Vulkan, OpenGL does not have any built-in raytracing support or features. Of course, this doesn't mean it's impossible to do raytracing (as many others on the internet have done it, including myself), but instead it meant that I was really on my own. Thankfully, however, OpenGL gives you a lot of control over what can be displayed on your screen, meaning that I had to take a "simpler" approach in order to make raytracing possible (you'll see why I put 'simpler' in quotes).

Let's think back for a minute about how video works: all a video is is just a continuous stream of images that produce the illusion of motion. This is something that we sometimes forget about or take for granted, even when it comes to the graphics on our screens. And what does RTIOW do, as I mentioned above? Well, it just produces a static image of a raytraced scene. Why do I bring this up? Well because the approach I decided to take, as described below, simply combines these 2 ideas!

1) First, we handle the user's input to retrieve necessary data (camera location, orientation, etc).
2) Then, we build a quad (rectangle) that fills the entire screen.
3) Using the given data described above, we produce a static raytraced scene, using a similar approach to RTIOW, and then store this image in a texture.
4) We then fill our fullscreen quad with this texture data for the user to see.
5) Render.
6) And then repeat for the next frame.

This is why I said that this approach is "simpler": it's a much more primitive technique than the simplicity of drawing geometry via rasterization, but it takes a bit more work to get up and running.

### RTIOW's Approach

RTIOW uses C++ for it's raytracing implemenation, which makes a lot of the programming quite nice actually. Things like operator overloading make dealing with vectors much easier (although we have them in GLSL so that doesn't count), and the use of inheritance, polymorphism, and `shared_ptr`'s make modelling different materials relatively simple. The issue, however, is that these features simply do not exist in GLSL, meaning that we, again, have to take a more primitive approach by using raw functions and conditionals to determine how a ray should behave with a particular material. 

Interestingly, however, I found that this more primitive approach greatly simplified + shrunk the code and made it more intuitive to reason about what is happening in the code. I also think that this approach puts more emphasis on the raytracing itself, rather than the inheritance + `shared_ptr` magic that RTIOW employs. That isn't to say that RTIOW's approach is a poor one - not at all; I just think that stripping out some of the C++ business cleans things up.

### Compute Shaders

Step 3 described above (generating the image texture) is evidently the most important, and was implemented in this engine by using GLSL compute shaders, rather than embedding this computation within the fragment shader. The reasoning for doing this is twofold:

1) Putting the raytracing computation within a compute shader helps to break up the rendering pipeline into clearer pieces. This becomes particularly important when we choose to have a variety of scenes and require multiple shaders. Putting the raytracing in the compute shader also makes the code less repetitive; the vertex shader ALWAYS builds the quad, and the fragment shader ALWAYS applies the texture. This means that we can have __1__ vertex+fragment program and then multiple compute programs for different scenes, rather than multiple fragment shaders that combine with the same vertex shader (
   Vertex (Quad) -> Compute (Raytrace) -> Fragment (Texture) -> Render      VS     Vertex (Quad) -> Fragment (Raytrace + Texture) -> Render
