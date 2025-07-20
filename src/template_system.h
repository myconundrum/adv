#ifndef TEMPLATE_SYSTEM_H
#define TEMPLATE_SYSTEM_H

#include "ecs.h"

// Initialize the template system
int template_system_init(void);

// Clean up the template system
void template_system_cleanup(void);

// Create an entity from a template
Entity create_entity_from_template(const char* template_name);

// Load templates from a JSON file
int load_templates_from_file(const char* filename);

#endif // TEMPLATE_SYSTEM_H 
