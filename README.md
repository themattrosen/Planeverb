# Planeverb
Project Planeverb is a CPU based real-time wave-based acoustics engine for games.
It is a proof of concept piece with a publication in the works for the Audio Engineering Society.  
It comes with an integration with the Unity Engine, and a C-API for custom integrations.
  
There are two modules: Acoustics and DSP. 
The acoustics module runs the wave simulation in a background thread, 
and the DSP module uses the acoustics module output during the audio thread run-time. 
The DSP module doesn't have an implemented reverb, so it makes use of Unity's built in reverb. 
  
Planeverb was implemented for the class MUS470 at DigiPen Institute of Technology as an undergraduate senior capstone project.
It is based off of the work of Microsoft Project Acoustics, 
with guidance from Microsoft Principal Researcher Nikunj Raghuvanshi. 
However, it handles a major case that Project Acoustics does not handle: dynamic, moving scene geometry. 
This is done with 100% utilization of the CPU to process the wave simulation in real-time. 
Because this CPU usage is unfeasible for most games, this is a proof of concept for future work. 
  
More work will be done with this get reverb implemented within the DSP module, add a GPU acoustics implementation, integration into Unreal 4, and other currently unimplemented features.
