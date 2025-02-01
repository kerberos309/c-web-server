#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
/*
TODO:
1. CREATE CONFIG FILE.
2. RETRIEVE LIST OF PATH FOR REGISTERED API.
3. IF THE PATH IS NOT ON LIST OF REGISTERED API, RETURN 404 ERROR.
*/
#define PORT 55000
char **endpointsAndMethods;
void exitServer(int sig)
{
    //free global variables.
    free(endpointsAndMethods);
    printf("Exiting server...");
    exit(0);
}
void clearEndpoints(char *str, char ch)
{
    int i, j = 0;
    int length = strlen(str);

    for (i = 0; i < length; i++) {
        if (str[i] != ch) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}
void loadServer()
{
    FILE *serverConfigFile = fopen("server.conf", "r");
    if(!serverConfigFile)
    {
        perror("File opening failed.");
        exit(1);
    }

    fseek(serverConfigFile, 0, SEEK_END);
    long fileSize = ftell(serverConfigFile);
    rewind(serverConfigFile);

    char *buffer = (char *)malloc(fileSize + 1);
    if(!buffer)
    {
        perror("Memory allocation failed.");
        fclose(serverConfigFile);
        exit(1);
    }

    fread(buffer, 1, fileSize, serverConfigFile);
    buffer[fileSize] = '\0';

    printf("%s\n", buffer);
    char *route = strtok(buffer, "=");
    char *path_array = strtok(NULL, " ");
    printf("API PATHS: %s\n", path_array);
    //get how many endpoints
    int endpoints_count = 1;
    for (int i = 0; path_array[i] != '\0'; i++)
    {
        if(path_array[i] == ',')
        {
            endpoints_count++;
        }
    }
    printf("Number of endpoints: %d\n", endpoints_count);

    //iterate endpoints
    char **api_endpoints = malloc(endpoints_count * sizeof(char *));
    if (api_endpoints == NULL)
    {
        printf("Memory allocation failed.");
        exit(1);        
    }
    char *route_value = strtok(path_array, ",");
    int counter = 0;
    while(route_value != NULL)
    {
        api_endpoints[counter] = malloc(strlen(route_value)+1);
        strcpy(api_endpoints[counter], route_value);
        counter++;
        route_value = strtok(NULL,",");
    }
    //initiate size of "endpoints"
    endpointsAndMethods = (char **)malloc(endpoints_count * sizeof(char *));
    //display endpoints
    for (int k = 0; k < endpoints_count; k++)
    {
        //clear endpoints
        clearEndpoints(api_endpoints[k], '"');
        //store api endpoints into char[][]
        endpointsAndMethods[k] = api_endpoints[k];
        printf("API ENDPOINTS and METHOD: %s\n", endpointsAndMethods[k]);
        free(api_endpoints[k]);
    }
    free(buffer);
    fclose(serverConfigFile);
}

int main() 
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024]={0};
    //setup exit handler
    if(signal(SIGINT, exitServer) == SIG_ERR)
    {
        perror("signal");
        return 1;   
    }

    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello, World";

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Loading Server Configuration...");
    loadServer();
    printf("Listening on port %d\n", PORT);
    
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0)
    {
        read(new_socket, buffer, 1024);
        printf("Request:\n%s\n", buffer);
        char *first_line = strtok(buffer, "\n");
        printf("First Line: %s\n", first_line);
        char *method = strtok(first_line, " ");
        printf("Method: %s\n",method);
        char *path = strtok(NULL, " ");
        printf("Path: %s\n", path);

        //check if path exists in [endpointsAndMethods]

        send(new_socket, response, strlen(response), 0);
        close(new_socket);
    }

    perror("Accept failed");
    return 0;
}
