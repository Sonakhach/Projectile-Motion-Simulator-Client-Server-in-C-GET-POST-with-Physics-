#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/select.h>

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

void print_list(Node *head) {
    Node *curr = head;
    while (curr != NULL) {
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

Node* get_list_node_for_name(Node *head, char *str) {
    Node *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->data.name, str) == 0) {
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
    Node *new_node = create_node(result);
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
        if (sscanf(json_payload, "{\"name\":\"%49[^\"]\",\"initial_velocity\":%lf,\"launch_angle\":%lf}", name, &velocity, &angle) == 3) {
            ProjectileParams params;
            SimulationResult result;
            Node *curr = get_list_node_for_name(head, name);

            strncpy(params.name, name, sizeof(params.name) - 1);
            params.initial_velocity = velocity;
            params.launch_angle = angle;
            TrajectoryData data = calculate_trajectory(params);

            if (curr != NULL) {
                strncpy(curr->data.name, name, sizeof(curr->data.name) - 1);
                curr->data.initial_velocity = velocity;
                curr->data.launch_angle = angle;
                curr->data.max_height = data.max_height;
                curr->data.time_of_flight = data.time_of_flight;
                curr->data.range = data.range;
                snprintf(response, sizeof(response),
                    "HTTP/1.1 200 OK\nContent-Type: application/json\n\n"
                    "{\"max_height\": %.2f, \"time_of_flight\": %.2f, \"range\": %.2f}\n\nSimulation data refresh stored successfully\n\n",
                    data.max_height, data.time_of_flight, data.range);
            } else {
                strncpy(result.name, name, sizeof(result.name) - 1);
                result.initial_velocity = velocity;
                result.launch_angle = angle;
                result.max_height = data.max_height;
                result.time_of_flight = data.time_of_flight;
                result.range = data.range;
                add_simulation_result(result);
                snprintf(response, sizeof(response),
                    "HTTP/1.1 200 OK\nContent-Type: application/json\n\n"
                    "{\"max_height\": %.2f, \"time_of_flight\": %.2f, \"range\": %.2f}\n\nSimulation data stored successfully\n\n",
                    data.max_height, data.time_of_flight, data.range);
            }

            printf("Sending POST response:\n%s\n", response);
            write(client_socket, response, strlen(response));
            print_list(head);
        } else {
            snprintf(response, sizeof(response), "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\n\nInvalid JSON payload");
            write(client_socket, response, strlen(response));
        }
    } else {
        snprintf(response, sizeof(response), "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\n\nNo JSON payload found in POST request");
        write(client_socket, response, strlen(response));
    }
}

void handle_get_request(int client_socket, char *buffer) {
    char response[BUFFER_SIZE];
    double velocity, angle;
    char name[50] = {0};

    if (sscanf(buffer, "GET /?name=%49[^&]&velocity=%lf&angle=%lf", name, &velocity, &angle) == 3) {
        ProjectileParams params;
        strncpy(params.name, name, sizeof(params.name) - 1);
        params.initial_velocity = velocity;
        params.launch_angle = angle;
        TrajectoryData data = calculate_trajectory(params);
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\nContent-Type: application/json\n\n"
            "{\"max_height\": %.2f, \"time_of_flight\": %.2f, \"range\": %.2f}",
            data.max_height, data.time_of_flight, data.range);
        printf("Sending GET response:\n%s\n", response);
        write(client_socket, response, strlen(response));
    } else if (sscanf(buffer, "GET /?name=%49[^ ]", name) == 1) {
        Node *curr = get_list_node_for_name(head, name);
        if (curr != NULL) {
            snprintf(response, sizeof(response),
                        "HTTP/1.1 200 OK\nContent-Type: application/json\n\n"
                        "\"name\": \"%s\"\n\"initial_velocity\": %.2f m/s\n\"launch_angle\": %.2f o\n\"max_height\": %.2f m\n\"time_of_flight\": %.2f s\n\"range\": %.2f m",
                        curr->data.name, curr->data.initial_velocity, curr->data.launch_angle, curr->data.max_height, curr->data.time_of_flight, curr->data.range);            
        } else {
            snprintf(response, sizeof(response), "HTTP/1.1 404 Not Found\nContent-Type: text/plain\n\nNo stored simulation found with name: %s", name);
        }
        printf("Sending GET response:\n%s\n", response);
        write(client_socket, response, strlen(response));
    } else {
        snprintf(response, sizeof(response), "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\n\nInvalid query parameters");
        write(client_socket, response, strlen(response));
    }
}


void handle_new_connection(int server_socket, int *max_fd, fd_set *master_fds) {
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
    if (client_socket < 0) {
        perror("Accept error");
    } else {
        FD_SET(client_socket, master_fds);
        if (client_socket > *max_fd) {
            *max_fd = client_socket;
        }
        printf("New connection: socket %d\n", client_socket);
    }
}

void handle_client_request(int client_socket, fd_set *master_fds) {
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            // printf("Socket %d closed connection\n", client_socket);
        } else {
            perror("Read error");
        }
        close(client_socket);
        FD_CLR(client_socket, master_fds);
    } else {
        buffer[bytes_read] = '\0';
        if (strncmp(buffer, "POST /", 6) == 0) {
            handle_post_request(client_socket, buffer);
        } else if (strncmp(buffer, "GET /", 5) == 0) {
            handle_get_request(client_socket, buffer);
        } else {
            char response[] = "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\n\nUnsupported request method";
            write(client_socket, response, sizeof(response) - 1);
        }
    }
}

void setup_server(int *server_socket, struct sockaddr_in *server_addr, char *ip, int port) {
    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(*server_socket);
        exit(EXIT_FAILURE);
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = inet_addr(ip);
    server_addr->sin_port = htons(port);

    if (bind(*server_socket, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0) {
        perror("Bind failed");
        close(*server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(*server_socket, 10) < 0) {
        perror("Listen failed");
        close(*server_socket);
        exit(EXIT_FAILURE);
    }
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
    char *ip = av[1];
    int port = atoi(av[2]);

    if (strcmp(av[1], "127.0.0.1") != 0 && strcmp(av[1], "0.0.0.0") != 0) {
        fprintf(stderr, "Invalid IP address: %s\n", ip);
        exit(EXIT_FAILURE);
    }
    if (port < 1024 || port > 49151){
        fprintf(stderr, "Invalid PORT: %d\n", port);
        exit(EXIT_FAILURE);
    }

    int server_socket;
    struct sockaddr_in server_addr;    
    fd_set read_fds, master_fds;
    int max_fd;

    setup_server(&server_socket, &server_addr, ip, port);
    FD_ZERO(&master_fds);
    FD_SET(server_socket, &master_fds);
    max_fd = server_socket;

    printf("Server listening on port %d\n", atoi(av[2]));

    while (42) {
        read_fds = master_fds;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        for (int fd = 3; fd <= max_fd; fd++) {
            
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == server_socket) {
                    handle_new_connection(server_socket, &max_fd, &master_fds);
                    
                } else {
                    handle_client_request(fd, &master_fds);
                    
                }
            }
        }
    }

    close(server_socket);
    free_list(head);
    return 0;
}
