#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>

// void handle_client(int client_socket);
// void handle_get_request(int client_socket, char *buffer);
// void handle_post_request(int client_socket, char *buffer);
// TrajectoryData calculate_trajectory(ProjectileParams params);
// void add_simulation_result(SimulationResult result);
// Node* get_list_node_for_name(Node *head, char *str);
// void free_list(Node *head);

#define BUFFER_SIZE 1024

typedef struct {
    char name[50];
    double initial_velocity;
    double launch_angle;
} ProjectileParams;

typedef struct {
    double max_height;
    double time_of_flight;
    double range;
} TrajectoryData;

typedef struct {
    char name[50];
    double initial_velocity;
    double launch_angle;
    double max_height;
    double time_of_flight;
    double range;
} SimulationResult;

typedef struct Node {
    SimulationResult data;
    struct Node* next;
} Node;

Node* head = NULL; 

void print_list(Node *head){
    Node *curr = head;
    while(curr != NULL){
        printf("Name: %s\n", curr->data.name);
        printf("Initial Velocity: %lf\n", curr->data.initial_velocity);
        printf("Launch Angle: %lf\n", curr->data.launch_angle);
        printf("Max Height: %lf\n", curr->data.max_height);
        printf("Time of Flight: %lf\n", curr->data.time_of_flight);
        printf("Range: %lf\n", curr->data.range);
        printf("\n");  
        curr = curr->next;
    }
}

Node *get_list_node_for_name(Node *head, char *str){
    Node *curr = head;
    while(curr != NULL){
        if (strcmp(curr->data.name, str) == 0){
            // printf("Name: %s\n", curr->data.name);
            // printf("Initial Velocity: %lf\n", curr->data.initial_velocity);
            // printf("Launch Angle: %lf\n", curr->data.launch_angle);
            // printf("Max Height: %lf\n", curr->data.max_height);
            // printf("Time of Flight: %lf\n", curr->data.time_of_flight);
            // printf("Range: %lf\n", curr->data.range);
            // printf("\n");
            return curr;  
        }
        curr = curr->next;
    }
    return NULL;
}



Node* create_node(SimulationResult value) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->data.name, value.name);
    new_node->data.initial_velocity = value.initial_velocity;
    new_node->data.launch_angle = value.launch_angle;
    new_node->data.max_height = value.max_height;
    new_node->data.time_of_flight = value.time_of_flight;
    new_node->data.range = value.range;
    new_node->next = NULL;
    return new_node;
    
}

TrajectoryData calculate_trajectory(ProjectileParams params) {
    TrajectoryData data;
    double g = 9.82;
    double theta = params.launch_angle * M_PI / 180.0;
    double v0 = params.initial_velocity;
    
    data.max_height = (sin(theta) * sin(theta)) / (2 * g) * v0 * v0;
    data.time_of_flight = (sin(theta)) / g * 2 * v0;
    data.range = (sin(2 * theta)) / g * v0 * v0;
    return data;
}


void add_simulation_result(SimulationResult result) {    
    Node * new_node = create_node(result);
    new_node->next = head;
    head = new_node;
}

void handle_post_request(int client_socket, char *buffer) {
    char response[BUFFER_SIZE];
    char* json_payload = strstr(buffer, "\r\n\r\n");
    char name[50] = {0};
    double velocity, angle;

    if (json_payload) {
        json_payload += 4; 
        double  max_height, time_of_flight, range;
        
        if(sscanf(json_payload, "{\"name\":\"%49[^\"]\",\"initial_velocity\":%lf,\"launch_angle\":%lf}", name, &velocity, &angle) == 3){            
            char response[BUFFER_SIZE];
            ProjectileParams params; 
            SimulationResult result;
            Node *curr = get_list_node_for_name(head, name);

            strncpy(params.name, name, sizeof(params.name) - 1);
            params.initial_velocity = velocity;
            params.launch_angle = angle;
            TrajectoryData data = calculate_trajectory(params);

            if (curr != NULL) {
                strncpy(curr->data.name, name, sizeof(curr->data.name) - 1);
                curr->data.initial_velocity=velocity;
                curr->data.launch_angle=angle;
                curr->data.max_height = data.max_height;
                curr->data.time_of_flight = data.time_of_flight;
                curr->data.range = data.range;
                snprintf(response, sizeof(response), "HTTP/1.1 200 OK\nContent-Type: text/plain\n\nSimulation data refresh stored successfully");                
            } else {                                    
                strncpy(result.name, name, sizeof(result.name) - 1);
                result.initial_velocity=velocity;
                result.launch_angle=angle;
                result.max_height = data.max_height;
                result.time_of_flight = data.time_of_flight;
                result.range = data.range;
                add_simulation_result(result);
                snprintf(response, sizeof(response), "HTTP/1.1 200 OK\nContent-Type: text/plain\n\nSimulation data stored successfully");                     
            }
            printf("Sending POST response:\n%s\n", response);
            write(client_socket, response, strlen(response));
            // print_list(head);
        } else {
            snprintf(response, sizeof(response), "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\n\nInvalid JSON payload");
            write(client_socket, response, strlen(response));
        } 
    }
    else {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\n\nNo JSON payload found in POST request");
        write(client_socket, response, strlen(response));
    }   
}

void handle_get_request(int client_socket, char *buffer) {
    char response[BUFFER_SIZE];
    double velocity, angle;
    char  name[50]={0};
    if (sscanf(buffer, "GET /?name=%49[^&]&velocity=%lf&angle=%lf", name, &velocity, &angle) == 3){                  
        ProjectileParams params;
        strncpy(params.name, name, sizeof(params.name) - 1);
        params.initial_velocity = velocity;
        params.launch_angle = angle;
        TrajectoryData data = calculate_trajectory(params);
        snprintf(response, sizeof(response),
                "HTTP/1.1 200 OK\nContent-Type: application/json\n\n"
                "\"max_height\": %.2f m\n\"time_of_flight\": %.2f s\n\"range\": %.2f m\n",
                data.max_height, data.time_of_flight, data.range);
        // printf("Sending GET response:\n%s\n", response);
        write(client_socket, response, strlen(response));
    } else if (sscanf(buffer, "GET /?name=%49[^ ]", name) == 1) {            
        Node *curr = get_list_node_for_name(head, name);
        if (curr != NULL) {     
            snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\nContent-Type: application/json\n\n"
                        "\"name\": \"%s\"\n\"initial_velocity\": %.2f m/s\n\"launch_angle\": %.2f o\n\"max_height\": %.2f m\n\"time_of_flight\": %.2f s\n\"range\": %.2f m",
                        curr->data.name, curr->data.initial_velocity, curr->data.launch_angle, curr->data.max_height, curr->data.time_of_flight, curr->data.range);
            // printf("Sending GET response:\n%s\n", response);
        } else {
            snprintf(response, sizeof(response),
                        "HTTP/1.1 404 Not Found\nContent-Type: text/plain\n\nSimulation data for %s not found", name);
            // printf("Sending 404 response:\n%s\n", response);
        }
        write(client_socket, response, strlen(response));
    } else {
        snprintf(response, sizeof(response),
                 "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\n\nInvalid GET request format");
        write(client_socket, response, strlen(response));
    }
}

void handle_client(int client_socket) {
   
    char buffer[BUFFER_SIZE];
    
    int read_size = read(client_socket, buffer, sizeof(buffer) - 1);
    if (read_size <= 0) {
        close(client_socket);
        return;
    }
    buffer[read_size] = '\0';

    printf("Received request:\n%s\n", buffer);
    if (strncmp(buffer, "GET", 3) == 0) {  
        handle_get_request(client_socket, buffer);
        
    } else if(strncmp(buffer, "POST", 4) == 0) {  
        handle_post_request(client_socket, buffer); 
        
    } else {
            printf("Unknown request type\n");
    }   
    close(client_socket);
}

void free_list(Node *head) {
    Node *tmp;

    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

int main(int ac, char *av[]) {
    if(ac != 3){
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", av[0]);
        exit(EXIT_FAILURE);
    }
    if (strcmp(av[1], "127.0.0.1") != 0 && strcmp(av[1], "0.0.0.0") != 0) {
        fprintf(stderr, "Invalid IP address: %s\n", av[1]);
        exit(EXIT_FAILURE);
    }
    if (atoi(av[2]) < 1024 || atoi(av[2]) > 49151){
        fprintf(stderr, "Invalid PORT: %s\n", av[2]);
        exit(EXIT_FAILURE);
    }

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    
    if ((server_fd = socket(AF_INET,  SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(av[1]);
    address.sin_port = htons(atoi(av[2]));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 7) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", atoi(av[2]));

    while ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        handle_client(client_socket);
    }

    close(server_fd);
    free_list(head);
    return 0;
}
