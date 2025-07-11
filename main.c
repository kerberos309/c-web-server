#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 55000
char **global_endpointsAndMethods;
int global_EndpointsAndMethodsCounter = 0;
#pragma region RESPONSE MANAGER
char *responseManager(const char *apiPath, const char *method, char *registeredApiPath){
    /*DESCRIPTION
    ->accepts path and method
    ->run function to accomodate request based on method(e.g., POST=>create item, GET=>retrieve item)
    ->read/create items from database(sqlite)*/
    printf("checking %s...\n", registeredApiPath);
    char **emails;
    int step_counter = 0;

    sqlite3 *database;
    sqlite3_stmt *statement;
    int rc;
    printf("Connecting to database...\n");
    rc = sqlite3_open("main.db", &database);//TODO: put the database name in server.conf

    if(rc != SQLITE_OK)
    {
        printf("Can't open database: %s\n", sqlite3_errmsg(database));
        exit(1);
    }else
    {
        printf("Connection Succes!\n");
    }

    char *response = NULL;

    if(strcmp(method, "GET") == 0)
    {
        printf("fetching data...\n");
        char *slugChecker = strchr(registeredApiPath, '{');
        const char initialQuery[] = "SELECT email, username FROM users";//TODO: MAKE THE FIELDS AND TABLE DYNAMIC
        size_t initialQueryLength = strlen(initialQuery);
        char *query;
        printf("slugChecker: %s\n", slugChecker);
        if(slugChecker == NULL)
        {
            printf("path is without {\n");
            query = malloc(initialQueryLength + sizeof(char));
            if(!query)
            {
                printf("Memory allocation failed\n");
                exit(1);
            }

            strcpy(query, initialQuery);

            rc = sqlite3_prepare_v2(database, query, -1, &statement, 0);

            if(rc != SQLITE_OK)
            {
                printf("Failed to prepare statement: %s\n", sqlite3_errmsg(database));
                sqlite3_close(database);
                exit(1);
            }

            response = malloc(256);
            if(!response)
            {
                printf("Memory allocation failed\n");
                sqlite3_finalize(statement);
                sqlite3_close(database);
                exit(1);
            }
            //assign memory to [emails]
            emails = malloc(256 * sizeof(char *));

            //validate
            if(!emails)
            {
                printf("Memory allocation failed\n");
                sqlite3_finalize(statement);
                sqlite3_close(database);
                exit(1);
            }

            //
            //assign char array
            while(sqlite3_step(statement) == SQLITE_ROW)
            {
                const char *email = (const char *)sqlite3_column_text(statement, 0);
                size_t email_size = strlen(email) + 1;
                emails[step_counter] = (char *)malloc(email_size * sizeof(char));
                if(!emails[step_counter])
                {
                    printf("Memory allocation failed\n");
                    sqlite3_finalize(statement);
                    sqlite3_close(database);
                    exit(1);
                }

                strcpy(emails[step_counter], email);
                step_counter++;
            }

            char *email_concat;
            //get total size of email char array
            size_t email_total_size = 0;
            for(int i = 0; i < step_counter; i++)
            {
                email_total_size = email_total_size + strlen(emails[i]);
            }
            //add 1 to [email_total_size]
            email_total_size += 1;    
            //assign the size to [email_concat]
            email_concat = (char *)malloc(email_total_size * sizeof(char));

            //concat [emails] to [email_concat]
            email_concat[0] = '\0'; 
            for(int i = 0; i < step_counter; i++)
            {
                strcat(email_concat, emails[i]);
            }

            //display [email_concat]
            snprintf(response, 256, "user:{'email':'%s'}", email_concat);//TODO: FIX DISPLAY OF EMAILS

            //free memory
            free(query);
            free(email_concat);
            for(int i = 0; i < step_counter; i++)
            {
                free(emails[i]);
            }
            free(emails);
        }
        else
        {
            printf("path is with {\n");
            int slugPosition = slugChecker - registeredApiPath;
            int slugNum = atoi(&apiPath[slugPosition]);
            char additionalQuery[16];
            snprintf(additionalQuery, sizeof(additionalQuery), " where id = %d;", slugNum);
            size_t additionalQueryLength = strlen(additionalQuery);
            size_t totalQueryLength = initialQueryLength + additionalQueryLength + 1;//add 1 memory for null terminator

            query = malloc(totalQueryLength + sizeof(char));

            strcpy(query, initialQuery);
            strcat(query, additionalQuery);

            rc = sqlite3_prepare_v2(database, query, -1, &statement, 0);

            if(rc != SQLITE_OK)
            {
                printf("Failed to prepare statement: %s\n", sqlite3_errmsg(database));
                sqlite3_close(database);
                exit(1);
            }

            while(sqlite3_step(statement) == SQLITE_ROW)
            {
                response = malloc(256);
                if(!response)
                {
                    printf("Memory allocation failed\n");
                    sqlite3_finalize(statement);
                    sqlite3_close(database);
                    exit(1);
                }
                const char *email = (const char *)sqlite3_column_text(statement, 0);
                const char *username = (const char *)sqlite3_column_text(statement, 1);
                snprintf(response, 256, "user:{'email':'%s','username':'%s'}", email, username);
                // printf("users:{'email':'%s','username':'%s'}\n", email, username);
            }
        }
        
        
    }

    if(strcmp(method, "POST") == 0)
    {
        printf("creating data...\n");
        response = malloc(32);
        if(!response)
        {
            printf("Memory allocation failed\n");
            sqlite3_finalize(statement);
            sqlite3_close(database);
            exit(1);
        }
        snprintf(response, 32, "POST request received");
    }
    printf("Closing connection to database...\n");
    sqlite3_finalize(statement);
    sqlite3_close(database);
    printf("Database is closed\n");
    if(!response)
    {
        response = malloc(16);
        if(!response)
        {
            printf("Memory allocation failed\n");
            exit(1);
        }
        strcpy(response, "no data");
    }

    return response;
}
#pragma endregion
#pragma region PATH VALIDATION

int pathValidation(char *requestPath, char *registeredPath)
{
    /*
    DESCRIPTION:
    ->TO VALIDATE THE REGISTERED PATH FROM CONFIG FILE AND REQUEST PATH
    */

    char *charPosition = strchr(registeredPath, '{');
    //TODO: IDENTIFY MULTIPLE SLUGS, AND CONFIRM IF IT IS STRING(CHAR[]) OR INT
    if(charPosition != NULL)
    {
        int position = charPosition - registeredPath;
        printf("position int: %d\n", position);
        printf("whole path: %s\n", requestPath);
        if(position < strlen(requestPath))
        {
            printf("value: %c\n", requestPath[position]);
            if(isdigit(requestPath[position]))
            {
                printf("Valid\n");
                return 1;
            }
            else
            {
                printf("Invalid\n");
                return 0;
            }
        }
        else
        {
            printf("Error: position %d out of bounds for requestPath \"%s\"\n", position, requestPath);
            return 0;
        }
    }
    else
    {
        printf("Character not found\n");
        return 0;
    }
} 
#pragma endregion
#pragma region STRING DUPLICATE
char *strdup(const char * s)
{
    /*
    Description
    ->function to duplicate string(char[])
    */
    if (s == NULL) {
        return NULL; // Handle null input
    }

    size_t len = strlen(s) + 1; // +1 for the null terminator
    char *new_s = malloc(len);

    if (new_s == NULL) {
        return NULL; // Handle memory allocation failure
    }

    memcpy(new_s, s, len); // Use memcpy for efficiency
    return new_s;
}
#pragma endregion
#pragma region EXIT SERVER
void exitServer(int sig)
{
    //free global variables.
    free(global_endpointsAndMethods);
    printf("Exiting server...");
    exit(0);
}
#pragma endregion
#pragma region CLEAR ENDPOINTS
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
#pragma endregion
#pragma region LOAD CONFIG
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

    char *buffer = malloc((fileSize + 1) * sizeof(char));
    if(!buffer)
    {
        perror("Memory allocation failed.");
        fclose(serverConfigFile);
        exit(1);
    }

    size_t bytesRead = fread(buffer, 1, fileSize, serverConfigFile);
    buffer[bytesRead] = '\0';

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
    char *path_array_copy = malloc((strlen(path_array) + 1) * sizeof(char));
    if(!path_array_copy)
    {
        printf("Memory allocation failed!");
        exit(1);
    }
    strcpy(path_array_copy, path_array);
    char *route_value = strtok(path_array_copy, ",");
    int counter = 0;
    while(route_value != NULL)
    {
        printf("route_value: %s\n", route_value);
        api_endpoints[counter] = malloc((strlen(route_value) + 1) * sizeof(char));
        if(api_endpoints[counter]==NULL)
        {
            perror("api_endpoints strdup failed");
            exit(1);
        }
        //assign value of route_value to api_endpoints[counter]
        strcpy(api_endpoints[counter], route_value);
        counter++;
        route_value = strtok(NULL,",");
    }
    //set the total count of endpoints to global variable
    global_EndpointsAndMethodsCounter = counter;
    //initiate size of "endpoints"
    global_endpointsAndMethods = malloc(endpoints_count * sizeof(char *));
    if(global_endpointsAndMethods == NULL)
    {
        perror("global_endpointsAndMethods malloc failed.");
        exit(1);
    }
    //display endpoints
    for (int k = 0; k < endpoints_count; k++)
    {
        if(api_endpoints[k])
        {
            //initialize api_endpoints[k]
            size_t api_endpoints_size = strlen(api_endpoints[k]) + 1;
            char *str = malloc(api_endpoints_size * sizeof(char));
            if(!str)
            {
                printf("Memory allocation failed!");
                exit(1);
            }
            //copy api_endpoints[k] to str
            strcpy(str, api_endpoints[k]);
            //clear endpoints
            clearEndpoints(str, '"');
            //initialize global_endpointsAndMethods[k]
            global_endpointsAndMethods[k] = malloc((strlen(str)+1)*sizeof(char));
            if(!global_endpointsAndMethods[k])
            {
                printf("Memory allocation failed!");
                exit(1);
            }
            //copy str to global_endpointsAndMethods[k]
            strcpy(global_endpointsAndMethods[k], str);
            //free str
            free(str);
            printf("API ENDPOINTS and METHOD: %s\n", global_endpointsAndMethods[k]);
        }
    }
    for(int j = 0; j < endpoints_count; j++)
    {
        free(api_endpoints[j]);
    }
    free(path_array_copy);
    free(api_endpoints);
    free(buffer);
    fclose(serverConfigFile);
}
#pragma endregion
#pragma region MAIN FUNC
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

    char *response = NULL;

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
    printf("Loading Server Configuration...\n");
    loadServer();
    printf("Listening on port %d\n", PORT);
    
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0)
    {
        memset(buffer,0,sizeof(buffer));
        read(new_socket, buffer, 1023);
        printf("Request:\n%s\n", buffer);
        char *first_line = strtok(buffer, "\n");
        printf("First Line: %s\n", first_line);
        char *method = strtok(first_line, " ");
        printf("Method: %s\n",method);
        char *path = strtok(NULL, " ");
        printf("Path: %s\n", path);
        //iterate endpoints
        int isPathRegistered = 0;
        char *registeredApiPath = NULL;
        for(int p = 0; p < global_EndpointsAndMethodsCounter; p++)
        {
            printf("Global Endpoints: %s\n", global_endpointsAndMethods[p]);
            char *registeredEndpoint = strtok(strdup(global_endpointsAndMethods[p]), ":");
            
            if(registeredEndpoint == NULL)
            {
                perror("registeredEndpoint strdup failed");
                exit(1);
            }

            clearEndpoints(registeredEndpoint, '[');
            clearEndpoints(registeredEndpoint, ']');
            clearEndpoints(registeredEndpoint, '\'');
            printf("Endpoints: %s\n", registeredEndpoint);
            printf("Request path: %s\n", path);
            //filter out registered path with '{}'
            char *endpointChecker = strchr(registeredEndpoint, '{');
            
            if(endpointChecker != NULL)
            {
                //validate the parameter for resource endpoints
                char *dup_path, *dup_endpoint;
                size_t dup_path_len, dup_endpoint_len;
                dup_path_len = strlen(path) + 1;
                dup_endpoint_len = strlen(registeredEndpoint) + 1;

                dup_path = malloc(dup_path_len * sizeof(char));
                dup_endpoint = malloc(dup_endpoint_len * sizeof(char));
                if(dup_path == NULL || dup_endpoint == NULL)
                {
                    perror("memory allocation failed");
                    exit(1);
                }

                strcpy(dup_path, path);
                strcpy(dup_endpoint, registeredEndpoint);
                isPathRegistered = pathValidation(dup_path,dup_endpoint);

                if(isPathRegistered != 0)
                {
                    printf("copying registered endpoint...\n");
                    size_t registeredEndpointLen = strlen(registeredEndpoint) + 1;
                    registeredApiPath = malloc(registeredEndpointLen * sizeof(char));
                    if(registeredApiPath == NULL)
                    {
                        printf("memory allocation failed.\n");
                        exit(1);
                    }
                    strcpy(registeredApiPath, registeredEndpoint);
                }

                

                free(dup_path);
                free(dup_endpoint);
                free(registeredEndpoint);
                break;
            }
            //filter out registered path without '{}'
            if(strcmp(registeredEndpoint, path) == 0)
            {
                isPathRegistered = 1;
                if(isPathRegistered != 0)
                {
                    printf("copying registered endpoint...\n");
                    size_t registeredEndpointLen = strlen(registeredEndpoint) + 1;
                    registeredApiPath = malloc(registeredEndpointLen * sizeof(char));
                    if(registeredApiPath == NULL)
                    {
                        printf("memory allocation failed.\n");
                        exit(1);
                    }
                    strcpy(registeredApiPath, registeredEndpoint);
                }

                free(registeredEndpoint);
                break;
            }
            
            free(registeredEndpoint);
        }

        if(isPathRegistered == 0)
        {
            const char *notFound = "HTTP/1.1 404 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 13\r\n"
                        "\r\n"
                        "404 not found";
            size_t notFoundLen = strlen(notFound) + 1;
            response = malloc(notFoundLen * sizeof(char));
            if(!response)
            {
                printf("Memory allocation failed\n");
                exit(1);
            }else
            {
                strcpy(response, notFound);
            }
        }else
        {
            if(registeredApiPath == NULL)
            {
                printf("Memory Allocation Failed.\n");
                exit(1);
            }
            char *responseData = responseManager(path, method, registeredApiPath);
            free(registeredApiPath); 
            if(responseData)
            {
                const char *header = "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: application/json\r\n"
                                 "Content-Length: %zu\r\n"
                                 "\r\n";
                size_t contentLength = strlen(responseData);
                size_t totalLength = strlen(header) + contentLength + 20;//additional space for Content-Length
                response = malloc(totalLength * sizeof(char));
                if(response)
                {
                    snprintf(response, totalLength, header, contentLength);
                    strcat(response, responseData);
                }
                free(responseData);
            }
        }
        
        send(new_socket, response, strlen(response), 0);
        close(new_socket);
        
    }

    perror("Accept failed");
    return 0;
}
#pragma endregion
