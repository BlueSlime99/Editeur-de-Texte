#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <poll.h>

#define Left "\33[D"
#define Right "\33[C"
#define Up "\33[A"
#define Down "\33[B"
#define Delete 127
#define ESC "\33"
#define Normal ""
#define Insert ""
#define Replace "R"

void affiche(char tab[1024][1024], int ligne, int colonne[], int down_page)
{
	system("clear");
	for (int i = 0; i < ligne && i < down_page - 1; i++)
	{
		for (int j = 0; j < colonne[i]; j++)
		{
			if (tab[i][j] == '\n')
			{
				write(STDOUT_FILENO, "\r\n", 2);
			}
			else
			{
				write(STDOUT_FILENO, &tab[i][j], 1);
			}
		}
	}
}

void cursor(int x, int y)
{
	char buffer[1024];
	sprintf(buffer, "\x1b[%d;%dH", x, y);
	write(STDOUT_FILENO, buffer, strlen(buffer));
}

int main(int argc, char *argv[])
{
	struct pollfd pollfds[2];
	pollfds[0].fd = STDIN_FILENO;
	pollfds[0].events = POLLIN;
	pollfds[1].events = POLLIN;
	struct winsize w_size;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &w_size);
	int down_page = w_size.ws_row;
	int right_page = w_size.ws_row;
	char buffer[1024];
	char in_buff[1024], mouse_buff[1024];
	char tab_file[1024][1024];
	int cp_colonne[1024] = {0};
	int l = 0, line = 0, k = 0;
	int x = 1, y = 1;
	int buff_size, fd;
	int in_size;
	bool normal_mode = true, insert_mode = false, replace_mode = false;
	if (argv[1] != NULL)
	{
		fd = open(argv[1], O_RDWR);
	}else{
		fd = open("copie", O_RDWR);
	}
	int mouse = open("/dev/input/mice", O_RDONLY);
	pollfds[1].fd = mouse;
	int clic_mouse_x = 1;
	int clic_mouse_y = 1;
	int fd_tab = open("table", O_RDWR);
	system("clear");

	/*Lire LE FICHIER ET REMPLIR LE TABLEAU*/

	while ((buff_size = read(fd, buffer, 1)) != 0)
	{
		if (buffer[0] == '\n')
		{
			tab_file[l][k] = '\n';
			cp_colonne[l]++;
			l++;
			k = 0;
		}
		else
		{
			tab_file[l][k] = buffer[0];
			k++;
			cp_colonne[l]++;
		}
	}
	line = l + 1;

	/*	sprintf(in_buff, "l = %d cp = %d", l, cp_colonne[l]);
	write(fd_tab, in_buff, strlen(in_buff));*/

	/*METTRE EN MODE RAW*/
	struct termios termios_p;
	struct termios termios_c;
	tcgetattr(STDIN_FILENO, &termios_p);
	cfmakeraw(&termios_c);
	tcsetattr(STDIN_FILENO, TCSADRAIN, &termios_c);
	affiche(tab_file, line, cp_colonne, down_page);
	cursor(x, y);
	/*GERER LES MODIFICATION ET LES AJOUT DANS LE TABLEAU*/
	l = 0;
	k = 0;
	while ((poll(pollfds, 2, -1) > 0))
	{

		if (pollfds[0].revents & POLLIN)
		{
			in_size = read(STDIN_FILENO, in_buff, 1024);
			in_buff[in_size] = '\0';
			if (replace_mode)
			{
				/*SAUT DE LIGNE*/
				if (in_buff[0] == '\r')
				{
					if (tab_file[l][k] != '\0')
					{

						int i;
						/*DEPLACER TOUTES LES LIGNES APRES UN \N*/
						for (i = line; i > l; i--)
						{
							for (int j = 0; j < cp_colonne[i - 1]; j++)
							{
								tab_file[i][j] = tab_file[i - 1][j];
							}
							/*A CHAQUE LIGNE ON AFFECTE LE NOMBRE AVANT DE COLONNE*/
							cp_colonne[i] = cp_colonne[i - 1];
						}
						/*AUGMENTER LA LIGNE AJOUTÉE*/
						l++;

						if (tab_file[l - 1][k] != '\n')
						{
							int c = 0;
							for (i = k; i < cp_colonne[l - 1]; i++)
							{
								tab_file[l][c] = tab_file[l - 1][i];
								c++;
							}
							tab_file[l - 1][k] = '\n';
							cp_colonne[l - 1] = cp_colonne[l - 1] - c + 1;
							cp_colonne[l] = c;
						}
						else
						{
							tab_file[l][0] = '\n';
							for (int j = 1; j < cp_colonne[l]; j++)
							{
								tab_file[l][j] = 0;
							}
							cp_colonne[l] = 1;
						}
						k = 0;
						x++;
						y = 1;
						line++;
					}
					else
					{
						tab_file[l][k] = '\n';
						cp_colonne[l]++;
						l++;
						line++;
						k = 0;
						x++;
						y = 1;
					}
				}
				else if (!strcmp(in_buff, ESC))
				{
					normal_mode = true;
					replace_mode = false;
					insert_mode = false;
				}
				else if (!strcmp(in_buff, Right))
				{
					if (k < cp_colonne[l] - 1)
					{
						k++;
						y++;
					}
				}
				else if (!strcmp(in_buff, Left))
				{
					if (k != 0)
					{
						k--;
						y--;
					}
				}
				else if (!strcmp(in_buff, Up))
				{
					if (l != 0)
					{
						if (k > cp_colonne[l - 1] - 1)
						{
							k = cp_colonne[l - 1] - 1;
							y = k + 1;
						}
						l--;
						x--;
					}
				}
				else if (!strcmp(in_buff, Down))
				{
					if (l < line - 1)
					{
						if (k > cp_colonne[l + 1] - 1)
						{
							k = cp_colonne[l + 1] - 1;
							y = k + 1;
						}
						l++;
						x++;
					}
				}
				else if (in_buff[0] == Delete)
				{
					if (k != 0)
					{
						y--;
						k--;
					}
				}
				else
				{
					if (tab_file[l][k] == '\0')
					{
						cp_colonne[l]++;
						tab_file[l][k] = in_buff[0];
						k++;
						y++;
					}
					else if (tab_file[l][k] == '\n')
					{
						tab_file[l][k] = in_buff[0];
						tab_file[l][k + 1] = '\n';
						cp_colonne[l]++;
						k++;
						y++;
					}
					else
					{
						tab_file[l][k] = in_buff[0];
						k++;
						y++;
					}
				}
			}
			else if (normal_mode)
			{
				if (in_buff[0] == ':')
				{
					cursor(down_page, 1);
					tcsetattr(STDIN_FILENO, TCSADRAIN, &termios_p);
					int size = read(STDIN_FILENO, in_buff, 1024);
					in_buff[size] = '\0';
					if (in_buff[0] == 'q')
					{
						exit(0);
					}
					else if (in_buff[0] == 'w')
					{
						int fd = open("copie", O_WRONLY | O_CREAT, 0644);
						for (int i = 0; i < line; i++)
						{
							for (int j = 0; j < cp_colonne[i]; j++)
							{
								write(fd, &tab_file[i][j], 1);
							}
						}
						if (fork() == 0)
						{
							execlp("mv", "mv", "copie", argv[1], NULL);
						}
						wait(NULL);
						close(fd);
					}
					tcsetattr(STDIN_FILENO, TCSADRAIN, &termios_c);
				}
				else if (in_buff[0] == 'i')
				{
					insert_mode = true;
					normal_mode = false;
					replace_mode = false;
				}
				else if (!strcmp(in_buff, Right))
				{

					if (k < cp_colonne[l] - 1 && tab_file[l][k] != '\n')
					{
						k++;
						y++;
					}
				}
				else if (!strcmp(in_buff, Left))
				{
					if (k != 0)
					{
						k--;
						y--;
					}
				}
				else if (!strcmp(in_buff, Up))
				{
					if (l != 0)
					{
						if (k > cp_colonne[l - 1] - 1)
						{
							k = cp_colonne[l - 1] - 1;
							y = k + 1;
						}
						l--;
						x--;
					}
				}
				else if (!strcmp(in_buff, Down))
				{
					if (l < line - 1)
					{
						if (k > cp_colonne[l + 1] - 1)
						{
							k = cp_colonne[l + 1] - 1;
							y = k + 1;
						}
						l++;
						x++;
					}
				}
				else if (in_buff[0] == 'r')
				{
					normal_mode = false;
					insert_mode = false;
					replace_mode = true;
				}
			}
			else if (insert_mode)
			{
			  if (in_buff[0] == '\r')
				{
					if (tab_file[l][k] != '\0')
					{

						int i;
						/*DEPLACER TOUTES LES LIGNES APRES UN \N*/
						for (i = line; i > l; i--)
						{
							for (int j = 0; j < cp_colonne[i - 1]; j++)
							{
								tab_file[i][j] = tab_file[i - 1][j];
							}
							/*A CHAQUE LIGNE ON AFFECTE LE NOMBRE AVANT DE COLONNE*/
							cp_colonne[i] = cp_colonne[i - 1];
						}
						/*AUGMENTER LA LIGNE AJOUTÉE*/
						l++;

						if (tab_file[l - 1][k] != '\n')
						{
							int c = 0;
							for (i = k; i < cp_colonne[l - 1]; i++)
							{
								tab_file[l][c] = tab_file[l - 1][i];
								c++;
							}
							tab_file[l - 1][k] = '\n';
							cp_colonne[l - 1] = cp_colonne[l - 1] - c + 1;
							cp_colonne[l] = c;
						}
						else
						{
							tab_file[l][0] = '\n';
							for (int j = 1; j < cp_colonne[l]; j++)
							{
								tab_file[l][j] = 0;
							}
							cp_colonne[l] = 1;
						}
						k = 0;
						x++;
						y = 1;
						line++;
					}
					else
					{
						tab_file[l][k] = '\n';
						cp_colonne[l]++;
						l++;
						line++;
						k = 0;
						x++;
						y = 1;
					}
				}
				else if (!strcmp(in_buff, ESC))
				{
					replace_mode = false;
					insert_mode = false;
					normal_mode = true;
				}
				else if (!strcmp(in_buff, Right))
				{

					if (k < cp_colonne[l] - 1 && tab_file[l][k] != '\n')
					{
						k++;
						y++;
					}
				}
				else if (!strcmp(in_buff, Left))
				{
					if (k != 0)
					{
						k--;
						y--;
					}
				}
				else if (!strcmp(in_buff, Up))
				{
					if (l != 0)
					{
						if (k > cp_colonne[l - 1] - 1)
						{
							k = cp_colonne[l - 1] - 1;
							y = k + 1;
						}
						l--;
						x--;
					}
				}
				else if (!strcmp(in_buff, Down))
				{
					if (l < line - 1)
					{
						if (k > cp_colonne[l + 1] - 1)
						{
							k = cp_colonne[l + 1] - 1;
							y = k + 1;
						}
						l++;
						x++;
					}
				}else if(in_buff[0] == Delete)
				{
					if(k==0)
					{
						if(l!=0)
						{
							if(tab_file[l][k] == '\n'){
								cp_colonne[l]--;
								x--;
								l--;
						}
							}else{
								int c = cp_colonne[l-1];
								for(int i =l; i> 0; i--){
									cp_colonne[l -1] = c + cp_colonne[l];
									for(int j =0; j<cp_colonne[l]; j++){
										tab_file[i -1][c] = tab_file[i][j];
										c++;
									}
								}
						}
					


					}
				}
				else
				{
					if (tab_file[l][k] != '\n' && tab_file[l][k] != '\0' )
					{
						for (int i = cp_colonne[l]; i > k; i--)
						{
							tab_file[l][i + 1] = tab_file[l][i];
							tab_file[l][i] = in_buff[0];
						}
						//if(in_buff[0]!=0){
						cp_colonne[l]++;
						//}
						k++;
						y++;
					}
					else
					{
						if (tab_file[l][k] == '\0')
						{
							cp_colonne[l]++;
							tab_file[l][k] = in_buff[0];
							k++;
							y++;
						}else if (tab_file[l][k] == '\n')
						{
							tab_file[l][k] = in_buff[0];
							tab_file[l][k + 1] = '\n';
							cp_colonne[l]++;
							k++;
							y++;
					
						}else
						{
							tab_file[l][k] = in_buff[0];
							k++;
							y++;
						}
					}
				}
			}

			affiche(tab_file, line, cp_colonne, down_page);
		}
		/*GESTION DE LA SOURIS*/
		if (pollfds[1].revents & POLLIN)
		{
			read(mouse, mouse_buff, 1024);
			int clic = 0;
			int mouse_x, mouse_y;
			clic = mouse_buff[0] & 0x1;
			mouse_x = mouse_buff[1];
			mouse_y = mouse_buff[2];
			if (mouse_y > 0)
			{
				if (clic_mouse_y > 1)
				{
					clic_mouse_y--;
				}
			}
			else if (mouse_y < 0)
			{
				if (clic_mouse_y < down_page)
				{
					clic_mouse_y++;
				}
			}
			if (mouse_x < 0)
			{
				if (clic_mouse_x > 1)
					clic_mouse_x--;
			}
			else if (mouse_x > 0)
			{
				if (clic_mouse_x < right_page)
				{
					clic_mouse_x++;
				}
			}

			if (clic == 1)
			{
				if (clic_mouse_y > line)
				{
					x = line;
					l = line - 1;
				}
				else
				{
					x = clic_mouse_y;
					l = clic_mouse_y - 1;
				}

				if (clic_mouse_x > cp_colonne[l])
				{
					y = cp_colonne[l];
					k = y - 1;
				}
				else
				{
					y = clic_mouse_x;
					k = y - 1;
				}
			}
		}
		cursor(down_page - 3, 12);
		if (normal_mode)
		{
			write(1, "Simple mode : 1-Press :q to exit 2-Press :w to save    ", 52);
			write(1, "                      3-Press :i for the Insert mode 4-Press :r for the Replace mode",85);
			
		}
		else if (insert_mode)
		{
			write(1, "Insert mode", 12);
		}
		else if (replace_mode)
		{
			write(1, "Replace mode", 13);
		}
		cursor(x, y);
	}

	for (int i = 0; i < line; i++)
	{
		for (int j = 0; j < cp_colonne[i]; j++)
		{
			write(fd_tab, &tab_file[i][j], 1);
		}
	}

	tcsetattr(STDIN_FILENO, TCSADRAIN, &termios_p);
	return 0;
}