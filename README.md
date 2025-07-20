# Adventure Game - ECS with Template System

A C-based adventure game using Entity Component System (ECS) architecture with SDL2 for rendering and a flexible template system for entity creation.

## Features

- **ECS Architecture**: Clean separation of data and logic
- **Template System**: Create entities from JSON configuration files
- **SDL2 Rendering**: Text-based rendering with SDL2 and SDL_ttf
- **Input Handling**: Arrow key movement with boundary checking
- **Modular Design**: Easy to extend with new components and systems

## Building

### Prerequisites
- SDL2 and SDL2_ttf
- cjson library
- GCC compiler

### Install Dependencies (macOS)
```bash
brew install sdl2 sdl2_ttf cjson
```

### Build
```bash
make clean && make
```

## Template System

The template system allows you to define entities in JSON format and create them dynamically at runtime.

### Template File Format (`data.json`)

```json
{
  "templates": [
    {
      "name": "player",
      "components": [
        {
          "type": "Position",
          "x": 5.0,
          "y": 5.0
        },
        {
          "type": "Representation",
          "symbol": "@",
          "color": 2
        },
        {
          "type": "Player",
          "is_player": 1,
          "name": "Player"
        },
        {
          "type": "Action",
          "action_type": 0,
          "action_data": 0
        }
      ]
    }
  ]
}
```

### Creating Entities from Templates

```c
// Initialize template system
template_system_init();

// Load templates from file
load_templates_from_file("data.json");

// Create entity from template
Entity player = create_entity_from_template("player");
```

### Supported Components

#### Position
- `x`: X coordinate (float)
- `y`: Y coordinate (float)

#### Representation
- `symbol`: Character to display (char)
- `color`: Color index (int)

#### Player
- `is_player`: Boolean flag (int)
- `name`: Player name (string)

#### Action
- `action_type`: Action type enum (int)
- `action_data`: Action data (int)

## Controls

- **Arrow Keys**: Move the player character
- **Close Window**: Quit the game

## Architecture

### Core Systems
- **ECS**: Entity Component System core
- **Template System**: JSON-based entity creation
- **Render System**: SDL2 text rendering
- **Action System**: Movement and action processing
- **Display System**: Grid and window management

### Components
- **Position**: 2D coordinates
- **Representation**: Visual appearance
- **Player**: Player-specific data
- **Action**: Movement and action data

## Extending the System

### Adding New Components
1. Define the component structure in `ecs.h`
2. Register it in `ecs.c`
3. Add parsing logic in `template_system.c`
4. Update the JSON template format

### Adding New Templates
1. Add template definition to `data.json`
2. Use `create_entity_from_template()` to instantiate

### Adding New Systems
1. Create system function
2. Register with ECS
3. Define component requirements

## File Structure

```
adv/
├── src/
│   ├── main.c              # Main program entry
│   ├── ecs.h/c             # ECS core system
│   ├── template_system.h/c # Template loading system
│   ├── render_system.h/c   # SDL2 rendering
│   ├── action_system.h/c   # Movement processing
│   ├── input_system.h/c    # Input handling
│   └── display.h/c         # Window management
├── data.json               # Entity templates
├── Makefile                # Build configuration
└── README.md              # This file
```

## License

This project is open source. Feel free to modify and extend it for your own projects.
