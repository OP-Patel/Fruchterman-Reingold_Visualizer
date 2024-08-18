#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 960

#define BOX_MARGIN 50 // Margin for the bounding box
#define BOX_THICKNESS 5 // Thickness of the bounding box

#define ADJUSTMENTS_COLUMN_WIDTH 200 // Width for the button column

#define BOX_WIDTH (WINDOW_WIDTH - 2 * BOX_MARGIN - ADJUSTMENTS_COLUMN_WIDTH)
#define BOX_HEIGHT (WINDOW_HEIGHT - 2 * BOX_MARGIN)

#define NUM_NODES 21
#define NODE_RADIUS 7 // Node radius

#define ITERATIONS 201 // Max frames before end: og 1000
#define COOLING_FACTOR 0.95 // Cooling factor for reducing temperature: og .95
#define FRAME_STEP_SIZE 3 // Smaller step size for smoother movement: og 5
#define FRAME_DELAY 16 // Approx 60 FPS (1000ms / 60 frames = ~16ms per frame)

#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 50

#define MIN_CELL_SIZE 20
#define MAX_CELL_SIZE 200


// Constants for the clipping region codes
#define INSIDE 0 // 0000
#define LEFT   1 // 0001
#define RIGHT  2 // 0010
#define BOTTOM 4 // 0100
#define TOP    8 // 1000

// Node structure
typedef struct {
    float x, y;   // Position
    float dx, dy; // Displacement
} Node;

// Edge structure
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
void draw_grid(SDL_Renderer *renderer, int cell_size, float grid_offset_x, float grid_offset_y); 
void draw_circle_clipped(SDL_Renderer *renderer, int x, int y, int radius, int left, int top, int right, int bottom);
void draw_line_clipped(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int left, int top, int right, int bottom);
int compute_code(int x, int y, int left, int top, int right, int bottom);

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
    Edge edges[42];

    initialize_nodes(nodes, NUM_NODES);
    initialize_edges(edges, 42);
    save_initial_state(nodes); // Save the initial state

    float temperature = 40.0f; // Initial temperature: og 50.0f 
    int cell_size = 30;
    float grid_offset_x = 0.0f; // Horizontal offset for panning
    float grid_offset_y = 0.0f; // Vertical offset for panning

    // Main loop flag
    int running = 1;
    int auto_play = 0;  // Flag for automatic play/pause
    int iteration_updated_manually = 0; // Flag for manual frame updates

    // Event handler
    SDL_Event e;

    int iteration = 0;
    int max_iterations = ITERATIONS;

    // "Generate Nodes" button
    SDL_Rect buttonRect = {WINDOW_WIDTH - BUTTON_WIDTH - 50, WINDOW_HEIGHT - BUTTON_HEIGHT - 37, BUTTON_WIDTH, BUTTON_HEIGHT};

    int left_bound = BOX_MARGIN;
    int top_bound = BOX_MARGIN;
    int right_bound = BOX_MARGIN + BOX_WIDTH;
    int bottom_bound = BOX_MARGIN + BOX_HEIGHT;

    // Initialization (ttf)
    if (TTF_Init() == -1) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Normal Sized Font
    TTF_Font *font = TTF_OpenFont("fonts/RobotoMono-VariableFont_wght.ttf", 24);
    if (font == NULL) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Button Sized Font
    TTF_Font *buttonFont = TTF_OpenFont("fonts/Roboto-Regular.ttf", 24); 
    if (buttonFont == NULL) {
        printf("TTF_OpenFont Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Frames text
    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Frame: 0", textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    // Algorithm name text
    const char *algorithmName = "Fruchterman-Reingold";
    SDL_Surface *algorithmTextSurface = TTF_RenderText_Solid(font, algorithmName, textColor);
    SDL_Texture *algorithmTextTexture = SDL_CreateTextureFromSurface(renderer, algorithmTextSurface);
    int text_width = 0;
    int text_height = 0;
    TTF_SizeText(font, algorithmName, &text_width, &text_height);
    SDL_FreeSurface(algorithmTextSurface);

    // Generate New text
    SDL_Surface *buttonTextSurface = TTF_RenderText_Solid(buttonFont, "Generate Nodes", textColor);
    SDL_Texture *buttonTextTexture = SDL_CreateTextureFromSurface(renderer, buttonTextSurface);
    SDL_FreeSurface(buttonTextSurface);

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                // Check if the "Generate Nodes" button is clicked
                if (is_point_in_rect(e.button.x, e.button.y, &buttonRect)) {
                    initialize_nodes(nodes, NUM_NODES); // Generate new nodes
                    iteration = 0; // Reset
                    temperature = 50.0f; // Reset
                    auto_play = 0; // Stop auto-play if active
                    save_initial_state(nodes); // Save the initial state
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
                // Check if the mouse is within the grid box area
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                if (mouse_x >= BOX_MARGIN && mouse_x <= BOX_MARGIN + BOX_WIDTH &&
                    mouse_y >= BOX_MARGIN && mouse_y <= BOX_MARGIN + BOX_HEIGHT) {

                    float prev_scale_factor = (float)cell_size / 50.0f;

                    // Zoom in or out based on the scroll direction
                    if (e.wheel.y > 0 && cell_size < MAX_CELL_SIZE) { // Scroll up
                        cell_size = (int)(cell_size * 1.1f);
                        if (cell_size > MAX_CELL_SIZE) {
                            cell_size = MAX_CELL_SIZE;
                        }
                    } else if (e.wheel.y < 0 && cell_size > MIN_CELL_SIZE) { // Scroll down
                        cell_size = (int)(cell_size / 1.1f);
                        if (cell_size < MIN_CELL_SIZE) {
                            cell_size = MIN_CELL_SIZE;
                        }
                    }

                    float new_scale_factor = (float)cell_size / 50.0f;
                    float scale_ratio = new_scale_factor / prev_scale_factor;

                    // Adjust grid offset
                    grid_offset_x = mouse_x - scale_ratio * (mouse_x - grid_offset_x);
                    grid_offset_y = mouse_y - scale_ratio * (mouse_y - grid_offset_y);
                }
            } else if (e.type == SDL_KEYDOWN) {
                // Arrow keys for panning the grid
                switch (e.key.keysym.sym) {

                    case SDLK_RIGHT: // Step forward
                    if (!auto_play && iteration < max_iterations - 1) {
                        iteration += 1;  // Increment iteration by 1
                        calculate_forces(nodes, edges, NUM_NODES, 42, temperature, iteration);
                        save_node_state(nodes, iteration); // Save the node state after each calculation
                        iteration_updated_manually = 1;
                    }
                    break;
                case SDLK_LEFT: // Step backward
                    if (!auto_play && iteration > 0) { // Ensure it doesn't go <0
                        iteration -= 1;  // Decrement iteration by 1
                        if (iteration == 0) {
                            restore_initial_state(nodes); // Restore initial positions
                        } else {
                            restore_node_state(nodes, iteration);
                        }
                        iteration_updated_manually = 1;
                    }
                    break;
                    case SDLK_w:
                        grid_offset_y += 10;
                        break;
                    case SDLK_s:
                        grid_offset_y -= 10;
                        break;
                    case SDLK_a:
                        grid_offset_x += 10;
                        break;
                    case SDLK_d:
                        grid_offset_x -= 10;
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

        // Draw the background grid
        draw_grid(renderer, cell_size, grid_offset_x, grid_offset_y);

        // Draw thicker bounding box (black outline)
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black color
        for (int i = 0; i < BOX_THICKNESS; i++) {
            SDL_Rect boundingBox = {BOX_MARGIN - i, BOX_MARGIN - i, BOX_WIDTH + 2 * i, BOX_HEIGHT + 2 * i};
            SDL_RenderDrawRect(renderer, &boundingBox);
        }

        // Calculate forces and update node positions only if auto_play is true
        if (auto_play || iteration_updated_manually) {
            if (iteration < ITERATIONS) {
                calculate_forces(nodes, edges, NUM_NODES, 42, temperature, iteration);
                save_node_state(nodes, iteration); // Save the node state after each calculation
                
                if (auto_play) {
                    iteration += 1;  // Increment by 1 for frame-by-frame animation
                    if (iteration >= max_iterations) {
                        iteration = max_iterations - 1;
                        auto_play = 0;
                    }
                }

                // Reset the manual step flag after updating
                iteration_updated_manually = 0;
            }
        }

        // Render nodes (keeping size constant, but adjusting position and clipping)
        for (int i = 0; i < NUM_NODES; i++) {
            int adjusted_x = grid_offset_x + (nodes[i].x - grid_offset_x) * ((float)cell_size / 50.0f);
            int adjusted_y = grid_offset_y + (nodes[i].y - grid_offset_y) * ((float)cell_size / 50.0f);
            draw_circle_clipped(renderer, adjusted_x, adjusted_y, NODE_RADIUS, left_bound, top_bound, right_bound, bottom_bound);
        }
        // Render edges in the same manner as nodes
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black
        for (int i = 0; i < 42; i++) {
            int from_x = grid_offset_x + (nodes[edges[i].from].x - grid_offset_x) * ((float)cell_size / 50.0f);
            int from_y = grid_offset_y + (nodes[edges[i].from].y - grid_offset_y) * ((float)cell_size / 50.0f);
            int to_x = grid_offset_x + (nodes[edges[i].to].x - grid_offset_x) * ((float)cell_size / 50.0f);
            int to_y = grid_offset_y + (nodes[edges[i].to].y - grid_offset_y) * ((float)cell_size / 50.0f);
            draw_line_clipped(renderer, from_x, from_y, to_x, to_y, left_bound, top_bound, right_bound, bottom_bound);
        }

        // Render "Generate Nodes" button
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); 
        SDL_RenderDrawRect(renderer, &buttonRect);

        
        // Update frame text
        char frameText[50];
        sprintf(frameText, "Frame: %d", iteration);
        textSurface = TTF_RenderText_Solid(font, frameText, textColor);
        textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);

        SDL_Rect textRect = {10, 10, 100, 30}; 
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        // Position the algorithm name centered above the bounding box
        SDL_Rect algorithmTextRect = {
            BOX_MARGIN + (BOX_WIDTH - text_width) / 2,
            BOX_MARGIN - text_height - 10,
            text_width,
            text_height
        };
        SDL_RenderCopy(renderer, algorithmTextTexture, NULL, &algorithmTextRect);

        int text_width = 0;
        int text_height = 0;
        TTF_SizeText(font, "Generate New", &text_width, &text_height);

        // Make sure the text fits within the button
        if (text_width > buttonRect.w || text_height > buttonRect.h) {
            buttonRect.w = text_width + 10;  // Padding
            buttonRect.h = text_height + 10;  
        }

        // Render button text (keeping it centered)
        SDL_Rect buttonTextRect = {
            buttonRect.x + (buttonRect.w - text_width) / 2,  
            buttonRect.y + (buttonRect.h - text_height) / 2,  
            text_width,
            text_height
        };

        SDL_RenderCopy(renderer, buttonTextTexture, NULL, &buttonTextRect);

        // Update display
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(textTexture);

        // Ensure delay only happens when auto_play is true
        if (auto_play) {
            SDL_Delay(FRAME_DELAY);
        }
    }

    SDL_DestroyTexture(textTexture);  
    SDL_DestroyTexture(algorithmTextTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}

// Initialize nodes with random positions within the bounding box
void initialize_nodes(Node nodes[], int num_nodes) {
    srand(time(NULL)); 

    for (int i = 0; i < num_nodes; i++) {
        nodes[i].x = (float)(rand() % BOX_WIDTH + BOX_MARGIN);
        nodes[i].y = (float)(rand() % BOX_HEIGHT + BOX_MARGIN);
        nodes[i].dx = 0;
        nodes[i].dy = 0;
    }
}

void initialize_edges(Edge edges[], int num_edges) { //currently hardcoded
    edges[0] = (Edge){0, 1};  // a -- b
    edges[1] = (Edge){0, 2};  // a -- c
    edges[2] = (Edge){0, 3};  // a -- d
    edges[3] = (Edge){1, 2};  // b -- c
    edges[4] = (Edge){1, 4};  // b -- e
    edges[5] = (Edge){2, 4};  // c -- e
    edges[6] = (Edge){2, 5};  // c -- f
    edges[7] = (Edge){3, 5};  // d -- f
    edges[8] = (Edge){3, 6};  // d -- g
    edges[9] = (Edge){4, 7};  // e -- h
    edges[10] = (Edge){5, 7}; // f -- h
    edges[11] = (Edge){5, 8}; // f -- i
    edges[12] = (Edge){5, 9}; // f -- j
    edges[13] = (Edge){5, 6}; // f -- g
    edges[14] = (Edge){6, 10}; // g -- k
    edges[15] = (Edge){7, 14}; // h -- o
    edges[16] = (Edge){7, 11}; // h -- l
    edges[17] = (Edge){8, 11}; // i -- l
    edges[18] = (Edge){8, 12}; // i -- m
    edges[19] = (Edge){8, 9};  // i -- j
    edges[20] = (Edge){9, 12}; // j -- m
    edges[21] = (Edge){9, 13}; // j -- n
    edges[22] = (Edge){9, 10}; // j -- k
    edges[23] = (Edge){10, 13}; // k -- n
    edges[24] = (Edge){10, 17}; // k -- r
    edges[25] = (Edge){11, 14}; // l -- o
    edges[26] = (Edge){11, 12}; // l -- m
    edges[27] = (Edge){12, 14}; // m -- o
    edges[28] = (Edge){12, 15}; // m -- p
    edges[29] = (Edge){12, 13}; // m -- n
    edges[30] = (Edge){13, 16}; // n -- q
    edges[31] = (Edge){13, 17}; // n -- r
    edges[32] = (Edge){14, 18}; // o -- s
    edges[33] = (Edge){14, 15}; // o -- p
    edges[34] = (Edge){15, 18}; // p -- s
    edges[35] = (Edge){15, 19}; // p -- t
    edges[36] = (Edge){15, 16}; // p -- q
    edges[37] = (Edge){16, 19}; // q -- t
    edges[38] = (Edge){16, 17}; // q -- r
    edges[39] = (Edge){17, 19}; // r -- t
    edges[40] = (Edge){18, 20}; // s -- z
    edges[41] = (Edge){19, 20}; // t -- z
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

    // Update positions based on forces
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
            int dx = radius - w; 
            int dy = radius - h; 
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

void draw_grid(SDL_Renderer *renderer, int cell_size, float grid_offset_x, float grid_offset_y) {
    SDL_SetRenderDrawColor(renderer, 0xDD, 0xDD, 0xDD, 0xFF);  // Light gray

    int left = BOX_MARGIN;
    int top = BOX_MARGIN;
    int right = BOX_MARGIN + BOX_WIDTH;
    int bottom = BOX_MARGIN + BOX_HEIGHT;

    // Calculate the starting points considering the grid offset
    int start_x = (int)(left + grid_offset_x) % cell_size;
    int start_y = (int)(top + grid_offset_y) % cell_size;

    // Draw vertical lines
    for (int x = start_x; x <= right; x += cell_size) {
        if (x >= left) {
            SDL_RenderDrawLine(renderer, x, top, x, bottom);
        }
    }

    // Draw horizontal lines
    for (int y = start_y; y <= bottom; y += cell_size) {
        if (y >= top) {
            SDL_RenderDrawLine(renderer, left, y, right, y);
        }
    }
}

void draw_circle_clipped(SDL_Renderer *renderer, int x, int y, int radius, int left, int top, int right, int bottom) {
    if (x - radius < left || x + radius > right || y - radius < top || y + radius > bottom) {
        return; // Skip drawing the circle if it's outside the bounding box
    }
    draw_circle(renderer, x, y, radius);
}

void draw_line_clipped(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int left, int top, int right, int bottom) {
    int code1 = compute_code(x1, y1, left, top, right, bottom);
    int code2 = compute_code(x2, y2, left, top, right, bottom);
    int accept = 0;
    while (1) {
        if ((code1 == 0) && (code2 == 0)) {
            // Both endpoints are inside the bounding box
            accept = 1;
            break;
        } else if (code1 & code2) {
            // Both endpoints are outside the bounding box in the same region
            break;
        } else {
            // Some portion of the line is inside the bounding box
            int code_out;
            int x, y;

            // At least one endpoint is outside the bounding box, pick it
            if (code1 != 0) code_out = code1;
            else code_out = code2;

            // Find the intersection point
            if (code_out & TOP) {
                // Up
                x = x1 + (x2 - x1) * (top - y1) / (y2 - y1);
                y = top;
            } else if (code_out & BOTTOM) {
                // Down
                x = x1 + (x2 - x1) * (bottom - y1) / (y2 - y1);
                y = bottom;
            } else if (code_out & RIGHT) {
                // Right
                y = y1 + (y2 - y1) * (right - x1) / (x2 - x1);
                x = right;
            } else if (code_out & LEFT) {
                // Left
                y = y1 + (y2 - y1) * (left - x1) / (x2 - x1);
                x = left;
            }
            // Replace outside point with intersection point and update the code
            if (code_out == code1) {
                x1 = x;
                y1 = y;
                code1 = compute_code(x1, y1, left, top, right, bottom);
            } else {
                x2 = x;
                y2 = y;
                code2 = compute_code(x2, y2, left, top, right, bottom);
            }
        }
    }

    if (accept) {
        // Draw the line
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

// Compute the region code for a point (x, y)
int compute_code(int x, int y, int left, int top, int right, int bottom) {
    int code = INSIDE;

    if (x < left)       code |= LEFT;
    else if (x > right) code |= RIGHT;
    if (y < top)        code |= BOTTOM;
    else if (y > bottom) code |= TOP;

    return code;
}