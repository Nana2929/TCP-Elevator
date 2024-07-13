#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
#include <thread>
#include <ctime>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********
 * ELEVATOR_SIGNAL_PRESS_1F = 1
 * ELEVATOR_SIGNAL_PRESS_1F = 2
 * ELEVATOR_SIGNAL_FLOOR_1_UP = 3
 * ELEVATOR_SIGNAL_FLOOR_2_DOWN = 4
 **********/

// resources: https://shengyu7697.github.io/cpp-windows-tcp-socket/
class Elevator {
private:
    int currState = 0; // 0: 1F-IDLE, 1: 1F-OPEN, 2: LIFT, 3: 2F-OPEN, 4: 2F-IDLE, 5: GO-DOWN
    int currFloor = 1;
    std::time_t actTime = 0;
    bool pressedBtns[5]; // 1-indexed
    std::mutex mtx; // `pressedBtns` has sync issues; use mutex lock to protect it!
    const int OPEN_TIME = 2; // unit: seconds
    const int MOVE_TIME = 5;
    // thread-safe reading and writing
    bool isPressed(const int btn) {
        std::lock_guard<std::mutex> lock(mtx);
        return pressedBtns[btn];
    }
    void cleanBtn(int btn){
        std::lock_guard<std::mutex> lock(mtx);
        pressedBtns[btn]=false;
    }
    static std::string translateState(const int state) {
        static std::unordered_map<int, std::string> state_map = {
            {0, "➡️⬅️ 1F-IDLE"},
            {1, "⬅️➡️ 1F-OPEN"},
            {2, "🔼 LIFT"},
            {3, "⬅️➡️ 2F-OPEN"},
            {4, "➡️⬅️ 2F-IDLE"},
            {5, "🔽 GO-DOWN"}
        };
        if (state_map.find(state) != state_map.end()) {
            return state_map[state];
        } else {
            return "UNKNOWN";
        }
    }

public:
    // constructor
    Elevator (): currState(0), currFloor(1), actTime(0){
        std::fill(pressedBtns, pressedBtns + 5, false);
    };
    // destructor
    ~Elevator () {};
    // ** print_thread
    void printState(){
        while (true){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << translateState(currState) << std::endl;
        }}
    // thread-safe writing; user-exposed API
    // ** listening
    void pressBtn(const int btn){
        std::lock_guard<std::mutex> lock(mtx);
        pressedBtns[btn]=true;
        std::cout << "Button " << btn << " is pressed." << std::endl;
    }
    // thread 2
    // ** control_thread
    void stateTransit(){
        while (true){
            // 1F-IDLE
            if (currState == 0){
                if (isPressed(1) || isPressed(3)){
                    currState = 1; // 1F-OPEN
                    actTime = std::time(0);
                    cleanBtn(1);
                    cleanBtn(3);
                }
                else if (isPressed(2) || isPressed(4)){
                    currState = 2; // LIFT
                    actTime = std::time(0);
                    cleanBtn(2);
                    cleanBtn(4);
                }
            }
            // 1F-OPEN
            else if (currState == 1){
                    // exception handling: if in the middle of opening, 1/3 is pressed, we need to reset the timer and reopne the door
                    while (std::difftime(std::time(0), actTime) < OPEN_TIME){
                        if (isPressed(1) || isPressed(3)){
                            actTime = std::time(0);
                            cleanBtn(1);
                            cleanBtn(3);
                        }
                    }
                    currState = 0;
                }
            // LIFT
            else if (currState == 2){

                while (std::difftime(std::time(0), actTime) < MOVE_TIME){;
                    }
                cleanBtn(1); // clean internal butttons
                cleanBtn(2); // clean internal butttons
                cleanBtn(4); // it is moving up anyway, not taking an effect
                currState = 3;
                actTime = std::time(0); // transit to 2F-OPEN
                } // 2F-OPEN
            // 2F-OPEN
            else if (currState == 3){
                // exception handling
                while (std::difftime(std::time(0), actTime) < OPEN_TIME){
                    if (isPressed(2) || isPressed(4)){
                        actTime = std::time(0);
                        cleanBtn(2);
                        cleanBtn(4);
                    }
                }
                currState = 4;
                actTime = std::time(0);}
            // 2F-IDLE
            else if (currState == 4){
                if (isPressed(1) || isPressed(3)){
                    currState = 5; // GO-DOWN
                    actTime = std::time(0);
                    cleanBtn(1);
                    cleanBtn(3);
                }
                else if (isPressed(2) || isPressed(4)){
                    currState = 2; // LIFT
                    actTime = std::time(0);
                    cleanBtn(2);
                    cleanBtn(4);
                }
            }
            // GO-DOWN
            else if (currState == 5){
                while (std::difftime(std::time(0), actTime) < MOVE_TIME){;}
                cleanBtn(1); // clean internal butttons
                cleanBtn(2); // clean internal butttons
                cleanBtn(3); // it is moving down anyway, not taking an effect
                currState = 1;
                actTime = std::time(0); // transit to 1F-OPEN
            }
    } // end of while
}}
;




void handle_signal(int signal, int client_fd, Elevator& elevator) {
    char response[1024] = {0};
    switch(signal) {
        case '1':
            snprintf(response, sizeof(response), "Received signal 1");
            elevator.pressBtn(1);
            break;
        case '2':
            snprintf(response, sizeof(response), "Received signal 2");
            elevator.pressBtn(2);
            break;
        case '3':
            snprintf(response, sizeof(response), "Received signal 3");
            elevator.pressBtn(3);
            break;
        case '4':
            snprintf(response, sizeof(response), "Received signal 4");
            elevator.pressBtn(4);
            break;
        default:
            return; // Ignore all other signals
    }
    send(client_fd, response, strlen(response), 0);
}

// void run() {
//         std::thread print_thread(&Elevator::printState, this);
//         std::thread control_thread(&Elevator::stateTransit, this);
//         print_thread.join();
//         control_thread.join();

//     }

void serve(Elevator & elevator){
    const char* host = "0.0.0.0";
    int port = 7000;

    int sock_fd, new_fd;
    socklen_t addrlen;
    struct sockaddr_in my_addr, client_addr;
    int status;
    char indata[1024] = {0};
    int on = 1;

    // create a socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("Socket creation error");
        exit(1);
    }

    // for "Address already in use" error message
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1) {
        perror("Setsockopt error");
        exit(1);
    }

    // server address
    my_addr.sin_family = AF_INET;
    inet_aton(host, &my_addr.sin_addr);
    my_addr.sin_port = htons(port);


    status = bind(sock_fd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if (status == -1) {
        perror("Binding error");
        exit(1);
    }
    printf("🛗 server starts at: %s:%d\n", inet_ntoa(my_addr.sin_addr), port);

    status = listen(sock_fd, 5);
    if (status == -1) {
        perror("Listening error");
        exit(1);
    }
    printf("wait for connection...\n");
    addrlen = sizeof(client_addr);
    // print this
    std::cout << "-----------------------------------------------" << std::endl;
    std::cout << "* ELEVATOR_SIGNAL_PRESS_1F     = 1" << std::endl;
    std::cout << "* ELEVATOR_SIGNAL_PRESS_1F     = 2" << std::endl;
    std::cout << "* ELEVATOR_SIGNAL_FLOOR_1_UP   = 3" << std::endl;
    std::cout << "* ELEVATOR_SIGNAL_FLOOR_2_DOWN = 4" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;

    while (true) {
        // connecting to client
        new_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addrlen);
        printf("connected by %s:%d\n", inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));
        // serving one client
        while (true) {
            int nbytes = recv(new_fd, indata, sizeof(indata), 0);
            if (nbytes <= 0) {
                close(new_fd);
                printf("client closed connection.\n");
                break;
            }
            printf("recv: %s\n", indata);

            // Only handle the first character of indata
            if (nbytes > 0) {
                handle_signal(indata[0], new_fd, elevator);
            }
        }
    }
    close(sock_fd);
}

int main() {
    Elevator elevator;
    std::thread print_thread(&Elevator::printState, &elevator);
    std::thread control_thread(&Elevator::stateTransit, &elevator);
    std::thread serve_thread(serve, std::ref(elevator));
    print_thread.join();
    control_thread.join();
    serve_thread.join();
    return 0;
}