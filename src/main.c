#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 960

#define BOX_MARGIN 50 // Margin for the bounding box
#define BOX_THICKNESS 5 // Thickness of the bounding box

#define ADJUSTMENTS_COLUMN_WIDTH 200 // Width for the adjustments column

#define BOX_WIDTH (WINDOW_WIDTH - 2 * BOX_MARGIN - ADJUSTMENTS_COLUMN_WIDTH)
#define BOX_HEIGHT (WINDOW_HEIGHT - 2 * BOX_MARGIN)

#define NUM_NODES 10
#define NODE_RADIUS 10 // Radius for the circular nodes

#define ITERATIONS 1000
#define COOLING_FACTOR 0.95 // Cooling factor for reducing temperature
#define FRAME_STEP_SIZE 5 // Smaller step size for smoother movement
#define FRAME_DELAY 16 // Approx 60 FPS (1000ms / 60 frames = ~16ms per frame)

#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 50

// Node structure
typedef struct {
    float x, y;   // Position
    float dx, dy; // Displacement (force)
} Node;

// Edge structure (connecting two nodes)
typedef struct {
    int from;
    int to;
} Edge;

// Global variables to store the initial state
Node initial_node_states[NUM_NODES];
Node node_states[ITERATIONS][NUM_NODES];
int initial_state_saved = 0;

// Function prototypes
void initialize_nodes(Node nodes[], int num_nodes);
void initialize_edges(Edge edges[], int num_edges);
void draw_circle(SDL_Renderer *renderer, int x, int y, int radius);
void calculate_forces(Node nodes[], Edge edges[], int num_nodes, int num_edges, float temperature, int iteration);
void save_node_state(Node nodes[], int iteration);
void restore_node_state(Node nodes[], int iteration);
void save_initial_state(Node nodes[]);
void restore_initial_state(Node nodes[]);
float clamp(float value, float min, float max);
int is_point_in_rect(int x, int y, SDL_Rect* rect);

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Fruchterman-Reingold Visualization", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (win == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        SDL_DestroyWindow(win);
        printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize nodes and edges
    Node nodes[NUM_NODES];
    Edge edges[NUM_NODES - 1];

    initialize_nodes(nodes, NUM_NODES);
    initialize_edges(edges, NUM_NODES - 1);
    save_initial_state(nodes); // Save the initial state of the first generated nodes

    float temperature = 50.0f; // Initial temperature

    // Main loop flag
    int running = 1;
    int auto_play = 0;  // Flag for automatic play/pause
    int iteration_updated_manually = 0; // Flag for manual frame updates

    // Event handler
    SDL_Event e;

    int iteration = 0;
    int max_iterations = ITERATIONS;

    // Define the "Generate New Nodes" button
    SDL_Rect buttonRect = {WINDOW_WIDTH - BUTTON_WIDTH - 20, WINDOW_HEIGHT - BUTTON_HEIGHT - 20, BUTTON_WIDTH, BUTTON_HEIGHT};

while (running) {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            running = 0;
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            // Check if the "Generate New Nodes" button is clicked
            if (is_point_in_rect(e.button.x, e.button.y, &buttonRect)) {
                initialize_nodes(nodes, NUM_NODES); // Generate new nodes
                iteration = 0; // Reset iteration
                temperature = 50.0f; // Reset temperature
                auto_play = 0; // Stop auto-play if active
                save_initial_state(nodes); // Save the initial state of the newly generated nodes
            }
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_RIGHT: // Step forward
                    if (!auto_play && iteration < max_iterations - 1) {
                        iteration += 1;  // Increment iteration by 1 for exact control
                        calculate_forces(nodes, edges, NUM_NODES, NUM_NODES - 1, temperature, iteration);
                        save_node_state(nodes, iteration); // Save the node state after each calculation
                        iteration_updated_manually = 1;
                    }
                    break;
                case SDLK_LEFT: // Step backward
                    if (!auto_play && iteration > 0) { // Ensure we don't go below 0
                        iteration -= 1;  // Decrement iteration by 1 for exact control
                        if (iteration == 0) {
                            restore_initial_state(nodes); // Restore initial positions for the first frame
                        } else {
                            restore_node_state(nodes, iteration);
                        }
                        iteration_updated_manually = 1;
                    }
                    break;
                case SDLK_SPACE: // Play/Pause
                    auto_play = !auto_play;
                    break;
            }
        }
    }

    // Clear the renderer with white background
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderClear(renderer);

    // Draw thicker bounding box (black outline)
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black color
    for (int i = 0; i < BOX_THICKNESS; i++) {
        SDL_Rect boundingBox = {BOX_MARGIN - i, BOX_MARGIN - i, BOX_WIDTH + 2 * i, BOX_HEIGHT + 2 * i};
        SDL_RenderDrawRect(renderer, &boundingBox);
    }

    // Calculate forces and update node positions only if auto_play is true or manual update occurred
    if (auto_play || iteration_updated_manually) {
        if (iteration < ITERATIONS) {
            calculate_forces(nodes, edges, NUM_NODES, NUM_NODES - 1, temperature, iteration);
            save_node_state(nodes, iteration); // Save the node state after each calculation
            
            if (auto_play) {
                iteration += 1;  // Increment by 1 for smooth, frame-by-frame animation
                if (iteration >= max_iterations) {
                    iteration = max_iterations - 1;
                    auto_play = 0;  // Stop auto-play when reaching the last frame
                }
            }

            // Reset the manual step flag after updating
            iteration_updated_manually = 0;
        }
    }

    // Render nodes (as black circles)
    for (int i = 0; i < NUM_NODES; i++) {
        draw_circle(renderer, (int)nodes[i].x, (int)nodes[i].y, NODE_RADIUS);
    }

    // Render edges (single line thickness)
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black color
    for (int i = 0; i < NUM_NODES - 1; i++) {
        SDL_RenderDrawLine(renderer, (int)nodes[edges[i].from].x, (int)nodes[edges[i].from].y, 
                                       (int)nodes[edges[i].to].x, (int)nodes[edges[i].to].y);
    }

    // Draw "Generate New Nodes" button
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black color
    SDL_RenderDrawRect(renderer, &buttonRect);

    // Update screen
    SDL_RenderPresent(renderer);

    // Ensure delay only happens when auto_play is true
    if (auto_play) {
        SDL_Delay(FRAME_DELAY); // Using a shorter, more consistent delay for smoother animation
    }
}
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

// Initialize nodes with random positions within the bounding box
void initialize_nodes(Node nodes[], int num_nodes) {
    for (int i = 0; i < num_nodes; i++) {
        nodes[i].x = (float)(rand() % BOX_WIDTH + BOX_MARGIN);
        nodes[i].y = (float)(rand() % BOX_HEIGHT + BOX_MARGIN);
        nodes[i].dx = 0;
        nodes[i].dy = 0;
    }
}

// Initialize edges connecting nodes in a simple chain (for now)
void initialize_edges(Edge edges[], int num_edges) {
    for (int i = 0; i < num_edges; i++) {
        edges[i].from = i;
        edges[i].to = i + 1;
    }
}

// Function to save the current node state
void save_node_state(Node nodes[], int iteration) {
    for (int i = 0; i < NUM_NODES; i++) {
        node_states[iteration][i] = nodes[i];
    }
}

// Function to restore the node state from a specific iteration
void restore_node_state(Node nodes[], int iteration) {
    for (int i = 0; i < NUM_NODES; i++) {
        nodes[i] = node_states[iteration][i];
    }
}

// Function to calculate forces and update node positions
void calculate_forces(Node nodes[], Edge edges[], int num_nodes, int num_edges, float temperature, int iteration) {
    // Reset displacements
    for (int i = 0; i < num_nodes; i++) {
        nodes[i].dx = 0;
        nodes[i].dy = 0;
    }

    // Calculate repulsive forces
    for (int i = 0; i < num_nodes; i++) {
        for (int j = i + 1; j < num_nodes; j++) {
            float dx = nodes[i].x - nodes[j].x;
            float dy = nodes[i].y - nodes[j].y;
            float distance = sqrtf(dx * dx + dy * dy);

            if (distance > 0) {
                float force = 1000.0f / distance; // Repulsive force
                nodes[i].dx += (dx / distance) * force;
                nodes[i].dy += (dy / distance) * force;
                nodes[j].dx -= (dx / distance) * force;
                nodes[j].dy -= (dy / distance) * force;
            }
        }
    }

    // Calculate attractive forces
    for (int i = 0; i < num_edges; i++) {
        int from = edges[i].from;
        int to = edges[i].to;
        float dx = nodes[from].x - nodes[to].x;
        float dy = nodes[from].y - nodes[to].y;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance > 0) {
            float force = (distance * distance) / 1000.0f; // Attractive force
            nodes[from].dx -= (dx / distance) * force;
            nodes[from].dy -= (dy / distance) * force;
            nodes[to].dx += (dx / distance) * force;
            nodes[to].dy += (dy / distance) * force;
        }
    }

    // Update positions based on forces and limit movement
    for (int i = 0; i < num_nodes; i++) {
        nodes[i].x += clamp(nodes[i].dx, -temperature, temperature);
        nodes[i].y += clamp(nodes[i].dy, -temperature, temperature);

        // Keep nodes within the bounding box
        nodes[i].x = clamp(nodes[i].x, BOX_MARGIN, BOX_MARGIN + BOX_WIDTH);
        nodes[i].y = clamp(nodes[i].y, BOX_MARGIN, BOX_MARGIN + BOX_HEIGHT);
    }
}

// Function to draw a filled circle
void draw_circle(SDL_Renderer *renderer, int x, int y, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

// Clamp function to limit values
float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Function to check if a point is inside a rectangle
int is_point_in_rect(int x, int y, SDL_Rect* rect) {
    return (x >= rect->x && x <= rect->x + rect->w &&
            y >= rect->y && y <= rect->y + rect->h);
}


void restore_initial_state(Node nodes[]) {
    for (int i = 0; i < NUM_NODES; i++) {
        nodes[i] = initial_node_states[i];
    }
}
void save_initial_state(Node nodes[]) {
    for (int i = 0; i < NUM_NODES; i++) {
        initial_node_states[i] = nodes[i];
    }
}