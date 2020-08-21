# Planeverb
Project Planeverb is a CPU based real-time wave-based acoustics engine tailored for games. 
It works in 2D, handling fully dynamic scenes in a single-CPU-core budget.
Technical details can be found in associated publication:

>Matthew Rosen, Keith W. Godin, Nikunj Raghuvanshi, <br/>**Interactive sound propagation for dynamic scenes using 2D wave simulation,** <br/>*SIGGRAPH/Eurographics Symposium on Computer Animation (SCA), 2020*

It comes with an integration with the Unity Engine, and a C-API for custom integrations.

## Code organization
There are two modules: Acoustics and DSP. 
The acoustics module runs the wave simulation in a background thread, updating dynamic per-source acoustic parameter data like obstruction, direction-of-arrival, and reverb, 
and the DSP module renders the parameters onto source audio in the audio thread. 
The DSP module doesn't have an implemented reverb, instead using Unity's built-in reverb. 

## Background
Planeverb was implemented for the class MUS470 taught by Prof. Matt Klassen at DigiPen Institute of Technology as an undergraduate senior capstone project, 
with guidance from Microsoft Principal Researcher [Nikunj Raghuvanshi](https://www.microsoft.com/en-us/research/people/nikunjr/).
It is based off of the ideas of Microsoft's [Project Triton](https://aka.ms/triton) technology used in [Project Acoustics](https://aka.ms/acoustics). In particular, the use of parametric encoding of wave simulation that model robust 
diffraction effects. But Project Acoustics does not allow arbitrary scenes changes, like destruction, because of its reliance on a "baking" step for offline high quality simulation. 

Planeverb is a forward-looking investigation into the question "What would it take to bring sound wave diffraction effects to fully dynamic scenes?"
These include smooth obstruction, sound re-direction around openings, source directivity, and dynamic reverb. 
We are able to show that if you restrict to two dimensions (like the slice of a floor plan or in an outdoor scene), 
it is possible to pull this off in real-time on a single CPU core. We hope the code serves as a useful proof-of-concept. 

## Commerical use / IP
While the code is released with an MIT license and is usable as such, please note that it uses intellectual property from [Project Acoustics](https://aka.ms/acoustics) covered in related Microsoft patents. 
If you wish to build on the code or ideas in it in a shipping commercial product, please contact @nikunjragh on the [Project Acoustics Github forum](https://github.com/microsoft/ProjectAcoustics/issues) to discuss further.

## Citing / Bibtex
Bibtex key to cite this work (we encourage you to):

```
@article{Rosen_Planeverb:2020,
 author = {Rosen, Matthew and Godin, Keith W. and Raghuvanshi, Nikunj},
 title = {Interactive sound propagation for dynamic scenes using 2D wave simulation},
 journal = {Computer Graphics Forum (Symposium on Computer Animation)},
 issue_date = {2020},
 volume = {39},
 number = {8},
 year = {2020}
}
```
