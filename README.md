# Maze Lab — Smart Maze Solver

An interactive DAA project website for visualizing and comparing BFS, DFS, Dijkstra, and A* pathfinding on generated mazes. The algorithms run in a native C++ backend; the browser renders and animates the backend's search results.

## Requirements

- Windows with MinGW-w64 (`g++`) or another C++17 compiler
- A modern browser

## Build and run (MinGW)

From the project root in PowerShell:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -o maze_server.exe backend/main.cpp -lws2_32
.\maze_server.exe
```

Open <http://localhost:8080>. To use a different port, pass it to the server, for example ` .\maze_server.exe 9000`.

The server serves the website from `web/` and accepts `POST /api/solve` requests. It is intentionally single-threaded and intended for a low-traffic classroom/demo environment.

## Deploy online (Render with Docker)

The repository includes a `Dockerfile` that builds the C++ backend on Linux and packages it with the website. To publish it using Render:

1. Push this project folder to a GitHub repository.
2. In Render, choose **New → Web Service** and connect that repository.
3. Select **Docker** as the runtime and keep the repository root as the build context.
4. Create the service. Render supplies the `PORT` environment variable; the backend listens on it automatically.
5. Open the public `onrender.com` URL after the first build completes.

The free service may sleep when idle and take a short time to wake up. This project uses a small custom HTTP server for its course demonstration; treat any public deployment as a demo, not a production service. Do not send sensitive data to it.

## Features

- Random perfect maze generation in the browser using recursive backtracking
- C++ implementations of BFS, DFS, Dijkstra, and A* with Manhattan distance
- Animated exploration and final path, plus visited nodes, path length, and solver time
- Adjustable maze size and playback speed; click or drag to draw/remove walls
- Responsive layout and fullscreen maze view

## Project structure

- `backend/main.cpp` — small HTTP server and pathfinding API
- `web/index.html` — project presentation and interactive UI
- `web/styles.css` — responsive visual design
- `web/app.js` — maze generation, controls, API calls, and animation

## Notes

The project facts and algorithm comparison are adapted from the supplied Smart Maze Solver DAA presentation. A* and Dijkstra use unit-cost movement in the current demo; weighted terrain can be added as an extension. Solver timings are local measurements and may vary by computer.
