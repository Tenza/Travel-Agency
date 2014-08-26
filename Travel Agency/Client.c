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

#include "Client.h"
 
command c;
boolean logged = FALSE;
 
/**
 * @brief main
 *      Responsible for all the FIFO setup.
 * @param argc
 * @param argv
 */
void main(int argc, char * argv[])
{
  message m;
  
  int client_fifo_hwnd;
  int keep_alive_hwnd;  
  char client_fifo_name[FIFONAME_SIZE];
    
  int count = 0;
  char *password;
  
  //Register signals
  signal(SIGINT, shutdown_client);
  signal(SIGUSR1, shutdown_client); 
  signal(SIGUSR2, shutdown_client); 
  signal(SIGTSTP, shutdown_client); 
  signal(SIGTERM, shutdown_client); 
  
  
  //Test server FIFO
  printf("A iniciar cliente...\n");
  if(access(SERVER_FIFO, W_OK) == 0)
  {
      printf("Cliente %d iniciado.\n\n", getpid());
  }
  else
  {   
      printf("O servidor não esta em execucao.\n");
      printf("Cliente %d terminado.\n", getpid());
      exit(EXIT_FAILURE);
  }
  
  
  //Create client FIFO for the reply
  printf("A criar FIFO de resposta...\n");
  sprintf(client_fifo_name, CLIENT_FIFO, getpid());
  if(mkfifo(client_fifo_name, 0777) == 0)
  {
    printf("O FIFO de resposta criado.\n\n");
  }
  else
  {   
    printf("Erro ao criar FIFO o de resposta.\n");
    printf("Cliente %d terminado.\n", getpid());
    exit(EXIT_FAILURE);
  }
  
  
  //Open client FIFO for reading, non blocking
  printf("A abrir FIFO de resposta para leitura, nao bloqueante...\n");
  if ((client_fifo_hwnd = open(client_fifo_name, O_RDONLY | O_NONBLOCK)) > 0) 
  {
    printf("FIFO de resposta aberto para leitura.\n\n");
  }
  else
  {
    printf("Erro ao abrir o fifo de resposta.\n");
    printf("Cliente %d terminado.\n", getpid());
    unlink(client_fifo_name);
    exit(EXIT_FAILURE);
  }
  
  
  //Changes client FIFO for blocking type.
  printf("A alterar FIFO de resposta para bloqueante...\n");
  if(fcntl(client_fifo_hwnd, F_SETFL, O_RDONLY) != -1)
  {
    printf("FIFO de resposta alterado para bloqueante.\n\n");
  }
  else
  {
    printf("Erro ao alterar o FIFO de resposta para bloqueante.\n");
    printf("Cliente %d terminado.\n", getpid());
    unlink(client_fifo_name);
    exit(EXIT_FAILURE);
  }
  
  
  //Open client FIFO for writing so it keeps alive
  printf("A abrir FIFO de resposta para escrita, keep alive...\n");
  if ((keep_alive_hwnd = open(client_fifo_name, O_WRONLY)) > 0) 
  {
    printf("FIFO de resposta aberto para escrita.\n\n");
  }
  else
  {
    printf("Erro ao abrir o fifo de resposta.\n");
    printf("Cliente %d terminado.\n", getpid());
    unlink(client_fifo_name);
    exit(EXIT_FAILURE);
  }


  //Start main loop
  while(1)
  {
    //Asks for the command or login (has limit and delay penalty)
    if(logged)
    {      
      printf("Comando: ");
      char_input(c.cmd, MAX_COMMAND);
    }
    else
    {   
      c.pid_client = getpid();
      
      printf("Utilizador: ");
      char_input(c.current_user.username, MAX_USERNAME); 
      
      password = getpass("Enter password:");
      strcpy(c.current_user.password, password);
      
      sprintf(c.cmd,"login");
    }
    
    send_command(c);
    
    printf("Le mensagem no FIFO do cliente...\n");
    if(read(client_fifo_hwnd, &m, sizeof(m)) > 0)
    {	
      printf("\n%s\n", m.msg);
      
      if(strcasecmp(c.cmd, "login") == 0)
      {
	if(m.error == 0)
	{
	  logged = TRUE;
	}
	else
	{	  
	  printf("Espere 5 segundos.\n");
	  sleep(5);
	  count++;
	  
	  if(count == 3)
	  {
	    printf("Limite atinguido!");
	    kill(getpid(), SIGUSR2);
	  }  
	}	  
      }
      else if((strcasecmp(c.cmd, "logout") == 0) && (m.error == 0))
      {
	logged = FALSE;
      }
    }
    
  } 
  
}

/**
 * @brief shutdown_client
 *      Shutdown current client, this function is used for signal callback.
 * @param signal
 *      The signal received.
 */
void shutdown_client(int signal)
{
  char client_fifo_name[FIFONAME_SIZE];
  sprintf(client_fifo_name, CLIENT_FIFO, getpid());
  
  if(signal != 10 && logged)
  {
    sprintf(c.cmd,"logout");
    send_command(c);
  }
  
  if(signal == 2)
  {
    printf("\nParagem a pedido do utilizador.\n");
    printf("Cliente %d terminado.\n", getpid());
    unlink(client_fifo_name); 
    exit(EXIT_SUCCESS);
  }
  else if(signal == 10)
  {
    printf("\nPagarem a pedido do servidor.\n");
    printf("Cliente %d terminado.\n", getpid());
    unlink(client_fifo_name);   
    exit(EXIT_SUCCESS);
  }
  else if(signal == 12)
  {
    printf("\nParagem por erro!\n");
    printf("Cliente %d terminado.\n", getpid());
    unlink(client_fifo_name);   
    exit(EXIT_FAILURE);
  }
  
}

/**
 * @brief send_command
 *      Sends the typed command to the server.
 * @param c
 *      The command to send.
 */
void send_command(command c)
{
    int server_fifo_hwnd;
  
    //Open server FIFO for writing
    printf("\nA abrir FIFO do servidor para escrita...\n");
    if((server_fifo_hwnd = open(SERVER_FIFO, O_WRONLY)) > 0)
    {
      printf("O FIFO do servidor foi aberto para escrita.\n\n");
    }
    else
    {
      printf("Erro ao abrir o FIFO do servidor.\n");
      kill(getpid(), SIGUSR2);
    }
    
    //Update server FIFO with the command
    printf("A escrever no FIFO do servidor...\n");
    if(write(server_fifo_hwnd, &c, sizeof(c)) > 0)
    {
      printf("Dados enviados para o servidor!\n\n");
    }
    else
    {
      printf("Erro ao escrever no FIFO do servidor.\n");
      kill(getpid(), SIGUSR2);
    }
    
    close(server_fifo_hwnd);
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
