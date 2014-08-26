/*
 * Travel - Agency
 * Copyright (C) 2013 Filipe Carvalho
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

//Pipes
#define FIFONAME_SIZE 10			//Maximum size for the client pipe
#define SERVER_FIFO "Pipes/Agency"		//Server pipe
#define CLIENT_FIFO "Pipes/Client_%d"		//Client pipe

//Files
#define FILENAME_SIZE 30			//Maximum input size for the name of the file
#define ADMIN_FILE "Files/SOADMPASS"		//Admin password file
#define AGENTS_FILE "Files/SOAGENTES"		//Agents file
#define PAST_FILE "Files/SOPAST"		//Past file

//Structs
#define MAX_CITYS 	30			//Maximum number of citys
#define MAX_USERS 	100			//Maximum number of users
#define MAX_FLIGHTS 	100			//Maximum number of flights
#define MAX_PASSAGERS 	100			//Maximum number of passengers

//Structs variables
#define MAX_FLIGHTID	10			//Maximum size for the flight ID
#define MAX_CITYNAME 	50 			//Maximum size for the city name
#define MAX_PASSPORT 	50			//Maximum size for the passaport
#define MAX_USERNAME 	20			//Maximum size for the username
#define MAX_PASSWORD 	20			//Maximum size for the password
#define MAX_COMMAND 	100			//Maximum size for the command
#define MAX_MESSAGE	1000			//Maximum size for the message

//Other
#define MIN_SEATS 5 				//Minimum number of seats
#define MAX_FGETS 100				//Maximum size for each readed line
#define MAX_DATABASE 100 			//Maximum size for the file path



struct passenger
{
  char passport[MAX_PASSPORT];
};
typedef struct passenger passenger;



struct user
{
  char username[MAX_USERNAME];
  char password[MAX_PASSWORD];
};
typedef struct user user;



struct user_extended
{
  pid_t pid_client;
  user current_user;
  int permission;
  int connected;
};
typedef struct user_extended user_extended;



struct city
{
  char city[MAX_CITYNAME];
};
typedef struct city city;



struct flight
{
  char id_flight[MAX_FLIGHTID];
  int seats;
  int seats_taken;
  char origin[MAX_CITYNAME];
  char destination[MAX_CITYNAME]; 
  int day;
  passenger passengers[MAX_PASSAGERS];
};
typedef struct flight flight;



struct agency
{
  int day;
  char database[MAX_DATABASE];
  
  city citys[MAX_CITYS];
  int citys_size;
  
  flight flights[MAX_FLIGHTS];
  int flights_size;
  
  user_extended users[MAX_USERS];
  int users_size;
};
typedef struct agency agency;



struct command
{
  pid_t pid_client;
  user current_user;
  char cmd[MAX_COMMAND];
};
typedef struct command command;



struct message
{
  int error;
  char msg[MAX_MESSAGE];
};
typedef struct message message;



enum boolean
{
  FALSE,
  TRUE
};
typedef enum boolean boolean;
