# IPC-LabyrinthQuest

## Description
LabyrinthQuest is an multiplayer game developed in C using the Ncurses library,threads and sockets. It allows up to four players to connect and play together via a server-client connection using sockets.

In the game, players will explore a labyrinth filled with coins to collect and dangerous beasts to avoid. The objective is to gather as many coins as possible while staying away from the beasts' reach. Be careful! If a player is caught by a beast, they will lose their collected coins.

The game offers real-time, synchronized gameplay, allowing players to compete against each other.

## Features
- Multiplayer Gameplay: Up to 4 players can connect to the game server and compete against each other.
- Server-Client Connection: The game uses sockets for communication between the server and clients, enabling multiplayer functionality.
- Labyrinth Generation: The game generates a labyrinth based on text file mapa.txt.
- Player Movement: Players can navigate through the labyrinth using arrow keys.
- Beast AI: Beasts are AI-controlled entities that hunt down players. Players must avoid them to stay alive and collect coins.
- Coin Collection: Players must collect coins scattered throughout the labyrinth and bring them to a designated base to earn points.
- Death and Respawn: If a player is caught by a beast, they die, and their collected coins are dropped. Players respawn at a designated location.
- Synchronized Gameplay: The game employs multi-threading to ensure synchronized gameplay across all connected players, providing a smooth and immersive experience.

## Usage


### 1. Compile the server code:
  - Open the terminal and navigate to the game directory.
  - Enter the following command: make

This will compile the server.c, bot.c, and client.c files and generate executable files for each.

### 2. Start the server:
   
- Run the server by executing the following command: ./server

### 3. Start a client:
  
- Run the client by executing the following command: ./client
- Repeat this step for each additional player (up to four players).
- The server and clients will establish a connection, and the game will begin.

### 4. Start a bot:
  
- Run the bot by executing the following command: ./bot
- The server and clients will establish a connection, and the game will begin.


## Map
To create a custom map, just edit map.txt or pacman.txt file.

## Controls

### Players:

- Use arrow keys (↑, ↓, ←, →) to move.
- 'q' or 'Q' to quit.
- 
The movement keys allow the player to navigate through the labyrinth, collect treasures, and interact with the environment.

## Server:

- 'c' to spawn a coin.
- 't' to spawn a  treasure.
- 'T' to spawn a large treasue.
- 'b' or 'B' to spawn a beast.
- 'q' or 'Q' to quit.

## Screenshots

### Server view :
![image](https://github.com/Joki004/IPC-LabyrinthQuest/assets/101185519/35fcdea9-ec57-4849-aa1d-b2104e0a798e)

### Player view:
![image](https://github.com/Joki004/IPC-LabyrinthQuest/assets/101185519/7220013d-6283-4921-ac24-78eb5c549dfb)

## Communication
The game features threaded communication using sockets. Players and the server exchange data to synchronize gameplay and enable real-time interaction.

## Contributing
Feel free to contribute to this project by submitting pull requests or opening issues.

## License
This project is licensed under the MIT License.

