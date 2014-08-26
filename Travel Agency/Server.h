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

#include "Data.h"

void do_shutdown(agency *a);
void do_login(user_extended *u, command *c, message *m);
void do_addcity(agency *a, message *m, char *city);
void do_delcity(agency *a, message *m, char *city);
void do_exit(user_extended *u, message *m);
void do_adduser(agency *a, message *m, char *user, char *pass);
void do_changepass(user_extended *u, message *m, char *old_pass, char *new_pass);
void do_getdate(message *m, int day);
void do_getinfo_admin(agency *a, message *m);
void do_addflight(agency *a, message *m, char *id_flight, char *origin, char *destination, char *day);
void do_getinfo_user(agency *a, message *m);
void do_search(agency *a, message *m, char *origin, char *destination);
void do_logout(user_extended *u, message *m);
void do_schedule(agency *a, message *m, char *id_flight, char *passport);
void do_unschedule(agency *a, message *m, char *id_flight, char *passport);
void do_cancelflight(agency *a, message *m, char *id_flight);
void do_seepast(FILE *past, message *m);
void do_changedate(agency *a, message *m, char *day);

void send_message(message m, pid_t pid_client);
void shutdown_server(int signal);
void kick_users(agency *a);
void clear_message(message *m);
void split_command(char result[MAX_COMMAND][MAX_COMMAND], char *string);
void get_split(char *result, char *string, int number);
int change_past(agency *a, int day, FILE *past);
boolean save_agency(agency *a, FILE *flights, FILE *users);
boolean load_agency(agency *a, FILE *flights, FILE *users, FILE *admin);

int check_flight_id(agency *a, char *id_flight);
int check_passaport(agency *a, int flight_index, char *passport);
int check_user(int *user_id, char *username, char *password, agency *a);
boolean check_city(agency *a, char *city);
boolean check_city_used(agency *a, char *city);

boolean get_syspath(char *database);
boolean get_sysdate(int *date);
FILE* get_file(char *filename, char *mode);
void get_filename(char *filename, int size);

void char_input(char *string, int size);
void int_input(int *integer, int size);
