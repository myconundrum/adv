# Adventure Game - ECS with Template System

A C-based adventure game using Entity Component System (ECS) architecture with SDL2 for rendering and a flexible template system for entity creation.

## Enhanced Component System

### BaseInfo Component Enhancements

The `BaseInfo` component has been significantly enhanced to serve as a centralized repository for commonly used entity properties:

#### New Fields Added:
- **`flags`** - 32-bit bitfield for entity properties (replaces individual boolean flags)
- **`weight`** - Weight of the entity (for inventory management)
- **`volume`** - Volume of the entity (for inventory management)  
- **`description`** - Detailed description of the entity (128 characters)

#### Consolidated Flags System

Previously scattered boolean flags have been consolidated into a centralized flags bitfield:

```c
typedef enum {
    ENTITY_FLAG_CARRYABLE = 1 << 0,    // Can this entity be picked up?
    ENTITY_FLAG_PLAYER = 1 << 1,       // Is this entity the player?
    ENTITY_FLAG_CAN_CARRY = 1 << 2,    // Can this entity carry items?
    ENTITY_FLAG_MOVED = 1 << 3,        // Has this entity moved this turn?
    ENTITY_FLAG_ALIVE = 1 << 4,        // Is this entity alive?
    ENTITY_FLAG_HOSTILE = 1 << 5,      // Is this entity hostile?
    ENTITY_FLAG_BLOCKING = 1 << 6,     // Does this entity block movement?
    ENTITY_FLAG_VISIBLE = 1 << 7,      // Is this entity visible?
    // Room for 24 more flags in a 32-bit field
} EntityFlags;
```

#### Flag Manipulation Macros

```c
#define ENTITY_HAS_FLAG(entity_flags, flag) ((entity_flags) & (flag))
#define ENTITY_SET_FLAG(entity_flags, flag) ((entity_flags) |= (flag))
#define ENTITY_CLEAR_FLAG(entity_flags, flag) ((entity_flags) &= ~(flag))
#define ENTITY_TOGGLE_FLAG(entity_flags, flag) ((entity_flags) ^= (flag))
```

#### Convenience Functions

```c
bool entity_is_player(Entity entity);
bool entity_can_carry(Entity entity);
bool entity_is_carryable(Entity entity);
bool entity_has_moved(Entity entity);
void entity_clear_moved_flag(Entity entity);
```

#### Usage Examples

```c
// Check if an entity can be picked up
if (entity_is_carryable(item_entity)) {
    pickup_item(player, item_entity);
}

// Set an entity as carryable using direct flag manipulation
BaseInfo *info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
ENTITY_SET_FLAG(info->flags, ENTITY_FLAG_CARRYABLE);

// Mark an entity as having moved
BaseInfo *info = (BaseInfo *)entity_get_component(entity, component_get_id("BaseInfo"));
ENTITY_SET_FLAG(info->flags, ENTITY_FLAG_MOVED);
```

#### Legacy Compatibility

The template system maintains backward compatibility with legacy JSON formats:
- Legacy `is_carryable`, `is_player`, and `can_carry` fields are automatically converted to the new flags system
- New templates can use either the legacy format or the new `flags` field directly

#### Benefits

1. **Centralized Properties**: Common entity properties are now in one place
2. **Memory Efficient**: 32-bit flags field can store 32 boolean properties
3. **Extensible**: Easy to add new flags without changing the component structure
4. **Performance**: Bitwise operations are faster than multiple boolean checks
5. **Backward Compatible**: Existing JSON templates continue to work

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
