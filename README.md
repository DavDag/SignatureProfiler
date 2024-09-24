# SignatureProfiler

This is a C++ Run-Time profiler I built for didactic purposes.<br>
You should be able to clone the repo and launch the project using the provided .sln since every dependency is also inside the repo.<br>

The repo contains the Library folder and 3 examples but I suggest you to look at a more concrete application [here](https://github.com/DavDag/Cpp-3D-Cellular-Automata).<br>
It's intended use is for Game profiling, thus it's built to be thread-safe, have minimal inpact and provide profiling info about frame-related data.<br> 

### Features

<ul>
  <li>Just by adding the library, all the code will be inspected automatically :)</li>
  <li>You can get statistics and information at runtime</li>
  <li>You can enable / disable the library at runtime</li>
  <li>You can annotate when a frame starts and when a frame ends</li>
  <li>You can display the profiling data using the built-in function for ImGui (its optional)</li>
</ul>

### Simple Console Sample
![image](https://github.com/user-attachments/assets/c72f9ebc-55fe-4d11-b060-461c0c47e2b7)

### Multi-Threaded Console Sample
![image](https://github.com/user-attachments/assets/cb32ddd0-4050-4567-8709-e39fef9ed34a)

### Simple UI Sample
![image](https://github.com/user-attachments/assets/7f62fbf4-61f3-4546-9266-a3c81676195d)

### Advanced UI Sample
https://github.com/DavDag/Cpp-3D-Cellular-Automata
