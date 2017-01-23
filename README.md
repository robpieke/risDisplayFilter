# risSampleFilter
Modular path tracer written using RIS sample filters in PRMan - (Stupid RenderMan Trick 2016)

## Motivation & Caveats

In early 2016, I ran across Peter Shirley's excellent [Ray Tracing in One Weekend](http://in1weekend.blogspot.co.uk/2016/01/ray-tracing-in-one-weekend.html) and set about implementing it in C++. A few months later it was time to think about a Stupid RenderMan Trick, and I started reading the release notes of PRMan 20. One of the new additions was a plugin API for reading/writing samples from the image buffers, typically to allow for colour grading etc. One interesting feature of the API however is that filters could reference each other, effectively allowing for a modular image-processing system to be built. I thought it would be an interesting stress-test of the system to try and write "filters" for each class in a ray tracer, from the integrator to the geometry to the materials and lighting. Ultimately it worked, and gave me some interesting insights into some of the challenges modern path tracing engines face when dealing with large ray batch sizes.

## Pre-requisites

To build the code, you'll need to first [download Pixar's RenderMan](https://renderman.pixar.com/view/get-renderman). Any version later than 20.0 should be fine.
