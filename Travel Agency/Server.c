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

#include "Server.h"

agency a;

/**
 * @brief main
 *      Responsible for all the FIFO setup.
 * @param argc
 * @param argv
 */
void main(int argc, char * argv[])
{   
  command c;
  message m;
  
  int server_fifo_hwnd;
  int keep_alive_hwnd;
  
  pid_t pid1, pid2, sid;
  
  
  //Register signals
  signal(SIGINT, shutdown_server);
  signal(SIGUSR1, shutdown_server);
  signal(SIGUSR2, shutdown_server); 
  signal(SIGTSTP, shutdown_server); 
  signal(SIGTERM, shutdown_server); 
  
  
  //Test server FIFO for multiple instances
  printf("A iniciar servidor...\n");
  if(access(SERVER_FIFO, R_OK) != 0)
  {
      printf("Servidor %d iniciado.\n\n", getpid());
  }
  else
  {
      printf("Já existe um servidor em execucao.\n");
      printf("Servidor %d terminado.\n", getpid());
      exit(EXIT_FAILURE);
  }
  
  
  //Create server FIFO
  printf("A criar FIFO 'Agency'...\n");
  if(mkfifo(SERVER_FIFO, 0777) == 0)	
  {
    printf("FIFO 'Agency' criado.\n\n");
  }
  else
  {
    printf("Erro ao criar FIFO 'Agency'.\n");
    printf("Servidor %d terminado.\n", getpid());
    exit(EXIT_FAILURE);
  }
  
  
  //Open server FIFO for writing, non blocking
  printf("A abrir FIFO 'Agency' para leitura, nao bloqueante...\n");
  if((server_fifo_hwnd = open(SERVER_FIFO, O_RDONLY | O_NONBLOCK)) > 0)
  {
    printf("FIFO 'Agency' aberto para leitura.\n\n");
  }
  else
  {
    printf("Erro ao abrir o FIFO 'Agency'.\n");
    printf("Servidor %d terminado.\n", getpid());
    unlink(SERVER_FIFO);
    exit(EXIT_FAILURE);
  }
  
  
  //Changes server FIFO for blocking type.
  printf("A alterar FIFO 'Agency' para bloqueante...\n");
  if(fcntl(server_fifo_hwnd, F_SETFL, O_RDONLY) != -1)
  {
    printf("FIFO 'Agency' alterado para bloqueante.\n\n");
  }
  else
  {
    printf("Erro ao alterar o FIFO 'Agency' para bloqueante.\n");
    printf("Servidor %d terminado.\n", getpid());
    unlink(SERVER_FIFO);
    exit(EXIT_FAILURE);
  }
  
  
  //Open server FIFO for writing so it keeps alive
  printf("A abrir FIFO 'Agency' para escrita, keep alive...\n");
  if ((keep_alive_hwnd = open(SERVER_FIFO, O_WRONLY)) > 0) 
  {
    printf("FIFO 'Agency' aberto para escrita.\n\n");
  }
  else
  {
    printf("Erro ao abrir o fifo 'Agency'.\n");
    printf("Servidor %d terminado.\n", getpid());
    unlink(SERVER_FIFO);
    exit(EXIT_FAILURE);
  }
  
  
  //Load environment variables
  printf("A carregar variaveis de ambiente...\n");
  if(get_syspath(a.database) && get_sysdate(&a.day))
  {
      printf("Variaveis database (%s) e data (%d) carregadas.\n\n", a.database, a.day);
  }
  else
  {
      printf("Erro ao obter as variaveis de ambiente.\n");
      printf("Servidor %d terminado.\n", getpid());
      unlink(SERVER_FIFO);
      exit(EXIT_FAILURE);
  }
  
  
  //Loads agency data
  printf("A carregar dados...\n");
  if(load_agency(&a, get_file(a.database, "rb"), get_file(AGENTS_FILE, "rb"), get_file(ADMIN_FILE, "r")))
  {
      printf("Dados carregados.\n\n");
      change_past(&a, a.day, get_file(PAST_FILE, "a"));
  }
  else
  {
      printf("Erro ao carregar os dados do sistema.\n");
      printf("Servidor %d terminado.\n", getpid());
      unlink(SERVER_FIFO);
      exit(EXIT_FAILURE);
  }
  
  
  /*
    Fork a child process so the parent can exit.  This returns control to
    the command-line or shell.  It also guarantees that the child will not
    be a process group leader, since the child receives a new process ID
    and inherits the parent's process group ID.  This step is required
    to insure that the next call to setsid() is successful.
  */  
  printf("A colocar servidor em background...\n");
  if ((pid1 = fork()) < 0)
  {
    exit(EXIT_FAILURE);
  }
  
  /*
    To become the session leader of this new session and the process group
    leader of the new process group, we call setsid().  The process is
    also guaranteed not to have a controlling terminal.
  */
  if ((sid = setsid()) < 0) 
  {
    exit(EXIT_FAILURE);
  }
  
  /*
    Fork a second child and exit immediately to prevent zombies.  This
    causes the second child process to be orphaned, making the init
    process responsible for its cleanup.  And, since the first child is
    a session leader without a controlling terminal, it's possible for
    it to acquire one by opening a terminal in the future (System V-
    based systems).  This second fork guarantees that the child is no
    longer a session leader, preventing the daemon from ever acquiring
    a controlling terminal.
  */
  if ((pid2 = fork()) > 0)
  {
    exit(EXIT_SUCCESS);
  }
  else
  {
    printf("\n\nO servidor esta agora em background.\n\n");
  }
    
  
  int user_id = 0;
  int permission = 0;
  char cmd[MAX_COMMAND][MAX_COMMAND];
  
  while(1)
  {   
    //Reads the FIFO for new commands
    printf("Le comando no FIFO do Servidor...\n");
    if(read(server_fifo_hwnd, &c, sizeof(c)) > 0)
    {
      printf("Comando recebido:\n");
      printf("Cliente PID: %d\n", c.pid_client);
      printf("Utilizador: %s\n", c.current_user.username);
      printf("Password: %s\n", c.current_user.password);
      printf("Comando: %s\n", c.cmd);
    }

    //Authentication

    clear_message(&m);
    split_command(cmd, c.cmd);
    permission = check_user(&user_id, c.current_user.username, c.current_user.password, &a);
    if(permission == 0)
    {
      m.error = 1;
      sprintf(m.msg, "Falha na autenticacao.");
    }
    else if(permission == 1)
    {

    //Terminal commands
	
	if(strcasecmp(cmd[0], "login") == 0)
	{
	  do_login(&a.users[user_id], &c, &m);
	}
	
	else if(strcasecmp(cmd[0], "exit") == 0)
	{
	  do_exit(&a.users[user_id], &m);
	}
	
	else if(strcasecmp(cmd[0], "logout") == 0)
	{
	  do_logout(&a.users[user_id], &m);
	}
	
	else if(strcasecmp(cmd[0], "mudapass") == 0)
	{
	  if(strcmp(cmd[1], "") != 0)
	  {
	    if(strcmp(cmd[2], "") != 0)
	    {
          do_changepass(&a.users[user_id], &m, cmd[1], cmd[2]);
	    }
	    else
	    {
		  m.error = 1;
		  sprintf(m.msg, "Parametro com a password nova em falta.");
	    }
	  }
	  else
	  {
	    	m.error = 1;
		sprintf(m.msg, "Parametro com o password antiga em falta.");
	  }
	}
	
	else if(strcasecmp(cmd[0], "lista") == 0)
	{
      do_getinfo_user(&a, &m);
	}
	
	else if(strcasecmp(cmd[0], "pesquisa") == 0)
	{
	  if(strcmp(cmd[1], "") != 0)
	  {
	    if(strcmp(cmd[2], "") != 0)
	    {
          do_search(&a, &m, cmd[1], cmd[2]);
	    }
	    else
	    {
		  m.error = 1;
		  sprintf(m.msg, "Parametro com o destino esta em falta.");
	    }
	  }
	  else
	  {
	    	m.error = 1;
		sprintf(m.msg, "Parametro com a origem esta em falta.");
	  }
	}
	
	else if(strcasecmp(cmd[0], "marca") == 0)
	{	  
	  if(strcmp(cmd[1], "") != 0)
	  {
	    if(strcmp(cmd[2], "") != 0)
	    {
          do_schedule(&a, &m, cmd[1], cmd[2]);
	    }
	    else
	    {
		  m.error = 1;
		  sprintf(m.msg, "Parametro com o passaporte em falta.");
	    }
	  }
	  else
	  {
	    	m.error = 1;
		sprintf(m.msg, "Parametro com o ID em falta.");
	  }
	}
	
	else if(strcasecmp(cmd[0], "desmarca") == 0)
	{
	  if(strcmp(cmd[1], "") != 0)
	  {
	    if(strcmp(cmd[2], "") != 0)
	    {
          do_unschedule(&a, &m, cmd[1], cmd[2]);
	    }
	    else
	    {
		  m.error = 1;
		  sprintf(m.msg, "Parametro com o passaporte em falta.");
	    }
	  }
	  else
	  {
	    	m.error = 1;
		sprintf(m.msg, "Parametro com o ID em falta.");
	  }
	}
	
	else
	{
	  m.error = 1;
	  sprintf(m.msg, "Comando nao reconhecido.");
	}
    }
    else if(permission == 2)
    {

    //Admin commands
		
	if(strcasecmp(cmd[0], "login") == 0)
	{  
	  do_login(&a.users[user_id], &c, &m);
	}
	
	else if(strcasecmp(cmd[0], "shutdown") == 0)
	{
	  do_shutdown(&a);
	}
	
	else if(strcasecmp(cmd[0], "addcity") == 0)
	{	  
	  if(strcmp(cmd[1], "") != 0)
	  {
	      do_addcity(&a, &m, cmd[1]);
	  }
	  else
	  {
	    	m.error = 1;
		sprintf(m.msg, "Parametro com o nome da cidade em falta.");
	  }
	}
	
	else if(strcasecmp(cmd[0], "delcity") == 0)
	{	  
	  if(strcmp(cmd[1], "") != 0)
	  {
	      do_delcity(&a, &m, cmd[1]);
	  }
	  else
	  {
	    	m.error = 1;
		sprintf(m.msg, "Parametro com o nome da cidade em falta.");
	  }
	}
	
	else if(strcasecmp(cmd[0], "seepast") == 0)
	{
	  do_seepast(get_file(PAST_FILE, "r"), &m);
	}
	
	else if(strcasecmp(cmd[0], "addvoo") == 0)
	{
	  if(strcmp(cmd[1], "") != 0)
	  {
	    if(strcmp(cmd[2], "") != 0)
	    {
	      if(strcmp(cmd[3], "") != 0)
	      {
		  if(strcmp(cmd[4], "") != 0)
		  {
              do_addflight(&a, &m, cmd[1], cmd[2], cmd[3], cmd[4]);
		  }
		  else
		  {
			m.error = 1;
			sprintf(m.msg, "Parametro com o dia em falta.");
		  }
	      }
	      else
	      {
		    m.error = 1;
		    sprintf(m.msg, "Parametro com o destino em falta.");
	      }
	    }
	    else
	    {
		  m.error = 1;
		  sprintf(m.msg, "Parametro com a origem em falta.");
	    }
	  }
	  else
	  {
		m.error = 1;
		sprintf(m.msg, "Parametro do ID em falta.");
	  }  
	}
	
	else if(strcasecmp(cmd[0], "cancel") == 0)
	{
	  if(strcmp(cmd[1], "") != 0)
	  {
        do_cancelflight(&a, &m, cmd[1]);
	  }
	  else
	  {
		m.error = 1;
		sprintf(m.msg, "Parametro do ID em falta.");
	  }
	}
	
	else if(strcasecmp(cmd[0], "mudadata") == 0)
	{
	  if(strcmp(cmd[1], "") != 0)
	  {
        do_changedate(&a, &m, cmd[1]);
	  }
	  else
	  {
		m.error = 1;
		sprintf(m.msg, "Parametro do dia em falta.");
	  }	  
	}
	
	else if(strcasecmp(cmd[0], "getdata") == 0)
	{
      do_getdate(&m, a.day);
	}
	
	else if(strcasecmp(cmd[0], "info") == 0)
	{
      do_getinfo_admin(&a, &m);
	}
	
	else if(strcasecmp(cmd[0], "adduser") == 0)
	{
	  if(strcmp(cmd[1], "") != 0)
	  {
	    if(strcmp(cmd[2], "") != 0)
	    {
	      do_adduser(&a, &m, cmd[1], cmd[2]);
	    }
	    else
	    {
		  m.error = 1;
		  sprintf(m.msg, "Parametro com a password em falta.");
	    }
	  }
	  else
	  {
	    	m.error = 1;
		sprintf(m.msg, "Parametro com o nome do utilizador em falta.");
	  }
	}
	
	else if(strcasecmp(cmd[0], "exit") == 0)
	{
	  do_exit(&a.users[user_id], &m);
	}
	
	else if(strcasecmp(cmd[0], "logout") == 0)
	{
	  do_logout(&a.users[user_id], &m);
	}
	
	else
	{
	  m.error = 1;
	  sprintf(m.msg, "Comando nao reconhecido.");
	}
    }
    
    if(strcmp(m.msg, "") != 0)
    {
      printf("\nMensagem para o cliente:\n%s\n", m.msg);
    }
    
    send_message(m, c.pid_client);
    
  }

} 

//**************************************** Functionalities ****************************************

/**
 * @brief do_login
 *      Login a user, if not already connected.
 * @param u
 *      User for authentication
 * @param c
 *      The command for the client pid.
 * @param m
 *      The message that is sent to the client.
 */
void do_login(user_extended *u, command *c, message *m)
{
    if(u->connected == 0)
    {
	u->connected = 1;
	u->pid_client = c->pid_client;
      
	m->error = 0;
	sprintf(m->msg, "Autenticado com sucesso.");
	printf("\nUtilizador '%s' conectado\n", c->current_user.username);
    }
    else
    {	   
      if(u->pid_client != c->pid_client)
      {
	  printf("Conta duplicada detectada. A enviar sinal para terminar...\n");
	  if(kill(c->pid_client, SIGUSR1) == 0)
	  {
	    printf("\nSinal SIGUSR2 enviado.\n");
	  }
	  else
	  {
	    printf("\nErro ao enviar o sinal SIGUSR2.\n");
	  }
      }
      else
      {
	  m->error = 1;
	  sprintf(m->msg, "Ja se encontra autenticado.");
      }
    }
}

/**
 * @brief do_exit
 *      Disconnects a user, if already connected.
 * @param u
 *      The user for disconnection.
 * @param m
 *      The message that is sent to the client.
 */
void do_exit(user_extended *u, message *m)
{
  if(u->connected == 1)
  {
    u->connected = 0;
    
    kill(u->pid_client, SIGUSR1);
    printf("\nUtilizador '%s' desconectado\n", u->current_user.username);
  }
  else
  {
    m->error = 1;
    sprintf(m->msg, "Nao se encontra conectado.");
  }
}

/**
 * @brief do_addcity
 *      Add a city to the system to allow new flights to be added from and to this city.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param city
 *      City to be added.
 */
void do_addcity(agency *a, message *m, char *city)
{
    if(a->citys_size < MAX_CITYS)
    {
      if(!check_city(a, city))
      {
	strcpy(a->citys[a->citys_size].city, city);
	a->citys_size++;
      
      	m->error = 0;
	sprintf(m->msg, "Cidade '%s' adicionada.", city);
      }
      else
      {
	  m->error = 1;
	  sprintf(m->msg, "A cidade '%s' ja se encontra na lista.", city);
      }
    }
    else
    {
	m->error = 1;
	sprintf(m->msg, "Limite de cidades atingido!");
    }
}

/**
 * @brief do_delcity
 *      Removes a city from the system, if there are no more flights.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param city
 *      City to be deleted.
 */
void do_delcity(agency *a, message *m, char *city)
{  
  int i = 0;
  
  for(i = 0; i < a->citys_size; i++)
  {
    if(strcasecmp(a->citys[i].city, city) == 0)
    {
      if(!check_city_used, city)
      {
	if(i != a->citys_size - 1)
	{
	  strcpy(a->citys[i].city, a->citys[a->citys_size - 1].city);
	}
	
	a->citys_size--;
	
	m->error = 0;
	sprintf(m->msg, "Cidade '%s' removida.", city);
	break;
      }
      else
      {
	m->error = 1;
	sprintf(m->msg, "Cidade '%s' está em uso!", city);
	break;
      }
    }
  }
  
  if(strcmp(m->msg, "") == 0)
  {
      m->error = 1;
      sprintf(m->msg, "Cidade '%s' nao encontrada.", city);
  }
  
}

/**
 * @brief do_shutdown
 *      Shuts down the whole system.
 * @param a
 *      The agency data structure.
 */
void do_shutdown(agency *a)
{  
  printf("\nA parar todo o sistema!\n");
  kill(getpid(), SIGUSR1);
}

/**
 * @brief do_adduser
 *      Add a new agent to the system.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param user
 *      The username.
 * @param pass
 *      The password.
 */
void do_adduser(agency *a, message *m, char *user, char *pass)
{
    if(a->users_size < MAX_USERS)
    {
      if(check_user(NULL, user, pass, a) == 0)
      {
	strcpy(a->users[a->users_size].current_user.username, user);
	strcpy(a->users[a->users_size].current_user.password, pass);
	a->users_size++;
      
      	m->error = 0;
	sprintf(m->msg, "Utilizador '%s' adicionado.", user);
      }
      else
      {
	m->error = 1;
	sprintf(m->msg, "Esse nome do utilizador ja esta em uso.");
      }
    }
    else
    {
	m->error = 1;
	sprintf(m->msg, "Limite de utilizadores atingido!");
    }
}

/**
 * @brief do_changepass
 *      Changes the password for a specific user.
 * @param u
 *      The user.
 * @param m
 *      The message that is sent to the client.
 * @param old_pass
 *      The old password for verification.
 * @param new_pass
 *      The new password.
 */
void do_changepass(user_extended *u, message *m, char *old_pass, char *new_pass)
{
    if(strcmp(u->current_user.password, old_pass) == 0)
    {
	strcpy(u->current_user.password, new_pass);
      
      	do_exit(u, m);
    }
    else
    {
	m->error = 1;
	sprintf(m->msg, "Password antiga errada.");
    }
}

/**
 * @brief do_getdate
 *      Gets the system date. (just builds the message)
 * @param m
 *      The message that is sent to the client.
 * @param day
 *      The day to be put on the message.
 */
void do_getdate(message *m, int day)
{
    m->error = 0;
    sprintf(m->msg, "O dia atual e: %d", day);
}

/**
 * @brief do_getinfo_admin
 *      Lists information on the system for the admin analyse.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 */
void do_getinfo_admin(agency *a, message *m)
{
  int i = 0;
  
  m->error = 0;
  
  if(a->users_size > 0)
  {
    sprintf(&m->msg[strlen(m->msg)], "Utilizadores:\n");
    sprintf(&m->msg[strlen(m->msg)], "\nUser\tPass\tConn\tPerm\n");
    
    for(i = 0; i < a->users_size; i++)
    {
      sprintf(&m->msg[strlen(m->msg)], "%s\t", a->users[i].current_user.username);    
      sprintf(&m->msg[strlen(m->msg)], "%s\t", a->users[i].current_user.password);   
      sprintf(&m->msg[strlen(m->msg)], "%d\t", a->users[i].connected);  
      sprintf(&m->msg[strlen(m->msg)], "%d\n", a->users[i].permission);   
    }
  }
  
  if(a->flights_size > 0)
  {
    sprintf(&m->msg[strlen(m->msg)], "\nVoos:\n");
    sprintf(&m->msg[strlen(m->msg)], "\nID\tLugares\tL Disp\tOrigem\tDestino\tDia\n");
    
    for (i = 0; i < a->flights_size; i++)
    {
      sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].id_flight); 
      sprintf(&m->msg[strlen(m->msg)], "%d\t", a->flights[i].seats);
      sprintf(&m->msg[strlen(m->msg)], "%d\t", (a->flights[i].seats - a->flights[i].seats_taken)); 
      sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].origin); 
      sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].destination); 
      sprintf(&m->msg[strlen(m->msg)], "%d\n", a->flights[i].day); 
    } 
  } 
  
  if(a->citys_size > 0)
  {
    sprintf(&m->msg[strlen(m->msg)], "\nCidades:\n\n");
    
    for (i = 0; i < a->citys_size; i++)
    {
      sprintf(&m->msg[strlen(m->msg)], "%s\n", a->citys[i].city); 
    } 
  }
     
}

/**
 * @brief do_addflight
 *      Adds a flight to the system
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param id_flight
 *      ID of the flight, cannot be repeted.
 * @param origin
 *      The origin of the flight
 * @param destination
 *      The destination of the flight.
 * @param day
 *      The day of the flight.
 */
void do_addflight(agency *a, message *m, char *id_flight, char *origin, char *destination, char *day)
{
  //It is possible to schedule flights with the same origin and destiny, the ID can be diferent.

  int date = 0;
  
  if(a->flights_size < MAX_FLIGHTS)
  {
    if(check_city(a, origin) && check_city(a, destination))
    { 
      if(check_flight_id(a, id_flight) == 0)
      {
	  date = (int)strtol(day, NULL, 0);
	  if(date != 0)
	  {
	    flight x;
	  
	    strcpy(x.id_flight, id_flight);
	    strcpy(x.origin, origin);
	    strcpy(x.destination, destination);
	    
	    x.seats = MIN_SEATS;
	    x.seats_taken = 0;     
	    x.day = date;  
	    
	    a->flights[a->flights_size] = x;
	    a->flights_size++;	  

	    m->error = 0;
	    sprintf(m->msg, "Voo %s -> %s adicionado.", origin, destination);
	  }
	  else
	  {
	    m->error = 1;
	    sprintf(m->msg, "Dia com formato invalido.");
	  }
	}
	else
	{
	  m->error = 1;
	  sprintf(m->msg, "Esse ID ja esta em uso.");
	}
     }
     else
     {
	m->error = 1;
	sprintf(m->msg, "Cidade nao encontrada.");
     }
  }
  else
  {
      m->error = 1;
      sprintf(m->msg, "Limite de voos atingido!");
  }
}

/**
 * @brief do_getinfo_user
 *      Lists information on the system for the user.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 */
void do_getinfo_user(agency *a, message *m)
{
  int i = 0;
  m->error = 0;
  
  if(a->flights_size > 0)
  {
      sprintf(&m->msg[strlen(m->msg)], "Voos:\n");
      sprintf(&m->msg[strlen(m->msg)], "\nID\tOrigem\tDestino\tVagas\n");
  
      for (i = 0; i < a->flights_size; i++)
      {
	if(a->flights[i].day >= a->day)
	{ 
	  sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].id_flight); 
	  sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].origin); 
	  sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].destination); 
	  sprintf(&m->msg[strlen(m->msg)], "%d\n", (a->flights[i].seats - a->flights[i].seats_taken));
	}
      }
  }
  else
  {
      sprintf(&m->msg[strlen(m->msg)], "Não existem voos disponiveis.");
  }
}

/**
 * @brief do_pesquisa
 *      Search for a flight.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param origin
 *      Origin of the flight.
 * @param destination
 *      Destination of the flight.
 */
void do_search(agency *a, message *m, char *origin, char *destination)
{
  int i = 0;
  m->error = 0;
  
  if(a->flights_size > 0)
  {
      sprintf(&m->msg[strlen(m->msg)], "Voos:\n");
      sprintf(&m->msg[strlen(m->msg)], "\nID\tOrigem\tDestino\tVagas\n");
  
      for (i = 0; i < a->flights_size; i++)
      {
	if ((a->flights[i].day >= a->day) && (strcasecmp(a->flights[i].origin, origin) == 0) && (strcasecmp(a->flights[i].destination, destination) == 0))
	{ 
	  sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].id_flight); 
	  sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].origin); 
	  sprintf(&m->msg[strlen(m->msg)], "%s\t", a->flights[i].destination); 
	  sprintf(&m->msg[strlen(m->msg)], "%d\n", (a->flights[i].seats - a->flights[i].seats_taken));
	}
      }
  }
  else
  {
	sprintf(&m->msg[strlen(m->msg)], "Não existem voos disponiveis.");
  }
}

/**
 * @brief do_logout
 *      Disconnects a specific user.
 * @param u
 *      The user.
 * @param m
 *      The message that is sent to the client.
 */
void do_logout(user_extended *u, message *m)
{
  if(u->connected == 1)
  {
    u->connected = 0;
    
    m->error = 0;
    sprintf(m->msg, "Desconectado.");
    printf("\nUtilizador '%s' desconectado\n", u->current_user.username);
  }
  else
  {
    m->error = 1;
    sprintf(m->msg, "Nao se encontra conectado.");
  }
}

/**
 * @brief do_schedule
 *      Schedules a new flight.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param id_flight
 *      The ID of the flight.
 * @param passport
 *      The passport of the client.
 */
void do_schedule(agency *a, message *m, char *id_flight, char *passport)
{
  int flight_index;
  if((flight_index = check_flight_id(a, id_flight)) != 0)
  {
    flight_index--;
    if(a->flights[flight_index].seats_taken < a->flights[flight_index].seats)
    {
      if(check_passaport(a, flight_index, passport) == 0)
      {
	strcpy(a->flights[flight_index].passengers[a->flights[flight_index].seats_taken].passport, passport);
	a->flights[flight_index].seats_taken++;
	
	m->error = 0;
	sprintf(m->msg, "O voo do passageiro %s foi marcado.", passport);
      }
      else
      {
	m->error = 1;
	sprintf(m->msg, "Ja se encontra neste voo.");
      }
    }
    else
    {
      m->error = 1;
      sprintf(m->msg, "O voo '%s' ja esta cheio.", id_flight);
    }
  }
  else
  {
    m->error = 1;
    sprintf(m->msg, "Voo nao encontrado.");
  }
}

/**
 * @brief do_unschedule
 *      Cancels an already scheduled flight.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param id_flight
 *      The ID of the flight.
 * @param passport
 *      The passport of the client.
 */
void do_unschedule(agency *a, message *m, char *id_flight, char *passport)
{
  int flight_index;
  int passaport_index;
  if((flight_index = check_flight_id(a, id_flight)) != 0)
  {
    flight_index--;
    if((passaport_index = check_passaport(a, flight_index, passport)) != 0)
    {
      passaport_index--;    
      
      if(passaport_index != a->flights[flight_index].seats_taken - 1)
      {
	strcpy(a->flights[flight_index].passengers[passaport_index].passport, 
	       a->flights[flight_index].passengers[a->flights[flight_index].seats_taken - 1].passport);
      }
           
      a->flights[flight_index].seats_taken--;
      
      m->error = 0;
      sprintf(m->msg, "O voo do passageiro %s foi desmarcado.", passport);
    }
    else
    {
      m->error = 1;
      sprintf(m->msg, "Nao se encontra nesse voo.");
    }
  }
  else
  {
    m->error = 1;
    sprintf(m->msg, "Voo nao encontrado.");
  }
}

/**
 * @brief do_cancel
 *      Cancels a flight, with or without clients.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param id_flight
 *      The passport of the client.
 */
void do_cancelflight(agency *a, message *m, char *id_flight)
{
  int flight_index;
  if((flight_index = check_flight_id(a, id_flight)) != 0)
  {
    flight_index--;
    
    if(flight_index != a->flights_size)
    {
      a->flights[flight_index] = a->flights[a->flights_size - 1];
    }
    
    a->flights_size--;
    
    m->error = 0;
    sprintf(m->msg, "O voo %s foi cancelado.", id_flight);
  }    
  else
  {
    m->error = 1;
    sprintf(m->msg, "Voo nao encontrado.");
  }
}

/**
 * @brief do_seepast
 *      Displays the past sucessfully concluded flights.
 * @param past
 *      The file that stores the data.
 * @param m
 *      The message that is sent to the client.
 */
void do_seepast(FILE *past, message *m)
{
  int count = 0;
  char line[MAX_FGETS];
  m->error = 0;
  
  if(past != NULL)
  {   
    while(fgets(line, MAX_FGETS, past) != NULL)
    {
      sprintf(&m->msg[strlen(m->msg)], "%s", line);
      count++;
    }
  }
  
  if(count == 0)
  {
    sprintf(m->msg, "Nao existem voos a apresentar.");
  }
}

/**
 * @brief do_changedate
 *      Changes the date of the system.
 * @param a
 *      The agency data structure.
 * @param m
 *      The message that is sent to the client.
 * @param day
 *      New day.
 */
void do_changedate(agency *a, message *m, char *day)
{ 
  int date = 0;
  int count = 0;
  date = (int)strtol(day, NULL, 0);
  
  if(date != 0)
  {
    if(date > a->day)
    {   
      a->day = date;
      count = change_past(a, date, get_file(PAST_FILE, "a"));
      if(count != 0)
      {
	m->error = 0;
	sprintf(m->msg, "Foram colocados no passado %d voos", count);
      }
      else
      {
	m->error = 0;
	sprintf(m->msg, "Nao existem voos no passado.");
      }  
    }
    else
    {
      m->error = 1;
      sprintf(m->msg, "Dia tem que ser superior a %d", a->day);
    }
  }
  else
  {
    m->error = 1;
    sprintf(m->msg, "Dia com formato invalido.");
  }
}

/**
 * @brief change_past
 *      Helper of the above function, flights in the past will be pushed to the file.
 * @param a
 *      The agency data structure.
 * @param day
 *      Current day.
 * @param past
 *      The file that stores the data.
 * @return
 *      Number of pushed flights.
 */
int change_past(agency *a, int day, FILE *past)
{
 int i = 0;
 int k = 0;
 int j = 0;
 
 int count = 0;
 
 flight x;
 boolean flag = FALSE;
 
 if(a->flights_size > 0)
 {
    for(i = 0; i < a->flights_size; i++)
    {
      if(a->flights[i].day < day)
      {
	for(k = a->flights_size - 1; k > i ; k--)
	{
	  if(a->flights[k].day >= day)
	  {
	    x = a->flights[i];
	    a->flights[i] = a->flights[k]; //Troca
	    a->flights[k] = x;
	    
	    flag = TRUE;
	    break;
	  }
	}
	
	if(flag == FALSE)
	{
	  count = a->flights_size - i; 
	  
	  //Passa para ficheiro
	  for(j = i; j < a->flights_size; j++)
	  {
		fprintf(past, "\nVoo %s de %s para %s com %d pessoas ocorreu no dia %d.\n",  
		a->flights[j].id_flight, a->flights[j].origin, a->flights[j].destination,
		a->flights[j].seats_taken, a->flights[j].day);
	  }
	  fclose(past);
	  
	  a->flights_size = i; //Remove
	  break;
	}
	
	flag = FALSE;
      }
    }
 }
 
 return count;
}


//**************************************** Send data ****************************************

/**
 * @brief send_message
 *      Sends the result of the above functions to the client.
 * @param m
 *      The message that is sent to the client.
 * @param pid_client
 *      The client to send the message to.
 */
void send_message(message m, pid_t pid_client)
{
    int client_fifo_hwnd;
    char client_fifo_name[FIFONAME_SIZE];
    
    //***** Abre FIFO do Cliente *****
    printf("\nA abrir FIFO do cliente para escrita...\n");
    sprintf(client_fifo_name, CLIENT_FIFO, pid_client);
    if((client_fifo_hwnd = open(client_fifo_name, O_WRONLY | O_NONBLOCK)) > 0)
    {
      printf("O FIFO do cliente foi aberto para escrita.\n\n");
    }
    else
    {
      printf("Erro ao abrir o FIFO do cliente.\n");
    }
    
    //***** Escreve no FIFO do Cliente *****
    printf("A escrever no FIFO do cliente...\n");
    if(write(client_fifo_hwnd, &m, sizeof(m)) > 0)
    {
      printf("Dados enviados para o cliente!\n\n");
    }
    else
    {
      printf("Erro ao escrever no FIFO do cliente.\n");
    }
     
    close(client_fifo_hwnd);
}


//**************************************** Loads, Checks, Signals ****************************************

/**
 * @brief check_user
 *      Check the permission of a user.
 * @param user_id
 *      The user ID.
 * @param username
 *      The username.
 * @param password
 *      The password.
 * @param a
 *      The agency data structure.
 * @return
 *      The permission of the user.
 */
int check_user(int *user_id, char *username, char *password, agency *a)
{
  int i;
  int result = 0;
  
  for(i = 0; i < a->users_size; i++)
  {
    if(strcmp(a->users[i].current_user.username, username) == 0)
    {
      if(strcmp(a->users[i].current_user.password, password) == 0)
      {
	if(a->users[i].permission == 0)
	{
	  result = 1;
	}
	else
	{
	  result = 2;
	}
	
	*user_id = i;	
	break;
      }
    }
  }
  
  return result;
}

/**
 * @brief check_flight_id
 *      Checks IF of a flight.
 * @param a
 *      The agency data structure.
 * @param id_flight
 *      The ID of the flight.
 * @return
 *      The number of flights with id_flight.
 */
int check_flight_id(agency *a, char *id_flight)
{
  int i = 0;
  int result = 0;
  
  if(a->flights_size > 0)
  {
      for (i = 0; i < a->flights_size; i++)
      {
	if (strcasecmp(a->flights[i].id_flight, id_flight) == 0)
	{ 
	  result = i + 1;
	  break;
	}
      }
  }
  
  return result;
}

/**
 * @brief check_passaport
 *      Check the users passport on a flight.
 * @param a
 *      The agency data structure.
 * @param flight_index
 *      The flight to check.
 * @param passport
 *      The passport to be checked.
 * @return
 *      Number of times that the passport was used.
 */
int check_passaport(agency *a, int flight_index, char *passport)
{
  int i = 0;
  int result = 0;
  
  if(a->flights[flight_index].seats_taken > 0)
  {
      for (i = 0; i < a->flights[flight_index].seats_taken; i++)
      {
	if (strcasecmp(a->flights[flight_index].passengers[i].passport, passport) == 0)
	{ 
	  result = i + 1;
	  break;
	}
      }
  }
  
  return result;
}

/**
 * @brief check_city_used
 *      Does the city already exists?
 * @param a
 *      The agency data structure.
 * @param city
 *      City to check.
 * @return
 *      Success or insuccess
 */
boolean check_city(agency *a, char *city)
{
  int i = 0;
  boolean result = FALSE;
  
  for(i = 0; i < a->citys_size; i++)
  {
    if(strcasecmp(a->citys[i].city, city) == 0)
    {
      result = TRUE;
      break;
    }
  }
  
  return result;
}

/**
 * @brief check_city_used
 *      Does the city exists and has flights to or from it?
 * @param a
 *      The agency data structure.
 * @param city
 *      City to check.
 * @return
 *      Success or insuccess
 */
boolean check_city_used(agency *a, char *city)
{
  int i = 0;
  boolean result = FALSE;
  
  for(i = 0; i < a->flights_size; i++)
  {
    if((strcasecmp(a->flights[i].origin, city) == 0) || (strcasecmp(a->flights[i].destination, city) == 0))
    {
      result = TRUE;
      break;
    }
  }
  
  return result;
}

/**
 * @brief load_agency
 *      Loads all the information from permanent storage.
 * @param a
 *      The agency data structure.
 * @param flights
 *      The flights file to save the information.
 * @param users
 *      The users file to save the information.
 * @param admin
 *      The admin file to save the information.
 * @return
 *      Success or insuccess
 */
boolean load_agency(agency *a, FILE *flights, FILE *users, FILE *admin)
{
  int i = 0;
  int k = 0;
  boolean result;
  char line[MAX_FGETS];
  
  if(a != NULL && flights != NULL && users != NULL)
  {    
    //Load cities
    fread(&a->citys_size, sizeof(a->citys_size), 1, flights); 
    for(i = 0; i < a->citys_size; i++)
    {
      fread(&a->citys[i].city, sizeof(a->citys[i].city), 1, flights);
    }
    
    //Load flights
    fread(&a->flights_size, sizeof(a->flights_size), 1, flights);   
    for(i = 0; i < a->flights_size; i++)
    {
	fread(&a->flights[i].id_flight, sizeof(a->flights[i].id_flight), 1, flights);
	fread(&a->flights[i].seats, sizeof(a->flights[i].seats), 1, flights);
	fread(&a->flights[i].seats_taken, sizeof(a->flights[i].seats_taken), 1, flights);
	fread(&a->flights[i].origin, sizeof(a->flights[i].origin), 1, flights);
	fread(&a->flights[i].destination, sizeof(a->flights[i].destination), 1, flights);
	fread(&a->flights[i].day, sizeof(a->flights[i].day), 1, flights);
	
	for(k = 0; k < a->flights[i].seats_taken; k++)
	{
	  fread(&a->flights[i].passengers[k].passport, sizeof(a->flights[i].passengers[k].passport), 1, flights);
	} 
    }
    
    //Load users
    fread(&a->users_size, sizeof(a->users_size), 1, users); 
    for(i = 0; i < a->users_size; i++)
    {
      fread(&a->users[i].current_user.username, sizeof(a->users[i].current_user.username), 1, users);
      fread(&a->users[i].current_user.password, sizeof(a->users[i].current_user.password), 1, users);
      a->users[i].permission = 0;
      a->users[i].connected = 0;
      a->users[i].pid_client = 0;
    }
    
    result = TRUE;
    
    //Load admin
    if(fscanf(admin, "%s", line) > 0)
    {
      strcpy(a->users[a->users_size].current_user.username, "admin");
      strcpy(a->users[a->users_size].current_user.password, line);
      a->users[a->users_size].permission = 1;
      a->users[a->users_size].connected = 0;
      a->users[a->users_size].pid_client = 0;
      
      a->users_size++;
    }
    else
    {
      result = FALSE;
    }
    
    fclose(flights);
    fclose(users);
    fclose(admin);
    
  }
  else
  {
    result = FALSE;
  }
  
  return result;
}

/**
 * @brief save_agency
 *      Save all the data on the system to permanent storage.
 * @param a
 *      The agency data structure.
 * @param flights
 *      The flights file to save the information.
 * @param users
 *      The users file to save the information.
 * @return
 *      Success or insuccess
 */
boolean save_agency(agency *a, FILE *flights, FILE *users)
{
  int i = 0;
  int k = 0;
  boolean result;
  
  if(a != NULL && flights != NULL && users != NULL)
  {    
    //Save citys
    fwrite(&a->citys_size, sizeof(a->citys_size), 1, flights);    
    for(i = 0; i < a->citys_size; i++)
    {
      fwrite(&a->citys[i].city, sizeof(a->citys[i].city), 1, flights);
    }
    
    //Save flights
    fwrite(&a->flights_size, sizeof(a->flights_size), 1, flights);   
    for(i = 0; i < a->flights_size; i++)
    {
	fwrite(&a->flights[i].id_flight, sizeof(a->flights[i].id_flight), 1, flights);
	fwrite(&a->flights[i].seats, sizeof(a->flights[i].seats), 1, flights);
	fwrite(&a->flights[i].seats_taken, sizeof(a->flights[i].seats_taken), 1, flights);
	fwrite(&a->flights[i].origin, sizeof(a->flights[i].origin), 1, flights);
	fwrite(&a->flights[i].destination, sizeof(a->flights[i].destination), 1, flights);
	fwrite(&a->flights[i].day, sizeof(a->flights[i].day), 1, flights);
	
	for(k = 0; k < a->flights[i].seats_taken; k++)
	{
	  fwrite(&a->flights[i].passengers[k].passport, sizeof(a->flights[i].passengers[k].passport), 1, flights);
	} 
    }
    
    //Save users
    a->users_size--;
    fwrite(&a->users_size, sizeof(a->users_size), 1, users);
    a->users_size++;
    
    for(i = 0; i < a->users_size; i++)
    {
      if(a->users[i].permission == 0)
      {
	fwrite(&a->users[i].current_user.username, sizeof(a->users[i].current_user.username), 1, users);
	fwrite(&a->users[i].current_user.password, sizeof(a->users[i].current_user.password), 1, users);
      }
    }
    
    fclose(flights);
    fclose(users);
    
    result = TRUE;
  }
  else
  {
    result = FALSE;
  }
  
  return result;
}

/**
 * @brief kick_users
 *      Kicks all the currently active useres from the system.
 * @param a
 *      The agency data structure.
 */
void kick_users(agency *a)
{
  int i = 0;
  
  for(i = 0; i < a->users_size; i++)
  {
    if(a->users[i].connected == 1)
    {
      kill(a->users[i].pid_client, SIGUSR1);
    }
  } 
}

/**
 * @brief shutdown_server
 *      Shuts down the whole system.
 * @param signal
 *      The message and operation varies on the signal received.
 */
void shutdown_server(int signal)
{  
  //Terminates all the users
  kick_users(&a);

  //Save current data
  printf("A gravar dados...\n");
  if(save_agency(&a, get_file(a.database, "wb"), get_file(AGENTS_FILE, "wb")))
  {
      printf("Dados gravados.\n\n");
  }
  else
  {
      printf("Erro ao gravar dados.\n");
      printf("Servidor %d terminado.\n", getpid());
      exit(EXIT_FAILURE);
  }
  
  //Deal with received signal
  if(signal == 2 || signal == 20 || signal == 15)
  {
    printf("Paragem a pedido do utilizador.\n");
    printf("Servidor %d terminado.\n", getpid());
    unlink(SERVER_FIFO);   
    exit(EXIT_SUCCESS);
  }
  else if(signal == 10)
  {
    printf("Paragem a pedido do administrador.\n");
    printf("Servidor %d terminado.\n", getpid());
    unlink(SERVER_FIFO);   
    exit(EXIT_SUCCESS);
  }
  else if(signal == 12)
  {
    printf("Paragem por erro!\n");
    printf("Servidor %d terminado.\n", getpid());
    unlink(SERVER_FIFO);   
    exit(EXIT_SUCCESS);
  }
}

/**
 * @brief clear_message
 *      Deletes the current message to be sent to the client.
 * @param m
 *      The current message.
 */
void clear_message(message *m)
{
    m->error = 0;
    sprintf(m->msg, "");
}

/**
 * @brief get_sysdate
 *      Gets the date of the system (not the OS).
 * @param date
 * @return
 */
boolean get_sysdate(int *date)
{
  char *path = NULL;
  path = getenv("SODATA");
  
  if(path != NULL)
  {
    *date = (int) strtol(path, NULL, 0);
    
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

/**
 * @brief get_syspath
 *      Gets the environment variable of the database.
 * @param database
 *      The database to copy the values.
 * @return
 *      Success or insuccess.
 */
boolean get_syspath(char *database)
{
  char *path = NULL;
  path = getenv("SOFICHEIRO");
  
  if(path != NULL)
  {
    strcpy(database, path);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

//**************************************** Gets, Inputs ****************************************

/**
 * @brief split_command
 *      Splits the command from a single string to an array of parameters.
 * @param result
 *      The splitted command.
 * @param string
 *      The command to split.
 */
void split_command(char result[MAX_COMMAND][MAX_COMMAND], char *string)
{
  int count = 0;
  char str[MAX_COMMAND];
  char *token;
  
  strcpy(str, string);
  
  for(count = 0; count < MAX_COMMAND; count++)
  {
    strcpy(result[count], "");
  }
  
  count = 0;
  if(str != NULL) 
  {
    token = strtok(str, " ");
    while(token != NULL)
    {     
      strcpy(result[count], token);
      token = strtok(NULL, " ");
      count++;
    }
  }
}

/**
 * @brief get_split
 *      Directly gets a parameter from the command string.
 *      This function is not in use because the command is
 *      splitted by the above function on arival.
 * @param result
 *      The parameter with the sent number on the command.
 * @param string
 *      The command to split.
 * @param number
 *      The number of the parameter.
 */
void get_split(char *result, char *string, int number)
{  
  int count = 0;
  char str[MAX_COMMAND];
  char *token;
  
  strcpy(str, string);
  
  if(str != NULL) 
  {
    token = strtok(str, " ");
    
    while(token != NULL)
    {     
      if (count == number)
      {
	break;
      }
      
      token = strtok(NULL, " ");
      count++;
    }
    
    if(token == NULL)
    {
      strcpy(result, "");
    }
    else
    {
      strcpy(result, token);
    }    
  }

}

/**
 * @brief get_file
 *      Opens a file, and in case of faliure asks the user on how to proceed.
 *      This is the only time that the server talks with the user. All checks are performed on startup.
 * @param filename
 *      The path of the file.
 * @param mode
 *      The mode on how to open the file.
 * @return
 *      null if it fails, else the file.
 * @remarks
 *      fclose required on the returned pointer.
 */
FILE* get_file(char *filename, char *mode)
{
	int option = 0;

	FILE *database = NULL;
	boolean loop = TRUE;

	char filename_aux[FILENAME_SIZE] = {0};
	strcpy(filename_aux, filename);

	while(loop)
	{
		if ((database = fopen(filename_aux, mode)) != NULL)
		{
			loop = FALSE;		
		}
		else
		{
			if((strcmp(mode, "rb") == 0) || (strcmp(mode, "r") == 0))
			{		
				printf("Ocurreu um erro na leitura do ficheiro: %s\n\n", filename_aux);
				printf("1 - Voltar a tentar ler o ficheiro\n");
				printf("2 - Procurar outro ficheiro manualmente\n");
				printf("3 - Continuar sem ler ficheiro\n\n");
				
			}
			else if((strcmp(mode, "wb") == 0) || (strcmp(mode, "w") == 0) || (strcmp(mode, "a") == 0))
			{
				printf("Ocurreu um erro ao escrever no ficheiro: %s\n\n", filename_aux);
				printf("1 - Voltar a tentar escrever no ficheiro\n");
				printf("2 - Criar um novo ficheiro\n");
				printf("3 - Sair sem guardar os dados\n\n");
			}

			printf("Opcao: ");
			int_input(&option, 2);

			switch(option)
			{
				case 1:
					{
						break;
					}
				case 2:
					{
						get_filename(filename_aux, FILENAME_SIZE);
						break;
					}
				case 3:
					{
						loop = FALSE;
						database = NULL;
						break;
					}
				default:
					{
						printf("Essa opcao nao existe.\n\n");
						break;
					}
			}
		}
	}

	return database;
}


/**
 * @brief get_filename
 *      Asks for a filename to write or read from.
 * @param filename
 *      Variable to write the new name.
 * @param size
 *      Size of the input.
 */
void get_filename(char *filename, int size)
{
	boolean loop = TRUE;

	while(loop)
	{
		printf("Nome do ficheiro: ");
		char_input(filename, size);

		if (strcmp(filename, "") != 0)
		{
			loop = FALSE;
		}
		else
		{
			printf("Escreva um nome para o ficheiro\n\n");
		}
	}
}

/**
 * @brief char_input
 *      Function that deals with char[] input.
 * @param string
 *      [in,out] If non-null, the string.
 * @param size
 *      The size.
 * @remarks
 *      The value returned will always have the size of 'size' -1, the last char is \0.
 *      The max value for 'size' is the same as the given array(string[size]).
 *      Will always fail, if the value of 'size' is 0 or 1.
 *      Returns 0 in case of failure.
 */
void char_input(char *string, int size)
{
	fgets(string, size, stdin);

	string[strcspn(string, "\r\n")] = '\0';

	if(strlen(string) == size - 1)
	{
		while (getchar() != '\n') {}
	}
}

/**
 * @brief int_input
 *      Function that deals with integer input.
 * @param integer
 *      [in,out] If non-null, the integer.
 * @param size
 *      The size.
 * @remarks
 *      The value returned will always have the size of 'size'.
 *      The max value for 'size' is the same as the length of the int range (2,147,483,647 ~ 9)
 *      Will always fail, if the value of 'size' is 0 or 1.
 *      Returns 0 in case of failure.
 */
void int_input(int *integer, int size)
{
	char *string = (char*) malloc(size);

	fgets(string, size, stdin);

	string[strcspn(string, "\r\n")] = '\0';

	if(strlen(string) == size - 1)
	{
		while (getchar() != '\n') {}
	}

	*integer = (int) strtol(string, NULL, 0);

	free(string);
}
