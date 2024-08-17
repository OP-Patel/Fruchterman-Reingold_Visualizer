# Fruchterman-Reingold Visualizer

## What is the Fruchterman-Reingold algorithm? 
The Fruchterman-Reingold algorithm is a type of force-directed graph drawing algorithm designed to arrange the nodes of a graph in a two-dimensional or three-dimensional space in an aesthetically pleasing manner. It aims to ensure that the edges between nodes are of approximately equal length and that edge crossings are minimized. This is achieved by assigning attractive and repulsive forces to the edges and nodes based on their positions and then adjusting these positions to simulate motion or minimize the overall energy of the system (this is measured through system temperature and cooling factors), resulting in a more visually appealing graph layout.

## The Visualizer
The visualizer is fully programmed in C using SDL2 (Simple DirectMedia Layer) for interactivity and visuals. 

### Keybind
| Key(s) | Action | 
| ------------- | ------------- |
| WASD | Grid panning, allows for moving the demo grid and nodes |
| Mouse Scoll | Adjusts zoom level on demo grid |
| LEFT/RIGHT Arrow Keys | One frame adjustments (rev or fwd) |
| Space Bar | Play/Pause |

There will be auto-generated nodes and edges on the visualizer upon starting. To generate new randomized node locations, click the "Generate Nodes" button. 

### Usage

Make sure you have both SDL2 and SDL2_ttf installed on your system. You can usually install them via your package manager 
<br> i.e MacOS using ``` brew install sdl2 sdl2_ttf ```. Also ensure you have a C compiler and make installed or you may have to do some adjustments to the MakeFile.

Start by cloning and navigating to the project,  
```
git clone https://github.com/OP-Patel/Fruchterman-Reingold_Visualizer.git
cd Fruchterman-Reingold_Visualizer
```
Then simply write, 
```
make
./build/debug/play
```


## Examples
Below are some examples of the Fruchterman-Reingold algorithm at play using different input graphs.  

| Fruchterman-Reingold  | GraphViz |
| ------------- | ------------- |
| ![GIFMaker_me (1)](https://github.com/user-attachments/assets/b65fb3d7-8791-41a8-986c-9c6f9930d037)| Large-scale Graph![image](https://github.com/user-attachments/assets/edb0c257-e9cd-4277-816f-99f705580746) | 
| ![GIFMaker_me (2)](https://github.com/user-attachments/assets/1e260eb8-7ae6-4707-b00a-187ad04de36c) | K6 Graph <br> ![image](https://github.com/user-attachments/assets/e1c98fd6-9600-480b-99c7-42f507666756)|
|  ![GIFMaker_me (3)](https://github.com/user-attachments/assets/d665aa93-aa3a-4a88-b217-8f890b26f2ca) | Binary Tree <br> ![image](https://github.com/user-attachments/assets/9b961e22-76f1-4564-b61d-b940e9d8786f)|
