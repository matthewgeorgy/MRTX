# glsl-raytracer

This is my raytracing rendering engine (code-jokename: MRTX) written using C++ and GLSL compute shaders (__heavily__).

![alt text](./preview.gif)
(very poor quality due to gyazo)

## [SCREENSHOTS](screenshots/screenshots.md)

## Rationale

I spent quite a few months getting into computer graphics and graphics programming. I started with regular OpenGL and rasterization for my Solar System project (https://github.com/matthewgeorgy/solarsystem), and then decided to dedicate some time to learn and understand raytracing.
After reading "Raytracing in One Weekend" (hereafter referred to as 'RTIOW') by Peter Shirley (https://raytracing.github.io/), the goal for my next project was to build a simple raytracing engine that works in real-time; meaning that you can move and look around within the scene, rather than a static image that RTIOW generates.

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

1. Putting the raytracing computation within a compute shader allows us to break up the pipeline into more specific pieces; namely, it allows us to separate the raytracing (step 3) from the texture application (step 4). This is very important for when we want to render the scene statically with a higher sample count, in which case we want to raytrace the image __ONCE__ and reuse the texture over multiple frames. Our pipeline then becomes this: Vertex (Quad) -> Compute (Raytrace) -> Fragment (Texture) -> Render

2. Performing the raytracing within a compute shader allows us to utilize the parallel computation capabilities of the GPU (which is what it excels at). This is possible because we calculate the color of each pixel individually (which is what RTIOW does), so parallelizing this computation greatly improves performance; in my case, the FPS increased by 10x when modifiying the compute shader to use multiple workgroups + invocations. 

### GLSL Loops

If you don't know, GLSL (and most shading languages as far as I know) don't deal with loops such as `for` and `while` very nicely. Why? I'm not 100% sure, as I don't know how GPU's internally execute instructions (and how GLSL translates into these instructions). This why that you may see code that looks something like this:

  ```
    scene.materials[i].type = MAT_LAMBERTIAN;
    scene.materials[i].albedo = vec3(1.0, 0.0, 0.0); 
    ...
    i++;
    
    scene.materials[i].type = MAT_LAMBERTIAN;
    scene.materials[i].albedo = vec3(1.0, 0.0, 0.0); 
    ...
    i++;
    
    .
    .
    .
  ```
  
This from the checkered texture example, as we require multiple spheres in order to create the scene. RTIOW does this by simply rolling this procedure into a loop and using RNG to determine the position, color, etc. of each sphere. We can't do this here because:

1) We don't want the spheres to randomly change position each frame; RTIOW can use RNG in this fashion because it only produces a static image.
2) Loops significantly decrease performance, hence why I've unrolled them like in the example above.
