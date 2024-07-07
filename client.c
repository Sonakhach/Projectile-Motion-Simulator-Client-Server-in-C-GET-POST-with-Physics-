#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include <math.h>

#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 1
#define TIMEOUT_USEC 0
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define The_first_space_velocity 8000

typedef struct {
    double initial_velocity;
    double launch_angle;
    double max_height;
    double time_of_flight;
    double range;
} TrajectoryData;


void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int create_and_connect_socket(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        handle_error("Socket creation failed");
    }

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        if (errno != EINPROGRESS) {
            handle_error("Connection failed");
        }
    }

    return sock;
}

void send_request(int sock, const char *message) {
    fd_set writefds;
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;

    int sent = 0;
    int message_len = strlen(message);

    while (sent < message_len) {
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);

        int activity = select(sock + 1, NULL, &writefds, NULL, &timeout);
        if (activity < 0) {
            handle_error("Select error");
        } else if (activity == 0) {
            fprintf(stderr, "Timeout occurred! No data sent.\n");
            return;
        } else if (activity > 0) {
            if (FD_ISSET(sock, &writefds)) {
                int bytes_sent = send(sock, message + sent, message_len - sent, 0);
                if (bytes_sent < 0) {
                    handle_error("Send failed");
                }
                sent += bytes_sent;
            }
        }
    }
}

void receive_response(int sock, char *buffer,  TrajectoryData *data, int ac) {
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        int activity = select(sock + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0) {
            handle_error("Select error");
        } else if (activity == 0) {
            // fprintf(stderr, "Timeout occurred! No data received.\n");
            return;
        } else if (activity > 0) {
            if (FD_ISSET(sock, &readfds)) {
                int len = recv(sock, buffer, BUFFER_SIZE - 1, 0);
                if (len < 0) {
                    handle_error("Receive failed");
                }
                if (len == 0) {
                    break; 
                }
                buffer[len] = '\0';
                printf("Response: %s\n", buffer);
                if(ac == 7){
                    sscanf(buffer, 
                       "HTTP/1.1 200 OK\nContent-Type: application/json\n\n{\"max_height\":%lf, \"time_of_flight\":%lf, \"range\":%lf}",
                       &data->max_height, &data->time_of_flight, &data->range);
                       
                    printf("max_height: %.2f m\n", data->max_height);
                    printf("time_of_flight: %.2f s\n", data->time_of_flight);
                    printf("range: %.2f m\n", data->range);
               }     

            }
        }
    }
}

void construct_get_request(char *message, const char *name, double velocity, double angle) {
    snprintf(message, BUFFER_SIZE, "GET /?name=%s&velocity=%lf&angle=%lf HTTP/1.1\r\n\r\n", name, velocity, angle);
}

void construct_post_request(char *message, const char *name, double velocity, double angle) {
    snprintf(message, BUFFER_SIZE, "POST / HTTP/1.1\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Content-Length: %ld\r\n\r\n"
                                   "{\"name\":\"%s\",\"initial_velocity\":%lf,\"launch_angle\":%lf}\r\n",
                                   strlen("{\"name\":\"%s\",\"initial_velocity\":%lf,\"launch_angle\":%lf}") - 6 + strlen(name) + 2 * 8, name, velocity, angle);
}

void validate_arguments(int ac, char *av[]) {
    if (ac != 7 && ac != 5) {
        fprintf(stderr, "Usage: %s <IP> <PORT> <GET/POST> <Name> [Velocity] [Angle]\n", av[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(av[2]);

    if (strcmp(av[1], "127.0.0.1") != 0 && strcmp(av[1], "0.0.0.0") != 0) {
        fprintf(stderr, "Invalid IP address: %s\n", av[1]);
        exit(EXIT_FAILURE);
    }

    if (port < 1024 || port > 49151) {
        fprintf(stderr, "Invalid PORT: %s\n", av[2]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(av[3], "GET") != 0 && strcmp(av[3], "POST") != 0) {
        fprintf(stderr, "Invalid method: %s\n", av[3]);
        exit(EXIT_FAILURE);
    }

    if (strlen(av[4]) > 49) {
        fprintf(stderr, "Name too long: %s\n", av[4]);
        exit(EXIT_FAILURE);
    }

    if (ac == 7) {
        double velocity = atof(av[5]);
        double angle = atof(av[6]);
        
        if (velocity < 1.0 || velocity > (The_first_space_velocity - 1)) {
            fprintf(stderr, "Invalid Velocity: %s\n", av[5]);
            exit(EXIT_FAILURE);
        }
        if (angle < 1.1 || angle > 90.0) {
            fprintf(stderr, "Invalid Angle: %s\n", av[6]);
            exit(EXIT_FAILURE);
        }
    }
}

void draw_trajectory(SDL_Renderer *renderer, double v0, double angle, double max_height, double range) {
    double rad_angle = angle * M_PI / 180.0;
    double t, x, y;
    double time_step = 0.01;
    double g = 9.82;
    
    if (range > WINDOW_WIDTH || max_height > WINDOW_HEIGHT) {
      printf("\n\nWhen your max_height > WINDOW_HEIGHT or range > WINDOW_WIDTH you see static, fixed graphic\n\n");  
    }
    else{
        printf("\n\nYour max_height < WINDOW_HEIGHT and range < WINDOW_WIDTH, you see dinamic graphic\n\n");
    }

    for (t = 0; t <= 2 * v0 * sin(rad_angle) / g; t += time_step) {
        x = v0 * cos(rad_angle) * t;        
        y = v0 * sin(rad_angle) * t - 0.5 * g * t * t;        
        int screen_x = (int)(x / range * WINDOW_WIDTH);
        int screen_y = WINDOW_HEIGHT - (int)(y / max_height * WINDOW_HEIGHT);        
        if (range >= 0 && range < WINDOW_WIDTH && max_height >= 0 && max_height < WINDOW_HEIGHT) {
            SDL_RenderDrawPoint(renderer, x, WINDOW_HEIGHT - y);
        } 
        else {            
            SDL_RenderDrawPoint(renderer, screen_x, screen_y);
        }
    }

}



int main(int ac, char *av[]) {
  
    validate_arguments(ac, av);
    
    TrajectoryData data = {0};
    char * ip = av[1];
    int port = atof(av[2]);
    char * method = av[3];
    char * name = av[4];
    if (ac == 7) {
        data.initial_velocity = atof(av[5]);
        data.launch_angle = atof(av[6]);        
    } 

    int sock = create_and_connect_socket(ip, port);

    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    if (strcmp(method, "GET") == 0) {
        if (ac == 7) {
            construct_get_request(message, name, data.initial_velocity, data.launch_angle);
        } else {
            snprintf(message, BUFFER_SIZE, "GET /?name=%s HTTP/1.1\r\n\r\n", name);
        }
    } else if (strcmp(method, "POST") == 0) {
        if (ac == 7) {
            construct_post_request(message, name, data.initial_velocity, data.launch_angle);
        } else {
            fprintf(stderr, "POST request requires name, velocity, and angle parameters\n");
            exit(EXIT_FAILURE);
        }
    }
 
    send_request(sock, message);
    receive_response(sock, response, &data, ac);    
    
    
   
    if (ac == 5){
        printf("\n\nIf you want see graphic then send POST or GET request with initial_velocity and launch_angle\n\n");
        return 0;
    }


    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Projectile Trajectory", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        return 1;
    }

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);    
    draw_trajectory(renderer, data.initial_velocity, data.launch_angle, data.max_height, data.range);
    SDL_RenderPresent(renderer);

    int quit = 0;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
