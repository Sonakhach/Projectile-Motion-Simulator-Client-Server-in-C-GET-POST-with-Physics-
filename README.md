# Projectile-Motion-Simulator-Client-Server-in-C-GET-POST-with-Physics-
## Step 1: Compile the Programs
For the server, you need the basic networking libraries, and for the client, you need SDL2 for graphical rendering.

Install SDL Library: Use sudo apt install libsdl2-dev to install the SDL2 development libraries on Linux.
```
sudo apt install libsdl2-dev
```
Link Math Library: Use -lm when compiling to link against the math library (libm).

Compile SDL Programs: Use -lSDL2 to link against the SDL2 library when compiling SDL programs.

To compile the client application:
```
gcc client.c -o client -lSDL2 -lm
```
To compile the server application:
```
gcc server.c -o server -lm
```
## Step 2: Run the Server
Start the server by providing the IP address and port number as arguments. For example, to run the server on localhost with port 8080:
```
./server 127.0.0.1 8080
```
The server will start listening for incoming connections on the specified IP address and port.

## Step 3: Run the Client
You can run the client with either a GET or POST request. The client requires the server IP, port, request method (GET/POST), and the name parameter. Optionally, you can provide velocity and angle parameters for the GET request. For the POST request, these parameters are mandatory.

Running a GET Request:
To run a GET request without velocity and angle:

```
./client 127.0.0.1 8080 GET "ProjectileName"
```
To run a GET request with velocity and angle:

```
./client 127.0.0.1 8080 GET "ProjectileName" 50.0 45.0
```
Running a POST Request:
To run a POST request with velocity and angle:

```
./client 127.0.0.1 8080 POST "ProjectileName" 50.0 45.0
```
## Step 4: View the Client Output
After sending the request, the client will display the request sent and the server's response. For GET requests, the server will calculate the trajectory based on the given parameters and return the result. For POST requests, the server will store or update the trajectory data in a linked list.

## Step 5: Graphical Rendering
The client application will also open a window using SDL2 to graphically display the trajectory of the projectile based on the initial velocity and launch angle provided. You can close the window by clicking the close button or pressing the escape key.

### Example Usage

Run a client with a GET request:
```
./client 127.0.0.1 8080 GET "TestProjectile" 50.0 45.0
```

**Additional Notes**
Ensure the server is running before starting the client.
The server can handle multiple clients but processes one request at a time.
The client will render the trajectory in a window and close it when you quit the SDL window.


# Physics Principles of Projectile Motion
The principles of physics that are used to model the motion of projectiles are brought to the problem of a body thrown at an angle. The origin of the coordinate axes is the projectile launch point. When the initial velocity is directed to an angle, the Vo vector must initially be converted into horizontal and vertical components. Projecting equations of motion S=Vo t + a bury t2/2 and acceleration a = (V-Vo)/t on X and Y axes, we will have: 

Vy =Vo*sina – g*t           and   Vy = 0    =>    0 = Vo*sina – g*t                =>    **t =  Vo*sina/g** 

Y =  Vo*sina*t– g*t*t/2   and   Y = H    =>    H = Vo*sina*t – g*t*t/2  

Vx = Vo*cosa                                              				

X = Vo*cosa*T      	     and    X = L    =>     L = 2*Vo*cosa*Vo*sina/g  =>    **L = Vo*Vo*sin2a/g**      


![im1](https://github.com/Sonakhach/Projectile-Motion-Simulator-Client-Server-in-C-GET-POST-with-Physics-/blob/main/image3-9.png)

The max height is that height H (Y) that the body manages to reach before the vertical component of the velocity (Vy) becomes zero. The distance of the flight is the module L of the horizontal movement (X), which the body manages to make during the duration of the flight, that is, from the moment of throwing (To = 0) to the moment of falling to the ground (Y = 0).

To = 0     **T = 2*Vo*sina/g**

The value To = 0 corresponds to the moment of the throw, and T is the time spent on the entire path.

**H = Vo*sina*t – g*t*t/2**     and     **t =  Vo*sina/g**      =>  **H =  Vo*sina* Vo*sina/2*g**

A thrown shot at an initial speed in the horizontal direction will quickly immerse itself in the ground, if shot vertically upwards, it will fall where it was thrown, and in order to throw as far as possible, it is necessary to hold the cannon at an angle to the horizon. And we have the max flight distance, when we hold the cannon at an angle of 45 degrees to the horizon /the surface of the image obtained by the velocity vectors is square/.

Let's note that in solving the above problem, we ignore the air resistance, but in real life it can play a crucial role.

```
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
```
T = 2*Vo*sina/g                       <=>  data.time_of_flight = (sin(theta)) / g * 2 * v0;

L = Vo*Vo*sin2a/g                   		<=>  data.range = (sin(2 * theta)) / g * v0 * v0;

H =  Vo*sina* Vo*sina/2*g  <=>  max_height = (sin(theta) * sin(theta)) / (2 * g) * v0 * v0;

The above formulas are equivalent, but I presented them in the structure so that the first one divides in a new multiplication in order to receive double number does not have a problem of going out of bounds.

• **theta = params.launch_angle * M_PI / 180.0**

this formula converts the degree measure of an angle to a radian measure

• **g = 9.82**    is the acceleration due to gravity (9.82 m/s2 near the surface of the earth).
• #define The_first_space_velocity **8000**

A body that has developed this speed and is moving horizontally with respect to the surface of the planet will not fall on it, but will continue to move in a circular orbit.

# Data Structures

ProjectileParams type is used to store input arguments.
```
typedef struct {
char name[50];
double initial_velocity;
double launch_angle;
} ProjectileParams;
```
TrajectoryData is used to store calculations that are done on the server.
```
typedef struct {
double max_height;
double time_of_flight;
double range;
} TrajectoryData;  
```

SimulationResult is used to store data involved in the above mentioned two structures.
```
typedef struct {
char name[50];
double initial_velocity;
double launch_angle;
double max_height;
double time_of_flight;
double range;
} SimulationResult;
```
 Each node has a data storage area /data/ and an address storage area /next/. Each time the arguments given to the program and the calculations performed in the simulation are stored in the SimulationResult type at some address of the Node, a new Node is created for the next entered data. These two nodes are linked by addresses.
```
typedef struct Node {
SimulationResult data;
struct Node* next;
} Node;
```

### Handling POST Requests
1. Extracting JSON Payload:
    - The server extracts the JSON payload from the request body.
2. Parsing JSON:
    - The server parses the JSON payload to extract the parameters (name, initial velocity, launch angle).
3. Computing Trajectory:
    - The server computes the projectile motion trajectory based on the provided parameters.
4. Storing Results:
    - The server stores the simulation result in a linked list.
5. Returning Confirmation:
    - The server returns a confirmation message indicating that the data has been successfully stored.
### Handling Errors
    - The server returns appropriate HTTP status codes and error messages for invalid requests, missing parameters, or JSON parsing errors.

## Client-Side Logic
### Sending Requests
1. Socket Creation:
    - The client creates a socket and connects it to the server using the provided IP address and port number.
2. Constructing Requests:
    - The client constructs GET or POST requests based on the provided parameters.
### Handling GET Requests
1. Constructing GET Request:
    - The client builds a GET request URL with the name, initial velocity, and launch angle as query parameters or requests by the name of projectile, that was posted in advance.
2. Sending Request:
    - The client sends the GET request to the server.
3. Receiving Response:
    - The client reads the server's response and processes the JSON data to display the trajectory results.
#### Handling POST Requests
1. Constructing POST Request:
    - The client builds a POST request with a JSON payload containing the name, initial velocity, and launch angle.
2. Sending Request:
    - The client sends the POST request to the server.
3. Receiving Response:
    - The client reads the server's response and displays the confirmation message.
### Handling Errors
  - The client handles connection errors, timeouts, and invalid responses by printing appropriate error messages and retrying if necessary.

##### These are prototype functions for handling GET and POST requests for projectile motion data on both the server and client sides.

**In server.c** 
```
void handle_post_request(int client_socket, char *buffer);
void handle_get_request(int client_socket, char *buffer);
```
**in client.c**
```
void send_request(int sock, const char *message) ;
void receive_response(int sock);
void construct_get_request(char *message, const char *name, double velocity, double angle) ;
void construct_post_request(char *message, const char *name, double velocity, double angle) ;
```
